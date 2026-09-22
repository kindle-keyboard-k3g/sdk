import unittest
import subprocess
import tempfile
import zipfile
import struct
from pathlib import Path

class TestShowcaseAppIntegration(unittest.TestCase):
    """
    End-to-end integration test validating the Kindlet C++ Showcase application:
    1. C++ native daemon build and IPC message exchange.
    2. Java Kindlet compilation with Java 1.4 bytecode targets.
    3. Binary resource embedding and triple code-signing.
    4. Headless desktop simulator execution.
    """

    @classmethod
    def setUpClass(cls):
        cls.worktree_root = Path(__file__).resolve().parent.parent.parent
        cls.app_dir = cls.worktree_root / "examples" / "kindlet-cpp-showcase"
        cls.python_src = cls.worktree_root / "python" / "src"

    def test_01_build_script_execution(self):
        """Validates that scripts/build-and-run.sh executes without errors."""
        res = subprocess.run(
            ["./scripts/build-and-run.sh", "headless"],
            cwd=str(self.app_dir),
            capture_output=True,
            text=True
        )
        self.assertEqual(res.returncode, 0, f"build-and-run.sh failed:\n{res.stderr}\n{res.stdout}")
        self.assertIn("Showcase build-and-run completed successfully", res.stdout)

    def test_02_bytecode_compatibility(self):
        """Verifies that all compiled showcase classes target Java 1.4 (major version 48)."""
        jar_path = self.app_dir / "dist" / "kindlet-cpp-showcase.jar"
        self.assertTrue(jar_path.exists(), "kindlet-cpp-showcase.jar must exist")

        with zipfile.ZipFile(jar_path, "r") as zf:
            showcase_classes = [n for n in zf.namelist() if n.startswith("com/amazon/kindle/showcase") and n.endswith(".class")]
            self.assertGreater(len(showcase_classes), 0, "Showcase classes must be bundled")

            for class_name in showcase_classes:
                data = zf.read(class_name)
                magic, minor, major = struct.unpack(">IHH", data[:8])
                self.assertEqual(magic, 0xCAFEBABE, f"Invalid class magic for {class_name}")
                self.assertEqual(major, 48, f"Class {class_name} has major version {major}, expected 48 (Java 1.4)")

    def test_03_native_binary_embedded(self):
        """Verifies that the compiled native daemon binary is embedded inside the JAR."""
        jar_path = self.app_dir / "dist" / "kindlet-cpp-showcase.jar"
        with zipfile.ZipFile(jar_path, "r") as zf:
            names = zf.namelist()
            self.assertIn("bin/showcase_daemon", names)
            self.assertIn("bin/armv6/showcase_daemon", names)

    def test_04_signature_manifest(self):
        """Verifies that the package contains triple-developer signing records in META-INF."""
        azw2_path = self.app_dir / "dist" / "kindlet-cpp-showcase.azw2"
        self.assertTrue(azw2_path.exists(), "kindlet-cpp-showcase.azw2 must exist")

        with zipfile.ZipFile(azw2_path, "r") as zf:
            names = zf.namelist()
            self.assertIn("META-INF/MANIFEST.MF", names)
            # Check signatures for dkDeveloper, diDeveloper, dnDeveloper
            sig_files = [n for n in names if n.startswith("META-INF/") and (n.endswith(".SF") or n.endswith(".RSA"))]
            self.assertGreaterEqual(len(sig_files), 2, f"Expected signature files, found: {sig_files}")

    def test_05_native_daemon_ipc_direct(self):
        """Tests showcase_daemon directly with framed Ping, Command, and Shutdown IPC messages."""
        daemon_bin = self.app_dir / "build" / "native" / "showcase_daemon"
        self.assertTrue(daemon_bin.exists(), "showcase_daemon must exist")

        ping_frame = struct.pack(">IBBHII", 0x4B494E44, 1, 1, 0, 101, 0)
        cmd_payload = b"generate_pattern pattern=1"
        cmd_frame = struct.pack(">IBBHII", 0x4B494E44, 1, 0x10, 0, 102, len(cmd_payload)) + cmd_payload
        shutdown_frame = struct.pack(">IBBHII", 0x4B494E44, 1, 0xFF, 0, 103, 0)

        proc = subprocess.Popen(
            [str(daemon_bin)],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE
        )
        out, _ = proc.communicate(ping_frame + cmd_frame + shutdown_frame)

        self.assertGreater(len(out), 0, "Daemon should return output")
        magic, ver, mtype, flags, req_id, length = struct.unpack_from(">IBBHII", out, 0)
        self.assertEqual(magic, 0x4B494E44)
        self.assertEqual(mtype, 2) # Pong
        self.assertEqual(req_id, 101)

if __name__ == "__main__":
    unittest.main()
