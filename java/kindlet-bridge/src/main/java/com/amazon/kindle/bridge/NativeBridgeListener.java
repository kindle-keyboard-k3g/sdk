package com.amazon.kindle.bridge;

public interface NativeBridgeListener {
    void onMessageReceived(NativeMessage message);
    void onProcessTerminated(int exitCode);
}
