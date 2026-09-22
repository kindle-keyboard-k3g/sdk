package com.amazon.kindle.emulator;

import com.amazon.kindle.kindlet.KindletContext;
import java.awt.Container;
import java.awt.Panel;
import java.io.File;

/**
 * Kindle simulator implementation of {@link KindletContext}.
 * Provides mock root container, isolated home directory, and orientation tracking.
 */
public class SimulatorKindletContext implements KindletContext {

    private final Container rootContainer = new Panel();
    private final File homeDirectory;
    private int orientation = 0;

    /**
     * Constructs a simulator context configured with a specific local storage directory.
     *
     * @param homeDirectory base directory for application data
     */
    public SimulatorKindletContext(File homeDirectory) {
        this.homeDirectory = homeDirectory;
    }

    /**
     * Returns the simulated root AWT container.
     *
     * @return Container instance
     */
    public Container getRootContainer() {
        return rootContainer;
    }

    /**
     * Returns the application private storage directory.
     *
     * @return File object
     */
    public File getHomeDirectory() {
        return homeDirectory;
    }

    /**
     * Returns current simulated screen orientation.
     *
     * @return orientation code
     */
    public int getOrientation() {
        return orientation;
    }

    /**
     * Sets the simulated screen orientation.
     *
     * @param orientation target orientation
     */
    public void setOrientation(int orientation) {
        this.orientation = orientation;
    }
}
