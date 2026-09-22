package com.amazon.kindle.kindlet.ui;

import java.awt.Color;
import java.awt.Dimension;
import java.awt.Graphics;

/**
 * Progress indicator optimized for e-ink displays.
 */
public class KProgressIndicator extends KComponent {

    private int progress = 0; // 0 to 100

    public KProgressIndicator() {
        super();
    }

    public int getProgress() {
        return this.progress;
    }

    public void setProgress(int progress) {
        if (progress < 0) progress = 0;
        if (progress > 100) progress = 100;
        this.progress = progress;
        repaint();
    }

    public void paint(Graphics g) {
        int width = getWidth();
        int height = getHeight();
        g.setColor(Color.BLACK);
        g.drawRect(0, 0, width - 1, height - 1);
        int fillWidth = (width - 4) * progress / 100;
        if (fillWidth > 0) {
            g.fillRect(2, 2, fillWidth, height - 4);
        }
    }

    public Dimension getPreferredSize() {
        return new Dimension(120, 16);
    }
}
