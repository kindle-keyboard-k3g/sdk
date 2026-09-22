package com.amazon.kindle.kindlet.ui;

import java.awt.Dimension;
import java.awt.Font;
import java.awt.Graphics;

/**
 * Text label component styled for e-ink readability.
 */
public class KLabel extends KComponent {

    private String text;

    public KLabel() {
        this("");
    }

    public KLabel(String text) {
        this.text = (text != null) ? text : "";
    }

    public String getText() {
        return this.text;
    }

    public void setText(String text) {
        this.text = (text != null) ? text : "";
        repaint();
    }

    public void paint(Graphics g) {
        if (text != null && text.length() > 0) {
            g.drawString(text, 0, g.getFontMetrics().getAscent());
        }
    }

    public Dimension getPreferredSize() {
        Font font = getFont();
        if (font != null) {
            return new Dimension(text.length() * 8, 16);
        }
        return new Dimension(60, 20);
    }
}
