package com.amazon.kindle.emulator;

import java.awt.Color;
import java.awt.Dimension;
import java.awt.Graphics;
import java.awt.Panel;
import java.awt.image.BufferedImage;

/**
 * Display canvas simulating Kindle E-Ink screen rendering and refresh flashes.
 */
public class EinkScreenPanel extends Panel {

    private final int screenWidth;
    private final int screenHeight;
    private BufferedImage buffer;
    private boolean flashing = false;

    /**
     * Constructs a panel matching the specified Kindle display dimensions.
     *
     * @param width screen width in pixels (e.g. 600 for K3, 824 for DX)
     * @param height screen height in pixels (e.g. 800 for K3, 1200 for DX)
     */
    public EinkScreenPanel(int width, int height) {
        this.screenWidth = width;
        this.screenHeight = height;
        this.buffer = new BufferedImage(width, height, BufferedImage.TYPE_INT_RGB);
        Graphics g = buffer.getGraphics();
        g.setColor(Color.WHITE);
        g.fillRect(0, 0, width, height);
        g.dispose();
    }

    /**
     * Returns preferred dimensions matching screen resolution.
     *
     * @return Dimension instance
     */
    public Dimension getPreferredSize() {
        return new Dimension(screenWidth, screenHeight);
    }

    /**
     * Simulates an E-Ink waveform refresh, optionally triggering a full black/white inversion flash.
     *
     * @param fullFlash true to simulate full inversion flash, false for partial update
     */
    public void triggerRefresh(boolean fullFlash) {
        if (fullFlash) {
            this.flashing = true;
            repaint();
            // Simulation flash
            this.flashing = false;
        }
        repaint();
    }

    /**
     * Paints either the simulated flash state or the quantized screen buffer.
     *
     * @param g target graphics context
     */
    public void paint(Graphics g) {
        if (flashing) {
            g.setColor(Color.BLACK);
            g.fillRect(0, 0, screenWidth, screenHeight);
        } else {
            g.drawImage(buffer, 0, 0, null);
        }
    }

    /**
     * Returns the underlying image buffer.
     *
     * @return BufferedImage representation of the display
     */
    public BufferedImage getBuffer() {
        return buffer;
    }
}
