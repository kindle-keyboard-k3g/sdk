package com.amazon.kindle.showcase;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.util.Properties;

/**
 * Manages persistent user preferences and launch statistics in the Kindlet's isolated home directory.
 * Complies with Java CDC 1.1 / Personal Basis Profile 1.1 constraints.
 */
public class ShowcaseSettingsManager {

    private static final String SETTINGS_FILE_NAME = "showcase.properties";
    private static final String KEY_LAUNCH_COUNT = "launch.count";
    private static final String KEY_ORIENTATION = "preferred.orientation";
    private static final String KEY_PATTERN_TYPE = "preferred.pattern.type";

    private final File homeDirectory;
    private int launchCount = 0;
    private int preferredOrientation = 0;
    private int preferredPatternType = 0;

    public ShowcaseSettingsManager(File homeDirectory) {
        this.homeDirectory = homeDirectory;
    }

    /**
     * Loads settings from home directory, incrementing launch counter.
     */
    public void load() {
        if (homeDirectory == null) {
            return;
        }

        if (!homeDirectory.exists()) {
            homeDirectory.mkdirs();
        }

        File file = new File(homeDirectory, SETTINGS_FILE_NAME);
        Properties props = new Properties();

        if (file.exists()) {
            FileInputStream fis = null;
            try {
                fis = new FileInputStream(file);
                props.load(fis);
            } catch (IOException e) {
                // Keep default values on read error
            } finally {
                if (fis != null) {
                    try {
                        fis.close();
                    } catch (IOException ignored) {
                    }
                }
            }
        }

        try {
            String countStr = props.getProperty(KEY_LAUNCH_COUNT);
            if (countStr != null) {
                this.launchCount = Integer.parseInt(countStr);
            }
        } catch (NumberFormatException ignored) {
        }

        try {
            String orientStr = props.getProperty(KEY_ORIENTATION);
            if (orientStr != null) {
                this.preferredOrientation = Integer.parseInt(orientStr);
            }
        } catch (NumberFormatException ignored) {
        }

        try {
            String patternStr = props.getProperty(KEY_PATTERN_TYPE);
            if (patternStr != null) {
                this.preferredPatternType = Integer.parseInt(patternStr);
            }
        } catch (NumberFormatException ignored) {
        }

        // Increment launch count on each load
        this.launchCount++;
        save();
    }

    /**
     * Persists current settings to disk.
     */
    public synchronized void save() {
        if (homeDirectory == null) {
            return;
        }

        if (!homeDirectory.exists()) {
            homeDirectory.mkdirs();
        }

        File file = new File(homeDirectory, SETTINGS_FILE_NAME);
        Properties props = new Properties();
        props.put(KEY_LAUNCH_COUNT, String.valueOf(launchCount));
        props.put(KEY_ORIENTATION, String.valueOf(preferredOrientation));
        props.put(KEY_PATTERN_TYPE, String.valueOf(preferredPatternType));

        FileOutputStream fos = null;
        try {
            fos = new FileOutputStream(file);
            props.save(fos, "Kindle C++ Showcase Settings");
        } catch (IOException e) {
            // Log or ignore on constrained filesystem
        } finally {
            if (fos != null) {
                try {
                    fos.close();
                } catch (IOException ignored) {
                }
            }
        }
    }

    public int getLaunchCount() {
        return launchCount;
    }

    public int getPreferredOrientation() {
        return preferredOrientation;
    }

    public void setPreferredOrientation(int preferredOrientation) {
        this.preferredOrientation = preferredOrientation;
    }

    public int getPreferredPatternType() {
        return preferredPatternType;
    }

    public void setPreferredPatternType(int preferredPatternType) {
        this.preferredPatternType = preferredPatternType;
    }
}
