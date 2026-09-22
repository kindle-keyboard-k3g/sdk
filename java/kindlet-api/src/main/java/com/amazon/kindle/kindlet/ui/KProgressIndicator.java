package com.amazon.kindle.kindlet.ui;

import java.awt.Color;
import java.awt.Dimension;
import java.awt.Graphics;

/**
 * Progress indicator optimized for high-contrast e-ink rendering.
 */
public class KProgressIndicator extends KComponent {

    private int progress = 0; // 0 to 100

    /**
     * Constructs a progress indicator initialized to 0%.
     */
    public KProgressIndicator() {
        super();
    }

    /**
     * Returns current progress percentage (0 to 100).
     *
     * @return current progress value
     */
    public int getProgress() {
        return this.progress;
    }

    /**
     * Updates progress value clamped between 0 and 100 and repaints.
     *
     * @param progress new percentage value
     */
    public void setProgress(int progress) {
        if (progress < 0) progress = 0;
        if (progress > 100) progress = 100;
        this.progress = progress;
        repaint();
    }

    /**
     * Renders high-contrast progress border and fill bar.
     *
     * @param g the Graphics context
     */
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

    /**
     * Returns default preferred dimensions for the progress bar.
     *
     * @return Dimension instance
     */
    public Dimension getPreferredSize() {
        return new Dimension(120, 16);
    }
}
