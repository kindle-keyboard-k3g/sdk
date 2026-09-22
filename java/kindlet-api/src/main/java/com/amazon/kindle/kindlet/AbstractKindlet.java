package com.amazon.kindle.kindlet;

/**
 * Convenience abstract base class implementing {@link Kindlet}.
 * Subclasses can override only the lifecycle methods they require.
 */
public abstract class AbstractKindlet implements Kindlet {

    private KindletContext context;

    /**
     * Initializes the Kindlet instance and stores the provided context.
     *
     * @param context the KindletContext providing access to UI and system properties
     * @throws KindletExecutionException if the provided context is null
     */
    public void create(KindletContext context) throws KindletExecutionException {
        if (context == null) {
            throw new KindletExecutionException("KindletContext cannot be null");
        }
        this.context = context;
    }

    /**
     * Called when the Kindlet becomes active and visible on the display.
     * Subclasses can override this method to start animations, timers, or background workers.
     */
    public void start() {
        // Default no-op
    }

    /**
     * Called when the Kindlet is paused, hidden, or loses focus.
     * Subclasses can override this method to pause timers or suspend resource consumption.
     */
    public void stop() {
        // Default no-op
    }

    /**
     * Called when the Kindlet is being destroyed and purged from memory.
     * Releases the stored context reference.
     */
    public void destroy() {
        this.context = null;
    }

    /**
     * Returns the active {@link KindletContext} provided during initialization.
     *
     * @return the active KindletContext, or null if destroyed
     */
    public KindletContext getContext() {
        return this.context;
    }
}
