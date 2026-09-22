package com.amazon.kindle.emulator;

import com.amazon.kindle.kindlet.KindletContext;
import java.awt.Container;
import java.awt.Panel;
import java.io.File;

public class SimulatorKindletContext implements KindletContext {

    private final Container rootContainer = new Panel();
    private final File homeDirectory;
    private int orientation = 0;

    public SimulatorKindletContext(File homeDirectory) {
        this.homeDirectory = homeDirectory;
    }

    public Container getRootContainer() {
        return rootContainer;
    }

    public File getHomeDirectory() {
        return homeDirectory;
    }

    public int getOrientation() {
        return orientation;
    }

    public void setOrientation(int orientation) {
        this.orientation = orientation;
    }
}
