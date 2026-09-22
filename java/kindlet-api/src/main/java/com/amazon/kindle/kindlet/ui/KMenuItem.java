package com.amazon.kindle.kindlet.ui;

import java.awt.event.ActionListener;

/**
 * Menu item abstraction for Kindle application menus.
 */
public class KMenuItem {

    private String label;
    private ActionListener actionListener;
    private boolean enabled = true;

    /**
     * Constructs a menu item with the specified display label.
     *
     * @param label the display text
     */
    public KMenuItem(String label) {
        this.label = (label != null) ? label : "";
    }

    /**
     * Returns the menu item label.
     *
     * @return the current label string
     */
    public String getLabel() {
        return this.label;
    }

    /**
     * Updates the menu item label.
     *
     * @param label the new label string
     */
    public void setLabel(String label) {
        this.label = (label != null) ? label : "";
    }

    /**
     * Checks if the menu item is currently enabled.
     *
     * @return true if enabled, false otherwise
     */
    public boolean isEnabled() {
        return this.enabled;
    }

    /**
     * Sets whether the menu item is enabled for interaction.
     *
     * @param enabled true to enable, false to disable
     */
    public void setEnabled(boolean enabled) {
        this.enabled = enabled;
    }

    /**
     * Attaches an action listener invoked when the menu item is triggered.
     *
     * @param l the ActionListener to register
     */
    public void addActionListener(ActionListener l) {
        this.actionListener = l;
    }

    /**
     * Returns the registered action listener.
     *
     * @return the active ActionListener or null
     */
    public ActionListener getActionListener() {
        return this.actionListener;
    }
}
