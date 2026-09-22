package com.amazon.kindle.showcase;

/**
 * Data model representing hardware, memory, and runtime metrics
 * reported by the native C++ engine.
 * Compatible with Java 1.4 / CDC 1.1 / PBP 1.1 runtime.
 */
public class TelemetryData {

    private String cpu = "Probing...";
    private String hardware = "Detecting...";
    private String soc = "Detecting...";
    private String arch = "Unknown";
    private long totalRamBytes = 0;
    private long freeRamBytes = 0;
    private long availableRamBytes = 0;
    private long ipcLatencyMs = 0;
    private int patternWidth = 200;
    private int patternHeight = 120;
    private int patternType = 0;
    private byte[] patternPixels = null;

    public TelemetryData() {
    }

    public String getCpu() {
        return cpu;
    }

    public void setCpu(String cpu) {
        this.cpu = cpu;
    }

    public String getHardware() {
        return hardware;
    }

    public void setHardware(String hardware) {
        this.hardware = hardware;
    }

    public String getSoc() {
        return soc;
    }

    public void setSoc(String soc) {
        this.soc = soc;
    }

    public String getArch() {
        return arch;
    }

    public void setArch(String arch) {
        this.arch = arch;
    }

    public long getTotalRamBytes() {
        return totalRamBytes;
    }

    public void setTotalRamBytes(long totalRamBytes) {
        this.totalRamBytes = totalRamBytes;
    }

    public long getFreeRamBytes() {
        return freeRamBytes;
    }

    public void setFreeRamBytes(long freeRamBytes) {
        this.freeRamBytes = freeRamBytes;
    }

    public long getAvailableRamBytes() {
        return availableRamBytes;
    }

    public void setAvailableRamBytes(long availableRamBytes) {
        this.availableRamBytes = availableRamBytes;
    }

    public long getIpcLatencyMs() {
        return ipcLatencyMs;
    }

    public void setIpcLatencyMs(long ipcLatencyMs) {
        this.ipcLatencyMs = ipcLatencyMs;
    }

    public int getPatternWidth() {
        return patternWidth;
    }

    public void setPatternWidth(int patternWidth) {
        this.patternWidth = patternWidth;
    }

    public int getPatternHeight() {
        return patternHeight;
    }

    public void setPatternHeight(int patternHeight) {
        this.patternHeight = patternHeight;
    }

    public int getPatternType() {
        return patternType;
    }

    public void setPatternType(int patternType) {
        this.patternType = patternType;
    }

    public byte[] getPatternPixels() {
        return patternPixels;
    }

    public void setPatternPixels(byte[] patternPixels) {
        this.patternPixels = patternPixels;
    }

    /**
     * Parses simple JSON key-value pairs formatted as {"key":"val","num":123}.
     * CDC 1.1 / Java 1.4 compatible manual parser without third-party dependencies.
     */
    public void parseTelemetryJson(String json) {
        if (json == null) {
            return;
        }

        String cpuVal = extractJsonString(json, "cpu");
        if (cpuVal != null) {
            this.cpu = cpuVal;
        }

        String hwVal = extractJsonString(json, "hardware");
        if (hwVal != null) {
            this.hardware = hwVal;
        }

        String socVal = extractJsonString(json, "soc");
        if (socVal != null) {
            this.soc = socVal;
        }

        String archVal = extractJsonString(json, "arch");
        if (archVal != null) {
            this.arch = archVal;
        }

        long totalVal = extractJsonLong(json, "total_ram");
        if (totalVal > 0) {
            this.totalRamBytes = totalVal;
        }

        long freeVal = extractJsonLong(json, "free_ram");
        if (freeVal > 0) {
            this.freeRamBytes = freeVal;
        }

        long availVal = extractJsonLong(json, "available_ram");
        if (availVal > 0) {
            this.availableRamBytes = availVal;
        }
    }

    /**
     * Parses pattern response JSON: width, height, pattern_type, hex_pixels.
     */
    public void parsePatternJson(String json) {
        if (json == null) {
            return;
        }

        long w = extractJsonLong(json, "width");
        if (w > 0) {
            this.patternWidth = (int) w;
        }

        long h = extractJsonLong(json, "height");
        if (h > 0) {
            this.patternHeight = (int) h;
        }

        long ptype = extractJsonLong(json, "pattern_type");
        this.patternType = (int) ptype;

        String hex = extractJsonString(json, "hex_pixels");
        if (hex != null && hex.length() >= 2) {
            int len = hex.length() / 2;
            byte[] pix = new byte[len];
            for (int i = 0; i < len; i++) {
                int high = Character.digit(hex.charAt(i * 2), 16);
                int low = Character.digit(hex.charAt(i * 2 + 1), 16);
                if (high >= 0 && low >= 0) {
                    pix[i] = (byte) ((high << 4) | low);
                }
            }
            this.patternPixels = pix;
        }
    }

    private static String extractJsonString(String json, String key) {
        String pattern = "\"" + key + "\":\"";
        int idx = json.indexOf(pattern);
        if (idx == -1) {
            return null;
        }
        int start = idx + pattern.length();
        int end = json.indexOf("\"", start);
        if (end == -1) {
            return null;
        }
        return json.substring(start, end);
    }

    private static long extractJsonLong(String json, String key) {
        String pattern = "\"" + key + "\":";
        int idx = json.indexOf(pattern);
        if (idx == -1) {
            return -1;
        }
        int start = idx + pattern.length();
        int end = start;
        while (end < json.length()) {
            char c = json.charAt(end);
            if (c >= '0' && c <= '9') {
                end++;
            } else {
                break;
            }
        }
        if (end > start) {
            try {
                return Long.parseLong(json.substring(start, end));
            } catch (NumberFormatException e) {
                return -1;
            }
        }
        return -1;
    }
}
