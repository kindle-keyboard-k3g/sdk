package com.amazon.kindle.kindlet.ui;

import java.awt.event.ActionListener;

/**
 * Menu item abstraction for Kindle menus.
 */
public class KMenuItem {

    private String label;
    private ActionListener actionListener;
    private boolean enabled = true;

    public KMenuItem(String label) {
        this.label = (label != null) ? label : "";
    }

    public String getLabel() {
        return this.label;
    }

    public void setLabel(String label) {
        this.label = (label != null) ? label : "";
    }

    public boolean isEnabled() {
        return this.enabled;
    }

    public void setEnabled(boolean enabled) {
        this.enabled = enabled;
    }

    public void addActionListener(ActionListener l) {
        this.actionListener = l;
    }

    public ActionListener getActionListener() {
        return this.actionListener;
    }
}
