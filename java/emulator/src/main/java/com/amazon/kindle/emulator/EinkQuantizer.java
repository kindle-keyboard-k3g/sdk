package com.amazon.kindle.emulator;

import java.awt.Color;
import java.awt.image.BufferedImage;

/**
 * Quantizer algorithm converting 24-bit RGB images to 16-level grayscale (4-bit per pixel)
 * matching Kindle E-Ink displays.
 */
public class EinkQuantizer {

    private static final int[] GRAYSCALE_16_LEVELS = new int[16];

    static {
        for (int i = 0; i < 16; i++) {
            GRAYSCALE_16_LEVELS[i] = i * 255 / 15;
        }
    }

    public static int quantizeRgbTo4bpp(int rgb) {
        int r = (rgb >> 16) & 0xFF;
        int g = (rgb >> 8) & 0xFF;
        int b = rgb & 0xFF;

        // Standard luminance formula
        int lum = (299 * r + 587 * g + 114 * b) / 1000;

        // Find closest 16-level value
        int level = (lum * 15 + 127) / 255;
        if (level < 0) level = 0;
        if (level > 15) level = 15;

        int gray = GRAYSCALE_16_LEVELS[level];
        return (0xFF << 24) | (gray << 16) | (gray << 8) | gray;
    }

    public static BufferedImage quantizeImage(BufferedImage src) {
        int width = src.getWidth();
        int height = src.getHeight();
        BufferedImage dst = new BufferedImage(width, height, BufferedImage.TYPE_INT_RGB);

        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                int rgb = src.getRGB(x, y);
                dst.setRGB(x, y, quantizeRgbTo4bpp(rgb));
            }
        }
        return dst;
    }
}
