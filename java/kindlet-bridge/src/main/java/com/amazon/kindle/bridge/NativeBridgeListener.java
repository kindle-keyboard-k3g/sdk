package com.amazon.kindle.bridge;

/**
 * Event callback listener receiving framed messages and exit notifications from native processes.
 */
public interface NativeBridgeListener {

    /**
     * Invoked when a complete framed message is received from the native process stdout.
     *
     * @param message decoded NativeMessage frame
     */
    void onMessageReceived(NativeMessage message);

    /**
     * Invoked when the native child process exits.
     *
     * @param exitCode process exit status code
     */
    void onProcessTerminated(int exitCode);
}
