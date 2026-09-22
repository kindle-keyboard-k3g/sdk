package com.amazon.kindle.bridge;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

/**
 * Supervises extraction, lifecycle, and IPC streaming with native C++ child processes.
 */
public class NativeProcessSupervisor {

    private final ProcessLauncher launcher;
    private final File workingDir;
    private Process process;
    private NativeBridgeListener listener;
    private Thread readerThread;
    private volatile boolean running = false;

    public NativeProcessSupervisor(ProcessLauncher launcher, File workingDir) {
        this.launcher = launcher;
        this.workingDir = workingDir;
    }

    public File extractResource(String resourcePath, String targetFileName) throws IOException {
        InputStream in = getClass().getResourceAsStream(resourcePath);
        if (in == null) {
            throw new IOException("Resource not found: " + resourcePath);
        }
        File targetFile = new File(workingDir, targetFileName);
        OutputStream out = new FileOutputStream(targetFile);
        byte[] buf = new byte[4095];
        int read;
        try {
            while ((read = in.read(buf)) != -1) {
                out.write(buf, 0, read);
            }
        } finally {
            in.close();
            out.close();
        }

        // Set executable permissions in POSIX environment
        try {
            Runtime.getRuntime().exec(new String[]{"chmod", "755", targetFile.getAbsolutePath()}).waitFor();
        } catch (InterruptedException e) {
            Thread.currentThread().interrupt();
        }

        return targetFile;
    }

    public void start(File executableFile, NativeBridgeListener listener) throws IOException {
        this.listener = listener;
        String[] cmd = new String[]{executableFile.getAbsolutePath()};
        this.process = launcher.launch(cmd, workingDir);
        this.running = true;

        this.readerThread = new Thread(new Runnable() {
            public void run() {
                InputStream in = process.getInputStream();
                while (running) {
                    try {
                        NativeMessage msg = NativeMessage.readFrom(in);
                        if (NativeProcessSupervisor.this.listener != null) {
                            NativeProcessSupervisor.this.listener.onMessageReceived(msg);
                        }
                    } catch (IOException e) {
                        break;
                    }
                }
                if (NativeProcessSupervisor.this.listener != null) {
                    try {
                        int exitCode = process.waitFor();
                        NativeProcessSupervisor.this.listener.onProcessTerminated(exitCode);
                    } catch (InterruptedException e) {
                        Thread.currentThread().interrupt();
                    }
                }
            }
        });
        this.readerThread.setDaemon(true);
        this.readerThread.start();
    }

    public void sendMessage(NativeMessage message) throws IOException {
        if (process != null && running) {
            message.writeTo(process.getOutputStream());
        } else {
            throw new IOException("Native process is not running");
        }
    }

    public void stop() {
        this.running = false;
        if (process != null) {
            try {
                // Send shutdown frame
                NativeMessage shutdownMsg = new NativeMessage(NativeMessage.TYPE_SHUTDOWN, 0, new byte[0]);
                sendMessage(shutdownMsg);
            } catch (Exception ignored) {
            }
            process.destroy();
            process = null;
        }
    }
}
