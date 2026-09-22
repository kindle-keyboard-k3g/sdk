package com.amazon.kindle.emulator;

import java.awt.Button;
import java.awt.GridLayout;
import java.awt.Panel;

/**
 * Visual navigation keypad simulating Kindle Keyboard and DX hardware navigation buttons.
 */
public class KeypadPanel extends Panel {

    /**
     * Constructs the 3x3 keypad layout featuring Page Turn, 5-way D-Pad, Select, Back, and Menu buttons.
     */
    public KeypadPanel() {
        setLayout(new GridLayout(3, 3, 4, 4));
        add(new Button("PgUp"));
        add(new Button("▲"));
        add(new Button("PgDn"));
        add(new Button("◄"));
        add(new Button("SELECT"));
        add(new Button("►"));
        add(new Button("BACK"));
        add(new Button("▼"));
        add(new Button("MENU"));
    }
}
