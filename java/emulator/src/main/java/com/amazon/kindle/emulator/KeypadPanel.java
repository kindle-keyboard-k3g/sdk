package com.amazon.kindle.emulator;

import java.awt.Button;
import java.awt.GridLayout;
import java.awt.Panel;

public class KeypadPanel extends Panel {

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
