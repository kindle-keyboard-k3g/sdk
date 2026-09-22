package com.amazon.kindle.kindlet;

import java.awt.Container;
import java.io.File;

/**
 * Interface providing a Kindlet with access to the Kindle environment,
 * display root container, home directory, and orientation settings.
 */
public interface KindletContext {

    /**
     * Gets the root AWT container where the Kindlet should place its UI components.
     *
     * @return the root Container
     */
    Container getRootContainer();

    /**
     * Gets the private home directory for persistent local storage.
     *
     * @return File representing the app's directory
     */
    File getHomeDirectory();

    /**
     * Gets the current screen orientation.
     *
     * @return orientation code
     */
    int getOrientation();

    /**
     * Requests screen orientation lock or change.
     *
     * @param orientation target orientation
     */
    void setOrientation(int orientation);
}
