package com.amazon.kindle.bridge;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

public class NativeProcessSupervisorTest {

    static class MockProcess extends Process {
        private final ByteArrayOutputStream out = new ByteArrayOutputStream();
        private final ByteArrayInputStream in;
        private boolean destroyed = false;

        public MockProcess(byte[] responseData) {
            this.in = new ByteArrayInputStream(responseData);
        }

        public OutputStream getOutputStream() { return out; }
        public InputStream getInputStream() { return in; }
        public InputStream getErrorStream() { return new ByteArrayInputStream(new byte[0]); }
        public int waitFor() { return 0; }
        public int exitValue() { return 0; }
        public void destroy() { this.destroyed = true; }
    }

    public static void main(String[] args) throws Exception {
        System.out.println("Running NativeProcessSupervisorTest...");

        // Test serialization and deserialization
        ByteArrayOutputStream pipe = new ByteArrayOutputStream();
        NativeMessage sendMsg = new NativeMessage(NativeMessage.TYPE_COMMAND, 101, "{\"action\":\"status\"}".getBytes());
        sendMsg.writeTo(pipe);

        ByteArrayInputStream readPipe = new ByteArrayInputStream(pipe.toByteArray());
        NativeMessage recvMsg = NativeMessage.readFrom(readPipe);

        if (recvMsg.getType() != NativeMessage.TYPE_COMMAND) {
            System.err.println("FAIL: Type mismatch");
            System.exit(1);
        }
        if (recvMsg.getRequestId() != 101) {
            System.err.println("FAIL: Request ID mismatch");
            System.exit(1);
        }
        if (!recvMsg.getPayloadAsString().equals("{\"action\":\"status\"}")) {
            System.err.println("FAIL: Payload mismatch");
            System.exit(1);
        }

        // Test Mock Process Supervisor
        final NativeMessage[] receivedHolder = new NativeMessage[1];
        final MockProcess mockProc = new MockProcess(pipe.toByteArray());

        ProcessLauncher mockLauncher = new ProcessLauncher() {
            public Process launch(String[] command, File workingDirectory) {
                return mockProc;
            }
        };

        NativeProcessSupervisor supervisor = new NativeProcessSupervisor(mockLauncher, new File("/tmp"));
        supervisor.start(new File("/tmp/dummy"), new NativeBridgeListener() {
            public void onMessageReceived(NativeMessage message) {
                receivedHolder[0] = message;
            }
            public void onProcessTerminated(int exitCode) {}
        });

        Thread.sleep(100);
        supervisor.stop();

        if (receivedHolder[0] == null || receivedHolder[0].getRequestId() != 101) {
            System.err.println("FAIL: Supervisor did not dispatch received message");
            System.exit(1);
        }

        System.out.println("PASS: NativeProcessSupervisorTest verified successfully!");
    }
}
