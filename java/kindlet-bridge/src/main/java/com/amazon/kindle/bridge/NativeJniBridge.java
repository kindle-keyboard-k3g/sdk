package com.amazon.kindle.bridge;

/**
 * Optional JNI bridge wrapper for direct in-process native execution.
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

    public static boolean isAvailable() {
        return loaded;
    }

    public static byte[] transact(int commandId, byte[] input) {
        if (!loaded) {
            throw new UnsupportedOperationException("JNI bridge library not available");
        }
        return nativeTransact(commandId, input);
    }

    private static native byte[] nativeTransact(int commandId, byte[] input);
}
