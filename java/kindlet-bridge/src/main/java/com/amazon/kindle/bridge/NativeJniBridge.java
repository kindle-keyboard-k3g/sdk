package com.amazon.kindle.bridge;

/**
 * Optional in-process JNI bridge wrapper for direct CVM native calls.
 */
public class NativeJniBridge {

    private static boolean loaded = false;

    static {
        try {
            System.loadLibrary("kindle_native");
            loaded = true;
        } catch (UnsatisfiedLinkError e) {
            loaded = false;
        }
    }

    /**
     * Checks if the native library was successfully loaded into CVM.
     *
     * @return true if JNI binding is functional, false otherwise
     */
    public static boolean isAvailable() {
        return loaded;
    }

    /**
     * Executes an in-process transaction across JNI boundary.
     *
     * @param commandId command identifier
     * @param input serialized input buffer
     * @return serialized output buffer
     */
    public static byte[] transact(int commandId, byte[] input) {
        if (!loaded) {
            throw new UnsupportedOperationException("JNI bridge library not available");
        }
        return nativeTransact(commandId, input);
    }

    private static native byte[] nativeTransact(int commandId, byte[] input);
}
