package com.amazon.kindle.kindlet;

/**
 * The core lifecycle interface for Amazon Kindle Active Content (Kindlets).
 * Executed under Sun CVM (CDC 1.1 / Personal Basis Profile 1.1).
 */
public interface Kindlet {

    /**
     * Initializes the Kindlet with its execution context.
     * This method must return promptly to avoid blocking the Kindle framework.
     *
     * @param context the KindletContext providing access to UI and system properties
     * @throws KindletExecutionException if initialization fails
     */
    void create(KindletContext context) throws KindletExecutionException;

    /**
     * Called when the Kindlet becomes active and visible on the display.
     */
    void start();

    /**
     * Called when the Kindlet is paused, hidden, or loses focus.
     */
    void stop();

    /**
     * Called when the Kindlet is being destroyed and purged from memory.
     * All background threads, resources, and child processes must be cleaned up here.
     */
    void destroy();
}
