package com.amazon.kindle.emulator;

import com.amazon.kindle.kindlet.Kindlet;
import java.awt.BorderLayout;
import java.awt.Frame;
import java.awt.event.WindowAdapter;
import java.awt.event.WindowEvent;
import java.io.File;

public class KindleSimulatorWindow extends Frame {

    private final EinkScreenPanel screenPanel;
    private final KeypadPanel keypadPanel;
    private final SimulatorKindletContext context;
    private Kindlet activeKindlet;

    public KindleSimulatorWindow(String title, int width, int height) {
        super(title);
        setLayout(new BorderLayout());

        this.screenPanel = new EinkScreenPanel(width, height);
        this.keypadPanel = new KeypadPanel();
        this.context = new SimulatorKindletContext(new File(System.getProperty("java.io.tmpdir"), "kindle-sim"));

        add(screenPanel, BorderLayout.CENTER);
        add(keypadPanel, BorderLayout.SOUTH);

        addWindowListener(new WindowAdapter() {
            public void windowClosing(WindowEvent e) {
                if (activeKindlet != null) {
                    activeKindlet.stop();
                    activeKindlet.destroy();
                }
                dispose();
                System.exit(0);
            }
        });

        pack();
    }

    public void launchKindlet(Kindlet kindlet) throws Exception {
        this.activeKindlet = kindlet;
        kindlet.create(context);
        kindlet.start();
        screenPanel.triggerRefresh(true);
    }
}
