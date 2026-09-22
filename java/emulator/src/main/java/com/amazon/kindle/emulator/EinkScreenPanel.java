package com.amazon.kindle.emulator;

import java.awt.Color;
import java.awt.Dimension;
import java.awt.Graphics;
import java.awt.Panel;
import java.awt.image.BufferedImage;

public class EinkScreenPanel extends Panel {

    private final int screenWidth;
    private final int screenHeight;
    private BufferedImage buffer;
    private boolean flashing = false;

    public EinkScreenPanel(int width, int height) {
        this.screenWidth = width;
        this.screenHeight = height;
        this.buffer = new BufferedImage(width, height, BufferedImage.TYPE_INT_RGB);
        Graphics g = buffer.getGraphics();
        g.setColor(Color.WHITE);
        g.fillRect(0, 0, width, height);
        g.dispose();
    }

    public Dimension getPreferredSize() {
        return new Dimension(screenWidth, screenHeight);
    }

    public void triggerRefresh(boolean fullFlash) {
        if (fullFlash) {
            this.flashing = true;
            repaint();
            // Simulation flash
            this.flashing = false;
        }
        repaint();
    }

    public void paint(Graphics g) {
        if (flashing) {
            g.setColor(Color.BLACK);
            g.fillRect(0, 0, screenWidth, screenHeight);
        } else {
            g.drawImage(buffer, 0, 0, null);
        }
    }

    public BufferedImage getBuffer() {
        return buffer;
    }
}
