package com.amazon.kindle.kindlet;

/**
 * Convenience abstract base class implementing {@link Kindlet}.
 * Subclasses can override only the lifecycle methods they require.
 */
public abstract class AbstractKindlet implements Kindlet {

    private KindletContext context;

    public void create(KindletContext context) throws KindletExecutionException {
        if (context == null) {
            throw new KindletExecutionException("KindletContext cannot be null");
        }
        this.context = context;
    }

    public void start() {
        // Default no-op
    }

    public void stop() {
        // Default no-op
    }

    public void destroy() {
        this.context = null;
    }

    public KindletContext getContext() {
        return this.context;
    }
}
