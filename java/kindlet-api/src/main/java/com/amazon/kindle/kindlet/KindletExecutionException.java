package com.amazon.kindle.kindlet;

/**
 * Thrown when an error occurs during Kindlet lifecycle execution.
 */
public class KindletExecutionException extends Exception {

    public KindletExecutionException() {
        super();
    }

    public KindletExecutionException(String message) {
        super(message);
    }

    public KindletExecutionException(String message, Throwable cause) {
        super(message, cause);
    }

    public KindletExecutionException(Throwable cause) {
        super(cause);
    }
}
