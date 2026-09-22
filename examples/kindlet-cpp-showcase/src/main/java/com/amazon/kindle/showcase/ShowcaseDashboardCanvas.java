package com.amazon.kindle.showcase;

import com.amazon.kindle.kindlet.ui.KComponent;

import java.awt.Color;
import java.awt.Dimension;
import java.awt.Font;
import java.awt.Graphics;

/**
 * Custom 16-level grayscale E-Ink dashboard canvas.
 * Renders C++ computed pixel buffers, hardware telemetry, memory gauges,
 * and key navigation status.
 */
public class ShowcaseDashboardCanvas extends KComponent {

    private TelemetryData telemetry;
    private String statusMessage = "Press SELECT or ENTER to query native C++ daemon.";
    private int selectedMenuIndex = 0;
    private static final String[] MENU_ACTIONS = {
        "[1] Query C++ Hardware Telemetry",
        "[2] Cycle 16-Level E-Ink Pattern",
        "[3] Toggle Screen Orientation",
        "[4] Save Preferences to Storage"
    };

    public ShowcaseDashboardCanvas() {
        this.telemetry = new TelemetryData();
        setPreferredSize(new Dimension(560, 480));
    }

    public void updateTelemetry(TelemetryData data) {
        if (data != null) {
            this.telemetry = data;
            repaint();
        }
    }

    public void setStatusMessage(String msg) {
        this.statusMessage = (msg != null) ? msg : "";
        repaint();
    }

    public void selectNextAction() {
        selectedMenuIndex = (selectedMenuIndex + 1) % MENU_ACTIONS.length;
        repaint();
    }

    public void selectPreviousAction() {
        selectedMenuIndex = (selectedMenuIndex - 1 + MENU_ACTIONS.length) % MENU_ACTIONS.length;
        repaint();
    }

    public int getSelectedMenuIndex() {
        return selectedMenuIndex;
    }

    public void paint(Graphics g) {
        int width = getWidth();
        int height = getHeight();

        // 1. Clear background to clean white
        g.setColor(Color.WHITE);
        g.fillRect(0, 0, width, height);

        // 2. Draw outer border (2px black)
        g.setColor(Color.BLACK);
        g.drawRect(2, 2, width - 5, height - 5);
        g.drawRect(3, 3, width - 7, height - 7);

        // 3. Header title bar
        g.fillRect(4, 4, width - 8, 28);
        g.setColor(Color.WHITE);
        Font titleFont = new Font("SansSerif", Font.BOLD, 15);
        g.setFont(titleFont);
        g.drawString("Kindle Native C++ Engine Showcase", 14, 23);

        // 4. Draw Hardware Telemetry Box
        g.setColor(Color.BLACK);
        g.drawRect(10, 40, width - 20, 110);
        Font boldFont = new Font("SansSerif", Font.BOLD, 12);
        Font textFont = new Font("SansSerif", Font.PLAIN, 12);

        g.setFont(boldFont);
        g.drawString("HARDWARE & KERNEL TELEMETRY (via C++ /proc probe):", 18, 56);

        g.setFont(textFont);
        g.drawString("CPU / SoC: " + telemetry.getCpu() + " [" + telemetry.getSoc() + "]", 18, 74);
        g.drawString("Architecture: " + telemetry.getArch() + "  |  Platform: " + telemetry.getHardware(), 18, 92);

        long totalMb = telemetry.getTotalRamBytes() / (1024 * 1024);
        long freeMb = telemetry.getFreeRamBytes() / (1024 * 1024);
        long availMb = telemetry.getAvailableRamBytes() / (1024 * 1024);
        g.drawString("RAM: Total " + totalMb + " MB | Free " + freeMb + " MB | Avail " + availMb + " MB", 18, 110);
        g.drawString("IPC Round-Trip Latency: " + telemetry.getIpcLatencyMs() + " ms", 18, 128);

        // 5. Memory Gauge Bar
        int gaugeX = 18;
        int gaugeY = 134;
        int gaugeW = width - 40;
        int gaugeH = 10;
        g.drawRect(gaugeX, gaugeY, gaugeW, gaugeH);
        if (totalMb > 0) {
            long usedMb = totalMb - freeMb;
            int fillW = (int) ((usedMb * gaugeW) / totalMb);
            if (fillW > gaugeW) fillW = gaugeW;
            if (fillW > 0) {
                g.fillRect(gaugeX + 1, gaugeY + 1, fillW, gaugeH - 1);
            }
        }

        // 6. Draw Pattern Preview Box
        int patternBoxY = 160;
        int patternBoxH = 150;
        g.setColor(Color.BLACK);
        g.drawRect(10, patternBoxY, width - 20, patternBoxH);

        g.setFont(boldFont);
        g.drawString("16-LEVEL GRAYSCALE PATTERN (Computed by C++ Native Engine):", 18, patternBoxY + 18);

        // Render pattern pixels or placeholder
        byte[] pix = telemetry.getPatternPixels();
        int pw = telemetry.getPatternWidth();
        int ph = telemetry.getPatternHeight();

        int drawX = 18;
        int drawY = patternBoxY + 24;

        if (pix != null && pw > 0 && ph > 0) {
            // Draw pixel grid scaled by 1 or 2
            int scale = (width - 40) / pw;
            if (scale < 1) scale = 1;
            if (scale > 2) scale = 2;

            for (int y = 0; y < ph; y += 2) {
                for (int x = 0; x < pw; x += 2) {
                    int pidx = y * pw + x;
                    if (pidx < pix.length) {
                        int gray = pix[pidx] & 0xFF;
                        g.setColor(new Color(gray, gray, gray));
                        g.fillRect(drawX + (x / 2) * scale, drawY + (y / 2) * scale, scale, scale);
                    }
                }
            }
        } else {
            // Placeholder: draw 16 gray steps directly
            int stepW = (width - 50) / 16;
            for (int i = 0; i < 16; i++) {
                int gray = (i * 255) / 15;
                g.setColor(new Color(gray, gray, gray));
                g.fillRect(drawX + i * stepW, drawY + 10, stepW, 80);
                g.setColor(Color.BLACK);
                g.drawRect(drawX + i * stepW, drawY + 10, stepW, 80);
            }
        }

        // 7. Interactive Menu & Action Selector Box
        int menuBoxY = 320;
        int menuBoxH = 100;
        g.setColor(Color.BLACK);
        g.drawRect(10, menuBoxY, width - 20, menuBoxH);

        g.setFont(boldFont);
        g.drawString("D-PAD / KEYPAD ACTIONS (UP/DOWN to select, ENTER to execute):", 18, menuBoxY + 18);

        for (int i = 0; i < MENU_ACTIONS.length; i++) {
            int itemY = menuBoxY + 36 + (i * 15);
            if (i == selectedMenuIndex) {
                g.setColor(Color.BLACK);
                g.fillRect(18, itemY - 11, width - 36, 14);
                g.setColor(Color.WHITE);
                g.drawString("> " + MENU_ACTIONS[i] + " <", 22, itemY);
                g.setColor(Color.BLACK);
            } else {
                g.drawString("  " + MENU_ACTIONS[i], 22, itemY);
            }
        }

        // 8. Footer Status Line
        int footerY = 430;
        g.setColor(Color.BLACK);
        g.drawRect(10, footerY, width - 20, 36);
        g.setFont(boldFont);
        g.drawString("STATUS:", 18, footerY + 22);
        g.setFont(textFont);
        g.drawString(statusMessage, 80, footerY + 22);
    }
}
