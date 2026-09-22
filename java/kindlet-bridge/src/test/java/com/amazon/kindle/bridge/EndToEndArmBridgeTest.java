package com.amazon.kindle.bridge;

import java.io.File;
import java.io.IOException;

/**
 * End-to-end integration test launching the cross-compiled ARMv6 C++ daemon
 * under QEMU and exchanging framed IPC messages via NativeProcessSupervisor.
 */
public class EndToEndArmBridgeTest {

    public static void main(String[] args) throws Exception {
        System.out.println("Running EndToEndArmBridgeTest with ARMv6 C++ binary under QEMU...");

        if (args.length < 1) {
            System.err.println("Usage: EndToEndArmBridgeTest <path-to-kindle_daemon>");
            System.exit(1);
        }

        File daemonBinary = new File(args[0]);
        if (!daemonBinary.exists()) {
            System.err.println("Daemon binary not found: " + daemonBinary);
            System.exit(2);
        }

        // Launcher running ARMv6 binary via qemu-arm with dynamic linker sysroot
        ProcessLauncher qemuLauncher = new ProcessLauncher() {
            public Process launch(String[] command, File workingDirectory) throws IOException {
                String[] qemuCmd = new String[]{
                    "qemu-arm",
                    "-L", "/usr/arm-linux-gnueabi",
                    command[0]
                };
                return Runtime.getRuntime().exec(qemuCmd, null, workingDirectory);
            }
        };

        File workingDir = daemonBinary.getParentFile();
        NativeProcessSupervisor supervisor = new NativeProcessSupervisor(qemuLauncher, workingDir);

        final Object lock = new Object();
        final NativeMessage[] receivedHolder = new NativeMessage[1];

        supervisor.start(daemonBinary, new NativeBridgeListener() {
            public void onMessageReceived(NativeMessage message) {
                System.out.println("Java Bridge received from C++ ARM: type=" + message.getType() + " id=" + message.getRequestId() + " payload=" + message.getPayloadAsString());
                synchronized (lock) {
                    receivedHolder[0] = message;
                    lock.notifyAll();
                }
            }

            public void onProcessTerminated(int exitCode) {
                System.out.println("ARM Daemon terminated with exitCode=" + exitCode);
            }
        });

        // 1. Send Ping to ARM C++ daemon
        System.out.println("Java Bridge sending PING to C++ ARM binary...");
        NativeMessage ping = new NativeMessage(NativeMessage.TYPE_PING, 1, "PING".getBytes());
        supervisor.sendMessage(ping);

        synchronized (lock) {
            lock.wait(3000);
        }

        if (receivedHolder[0] == null || receivedHolder[0].getType() != NativeMessage.TYPE_PONG) {
            System.err.println("FAIL: Expected PONG from ARM C++ daemon, got: " + receivedHolder[0]);
            supervisor.stop();
            System.exit(3);
        }
        System.out.println("PASS: PONG received from ARM C++ daemon: " + receivedHolder[0].getPayloadAsString());

        // 2. Send JSON command to ARM C++ daemon
        receivedHolder[0] = null;
        System.out.println("Java Bridge sending Command to C++ ARM binary...");
        NativeMessage cmd = new NativeMessage(NativeMessage.TYPE_COMMAND, 2, "{\"action\":\"status\"}".getBytes());
        supervisor.sendMessage(cmd);

        synchronized (lock) {
            lock.wait(3000);
        }

        if (receivedHolder[0] == null || receivedHolder[0].getType() != NativeMessage.TYPE_RESPONSE) {
            System.err.println("FAIL: Expected RESPONSE from ARM C++ daemon, got: " + receivedHolder[0]);
            supervisor.stop();
            System.exit(4);
        }
        System.out.println("PASS: Response received from ARM C++ daemon: " + receivedHolder[0].getPayloadAsString());

        // 3. Graceful shutdown
        supervisor.stop();
        System.out.println("PASS: End-to-end Kindlet C++ ARM bridge verification succeeded!");
    }
}
