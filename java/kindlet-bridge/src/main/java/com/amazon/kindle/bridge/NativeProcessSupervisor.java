package com.amazon.kindle.bridge;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

/**
 * Supervises extraction, lifecycle, and IPC streaming with native C++ child processes.
 * Complies with Java CDC 1.1 / Personal Basis Profile 1.1 constraints.
 */
public class NativeProcessSupervisor {

    private final ProcessLauncher launcher;
    private final File workingDir;
    private Process process;
    private NativeBridgeListener listener;
    private Thread readerThread;
    private volatile boolean running = false;

    /**
     * Constructs a supervisor using the provided launcher and working directory.
     *
     * @param launcher ProcessLauncher strategy
     * @param workingDir working directory where binaries are extracted and executed
     */
    public NativeProcessSupervisor(ProcessLauncher launcher, File workingDir) {
        this.launcher = launcher;
        this.workingDir = workingDir;
    }

    /**
     * Extracts an embedded native binary from JAR resource to disk and sets execution permissions.
     *
     * @param resourcePath classpath to the binary resource (e.g. "/bin/armv6/kindle_daemon")
     * @param targetFileName name of destination file on disk
     * @return File object referencing extracted binary
     * @throws IOException if extraction fails
     */
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

    /**
     * Launches the native binary and starts background message reading thread.
     *
     * @param executableFile target executable
     * @param listener event listener for incoming messages and termination
     * @throws IOException if launch fails
     */
    public void start(File executableFile, NativeBridgeListener listener) throws IOException {
        start(executableFile, listener, new String[0]);
    }

    /**
     * Launches the native binary with additional command-line arguments.
     *
     * @param executableFile target executable
     * @param listener event listener for incoming messages and termination
     * @param extraArguments additional arguments passed after the executable
     * @throws IOException if launch fails
     */
    public void start(File executableFile, NativeBridgeListener listener,
                      String[] extraArguments) throws IOException {
        if (executableFile == null) {
            throw new IllegalArgumentException("executableFile must not be null");
        }
        synchronized (this) {
            this.listener = listener;
            String[] arguments = (extraArguments != null) ? extraArguments : new String[0];
            String[] cmd = new String[arguments.length + 1];
            cmd[0] = executableFile.getAbsolutePath();
            System.arraycopy(arguments, 0, cmd, 1, arguments.length);
            this.process = launcher.launch(cmd, workingDir);
            this.running = true;
            final Process processForReader = this.process;

            this.readerThread = new Thread(new Runnable() {
                public void run() {
                    InputStream in = processForReader.getInputStream();
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

                    int exitCode = -1;
                    try {
                        exitCode = processForReader.waitFor();
                    } catch (InterruptedException e) {
                        Thread.currentThread().interrupt();
                        try {
                            exitCode = processForReader.exitValue();
                        } catch (IllegalThreadStateException ignored) {
                        }
                    }
                    if (NativeProcessSupervisor.this.listener != null) {
                        NativeProcessSupervisor.this.listener.onProcessTerminated(exitCode);
                    }
                }
            });
            this.readerThread.setDaemon(true);
            this.readerThread.start();
        }
    }

    /**
     * Sends a framed IPC message to the child process stdin.
     *
     * @param message frame to serialize and send
     * @throws IOException if the process is not running or stream fails
     */
    public void sendMessage(NativeMessage message) throws IOException {
        synchronized (this) {
            if (message == null) {
                throw new IllegalArgumentException("message must not be null");
            }
            if (process != null && running) {
                message.writeTo(process.getOutputStream());
            } else {
                throw new IOException("Native process is not running");
            }
        }
    }

    /**
     * Gracefully stops the child process by transmitting a shutdown frame, then destroying the process.
     */
    public void stop() {
        synchronized (this) {
            if (process != null) {
                try {
                    NativeMessage shutdownMsg = new NativeMessage(
                        NativeMessage.TYPE_SHUTDOWN, 0, new byte[0]);
                    if (running) {
                        sendMessage(shutdownMsg);
                    }
                } catch (Exception ignored) {
                }
                running = false;
                process.destroy();
                process = null;
            } else {
                running = false;
            }
        }
    }
}
