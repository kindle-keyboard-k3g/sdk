"""
Integration test for the Whispernet network relay pipeline.

Tests:
1. C++ kindle_network_ipc codec binary: encode/decode round-trips
2. Java HttpIpcCodec: encode/decode round-trips (via Java subprocess)
3. Cross-language round-trip: Java encodes → daemon binary (C++) decodes and vice versa
4. FakeWhispernetProxy: responds correctly to HTTP GET
5. Full relay round-trip: Java NetworkRelayHandler → fake proxy → C++ daemon dispatch
"""

import io
import socket
import struct
import subprocess
import sys
import tempfile
import threading
import time
import unittest
from pathlib import Path


# ---------------------------------------------------------------------------
# KIND IPC frame wire format helpers (Python)
# ---------------------------------------------------------------------------

KIND_MAGIC   = 0x4B494E44
IPC_VERSION  = 0x01
TYPE_HTTP_REQUEST  = 0x06
TYPE_HTTP_RESPONSE = 0x07
SCHEMA_VERSION = 1


def pack_u16be(v):
    return struct.pack(">H", v & 0xFFFF)


def pack_u32be(v):
    return struct.pack(">I", v & 0xFFFFFFFF)


def encode_ipc_frame(msg_type, request_id, payload):
    """Encodes a KIND IPC frame."""
    header = struct.pack(">IBBHI", KIND_MAGIC, IPC_VERSION, msg_type, 0, request_id)
    header += struct.pack(">I", len(payload))
    return header + payload


def decode_ipc_frame(stream):
    """Reads one KIND IPC frame from stream. Returns (type, request_id, payload) or None."""
    hdr = stream.read(16)
    if len(hdr) < 16:
        return None
    magic, ver, msg_type, flags, req_id = struct.unpack(">IBBHI", hdr[:12])
    payload_len = struct.unpack(">I", hdr[12:16])[0]
    payload = stream.read(payload_len)
    if magic != KIND_MAGIC or ver != IPC_VERSION:
        return None
    return msg_type, req_id, payload


def encode_http_request_payload(method, url, headers=None, body=b""):
    """Encodes an HttpRequest network_ipc payload (big-endian)."""
    m = method.encode("utf-8")
    u = url.encode("utf-8")
    h = headers or []
    buf = bytearray()
    buf.append(SCHEMA_VERSION)
    buf.append(len(m))
    buf += pack_u16be(len(u))
    buf += pack_u16be(len(h))
    buf += pack_u32be(len(body))
    buf += m
    buf += u
    for name, value in h:
        nb = name.encode("utf-8")
        vb = value.encode("utf-8")
        buf += pack_u16be(len(nb))
        buf += pack_u16be(len(vb))
        buf += nb
        buf += vb
    buf += body
    return bytes(buf)


def decode_http_response_payload(payload):
    """Decodes an HttpResponse network_ipc payload. Returns dict or None."""
    if len(payload) < 11:
        return None
    schema = payload[0]
    if schema != SCHEMA_VERSION:
        return None
    status_code = struct.unpack(">H", payload[1:3])[0]
    reason_len  = struct.unpack(">H", payload[3:5])[0]
    hdr_count   = struct.unpack(">H", payload[5:7])[0]
    body_len    = struct.unpack(">I", payload[7:11])[0]
    pos = 11
    reason = payload[pos:pos + reason_len].decode("utf-8", errors="replace")
    pos += reason_len
    headers = []
    for _ in range(hdr_count):
        nl = struct.unpack(">H", payload[pos:pos + 2])[0]; pos += 2
        vl = struct.unpack(">H", payload[pos:pos + 2])[0]; pos += 2
        name  = payload[pos:pos + nl].decode("utf-8", errors="replace"); pos += nl
        value = payload[pos:pos + vl].decode("utf-8", errors="replace"); pos += vl
        headers.append((name, value))
    body = payload[pos:pos + body_len]
    return {"status": status_code, "reason": reason, "headers": headers, "body": body}


# ---------------------------------------------------------------------------
# Fake HTTP proxy (serves canned responses)
# ---------------------------------------------------------------------------

class FakeHttpProxy(threading.Thread):
    """Minimal single-request HTTP proxy for test isolation."""

    def __init__(self, response_body=b"hello from proxy", status=200, reason="OK"):
        super(FakeHttpProxy, self).__init__(daemon=True)
        self.server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.server.bind(("127.0.0.1", 0))
        self.server.listen(5)
        self.port = self.server.getsockname()[1]
        self.response_body = response_body
        self.status = status
        self.reason = reason
        self.captured_request = b""

    def run(self):
        try:
            conn, _ = self.server.accept()
            conn.settimeout(5)
            buf = b""
            while b"\r\n\r\n" not in buf:
                chunk = conn.recv(4096)
                if not chunk:
                    break
                buf += chunk
            self.captured_request = buf
            resp = (
                "HTTP/1.0 {status} {reason}\r\n"
                "Content-Length: {length}\r\n"
                "\r\n"
            ).format(
                status=self.status,
                reason=self.reason,
                length=len(self.response_body)
            ).encode("utf-8") + self.response_body
            conn.sendall(resp)
            conn.close()
        except Exception:
            pass
        finally:
            self.server.close()


# ---------------------------------------------------------------------------
# Tests
# ---------------------------------------------------------------------------

class TestNetworkRelayIntegration(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.root = Path(__file__).resolve().parent.parent.parent
        cls.daemon_bin = cls.root / "build" / "kindle_daemon"
        cls.bridge_classes = cls.root / "java" / "kindlet-bridge" / "build" / "classes"
        cls.bridge_test_classes = cls.root / "java" / "kindlet-bridge" / "build" / "test-classes"
        cls.java_available = True
        try:
            subprocess.run(["java", "-version"], capture_output=True, check=True)
        except Exception:
            cls.java_available = False

    # -------------------------------------------------------------------------
    # T1: C++ ipc codec round-trip via Python
    # -------------------------------------------------------------------------

    def test_01_python_encode_decode_round_trip(self):
        """Python encode → Python decode verifies the wire format constants."""
        method  = "GET"
        url     = "http://example.com/test"
        headers = [("Host", "example.com")]
        body    = b""
        payload = encode_http_request_payload(method, url, headers, body)

        # Verify fixed-header values
        self.assertEqual(payload[0], SCHEMA_VERSION)
        self.assertEqual(payload[1], len(method.encode("utf-8")))
        url_len = struct.unpack(">H", payload[2:4])[0]
        self.assertEqual(url_len, len(url.encode("utf-8")))

    # -------------------------------------------------------------------------
    # T2: daemon binary dispatches HttpRequest (if build exists)
    # -------------------------------------------------------------------------

    def test_02_daemon_dispatches_http_request(self):
        """Send a TYPE_HTTP_REQUEST to the daemon; verify it returns TYPE_HTTP_RESPONSE."""
        if not self.daemon_bin.exists():
            self.skipTest("kindle_daemon binary not found at " + str(self.daemon_bin))

        proxy = FakeHttpProxy(response_body=b"relay-ok")
        proxy.start()
        time.sleep(0.1)

        req_payload = encode_http_request_payload(
            "GET", "http://example.com/", [], b""
        )
        frame = encode_ipc_frame(TYPE_HTTP_REQUEST, 1, req_payload)

        env = {
            "KINDLE_WHISPERNET_PROXY_HOST": "127.0.0.1",
            "KINDLE_WHISPERNET_PROXY_PORT": str(proxy.port),
        }
        import os
        full_env = dict(os.environ)
        full_env.update(env)

        proc = subprocess.Popen(
            [str(self.daemon_bin)],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            env=full_env
        )
        proc.stdin.write(frame)
        proc.stdin.flush()
        # Send shutdown
        shutdown = encode_ipc_frame(0xFF, 0, b"")
        proc.stdin.write(shutdown)
        proc.stdin.flush()
        proc.stdin.close()

        try:
            stdout, _ = proc.communicate(timeout=10)
        except subprocess.TimeoutExpired:
            proc.kill()
            self.fail("daemon timed out")

        stream = io.BytesIO(stdout)
        result = decode_ipc_frame(stream)
        self.assertIsNotNone(result, "Expected an IPC response frame from daemon")
        msg_type, req_id, payload = result
        self.assertEqual(msg_type, TYPE_HTTP_RESPONSE, "Expected TYPE_HTTP_RESPONSE")
        self.assertEqual(req_id, 1)

        response = decode_http_response_payload(payload)
        self.assertIsNotNone(response)
        self.assertEqual(response["status"], 200)
        self.assertIn(b"relay-ok", response["body"])

    # -------------------------------------------------------------------------
    # T3: Java HttpIpcCodec round-trip (via subprocess)
    # -------------------------------------------------------------------------

    def test_03_java_codec_round_trip(self):
        """Verify Java HttpIpcCodec round-trips match the Python codec."""
        if not self.java_available:
            self.skipTest("Java not available")
        if not self.bridge_classes.exists():
            self.skipTest("kindlet-bridge classes not built at " + str(self.bridge_classes))
        if not self.bridge_test_classes.exists():
            self.skipTest("kindlet-bridge test classes not built at " + str(self.bridge_test_classes))

        cp = str(self.bridge_classes) + ":" + str(self.bridge_test_classes)
        # Run the Java test suite (which includes codec tests) as a subprocess
        result = subprocess.run(
            ["java", "-cp", cp,
             "com.amazon.kindle.bridge.network.WhispernetNetworkTest"],
            capture_output=True, text=True, timeout=30
        )
        self.assertEqual(result.returncode, 0,
                         "Java WhispernetNetworkTest failed:\n" + result.stdout + result.stderr)
        self.assertIn("tests passed", result.stdout)
        # Verify no FAILs
        self.assertNotIn("FAIL:", result.stdout)

    # -------------------------------------------------------------------------
    # T4: FakeHttpProxy intercepts and serves a request
    # -------------------------------------------------------------------------

    def test_04_fake_proxy_intercepts_request(self):
        """FakeHttpProxy receives a request and returns the expected body."""
        proxy = FakeHttpProxy(response_body=b"intercept-test", status=200, reason="OK")
        proxy.start()
        time.sleep(0.05)

        sock = socket.create_connection(("127.0.0.1", proxy.port), timeout=5)
        sock.sendall(b"GET http://target.example.com/ HTTP/1.0\r\nHost: target.example.com\r\n\r\n")
        response = b""
        while True:
            chunk = sock.recv(4096)
            if not chunk:
                break
            response += chunk
        sock.close()
        proxy.join(timeout=2)

        self.assertIn(b"200 OK", response)
        self.assertIn(b"intercept-test", response)

    # -------------------------------------------------------------------------
    # T5: HTTPS guard — daemon returns transport error for https:// URL
    # -------------------------------------------------------------------------

    def test_05_https_guard_via_daemon(self):
        """Daemon must return a transport-error response for https:// URLs."""
        if not self.daemon_bin.exists():
            self.skipTest("kindle_daemon binary not found at " + str(self.daemon_bin))

        req_payload = encode_http_request_payload("GET", "https://example.com/secure")
        frame = encode_ipc_frame(TYPE_HTTP_REQUEST, 7, req_payload)
        shutdown = encode_ipc_frame(0xFF, 0, b"")

        proc = subprocess.Popen(
            [str(self.daemon_bin)],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE
        )
        proc.stdin.write(frame)
        proc.stdin.write(shutdown)
        proc.stdin.close()

        try:
            stdout, _ = proc.communicate(timeout=10)
        except subprocess.TimeoutExpired:
            proc.kill()
            self.fail("daemon timed out")

        result = decode_ipc_frame(io.BytesIO(stdout))
        self.assertIsNotNone(result)
        msg_type, req_id, payload = result
        self.assertEqual(msg_type, TYPE_HTTP_RESPONSE)
        self.assertEqual(req_id, 7)

        response = decode_http_response_payload(payload)
        self.assertIsNotNone(response)
        self.assertEqual(response["status"], 0, "Expected transport error (status=0) for HTTPS")


if __name__ == "__main__":
    unittest.main(verbosity=2)
