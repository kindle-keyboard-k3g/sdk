package com.amazon.kindle.emulator;

public class EinkQuantizerTest {

    public static void main(String[] args) {
        System.out.println("Running EinkQuantizerTest...");

        // White (255, 255, 255)
        int white = 0x00FFFFFF;
        int qWhite = EinkQuantizer.quantizeRgbTo4bpp(white) & 0xFF;
        if (qWhite != 255) {
            System.err.println("FAIL: Expected 255 for white, got " + qWhite);
            System.exit(1);
        }

        // Black (0, 0, 0)
        int black = 0x00000000;
        int qBlack = EinkQuantizer.quantizeRgbTo4bpp(black) & 0xFF;
        if (qBlack != 0) {
            System.err.println("FAIL: Expected 0 for black, got " + qBlack);
            System.exit(1);
        }

        // Middle Gray (128, 128, 128)
        int midGray = (128 << 16) | (128 << 8) | 128;
        int qMid = EinkQuantizer.quantizeRgbTo4bpp(midGray) & 0xFF;
        if (qMid < 110 || qMid > 140) {
            System.err.println("FAIL: Expected mid-level gray, got " + qMid);
            System.exit(1);
        }

        System.out.println("PASS: EinkQuantizerTest verified successfully!");
    }
}
