package com.amazon.kindle.kindlet.ui;

import java.awt.Dimension;
import java.awt.Font;
import java.awt.Graphics;

/**
 * Text label component styled for e-ink readability and high contrast.
 */
public class KLabel extends KComponent {

    private String text;

    /**
     * Constructs a label with an empty text string.
     */
    public KLabel() {
        this("");
    }

    /**
     * Constructs a label with the specified display text.
     *
     * @param text the initial label string
     */
    public KLabel(String text) {
        this.text = (text != null) ? text : "";
    }

    /**
     * Returns the current label string.
     *
     * @return the current text
     */
    public String getText() {
        return this.text;
    }

    /**
     * Sets the label string and requests a component repaint.
     *
     * @param text the new text to display
     */
    public void setText(String text) {
        this.text = (text != null) ? text : "";
        repaint();
    }

    /**
     * Renders the label text using the current Graphics context.
     *
     * @param g the Graphics context
     */
    public void paint(Graphics g) {
        if (text != null && text.length() > 0) {
            g.drawString(text, 0, g.getFontMetrics().getAscent());
        }
    }

    /**
     * Calculates preferred dimensions based on font metrics.
     *
     * @return preferred component Dimension
     */
    public Dimension getPreferredSize() {
        Font font = getFont();
        if (font != null) {
            return new Dimension(text.length() * 8, 16);
        }
        return new Dimension(60, 20);
    }
}
