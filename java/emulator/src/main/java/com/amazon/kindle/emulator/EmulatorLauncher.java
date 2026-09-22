package com.amazon.kindle.emulator;

import com.amazon.kindle.kindlet.Kindlet;
import java.io.File;
import java.net.URL;
import java.net.URLClassLoader;
import java.util.jar.JarFile;
import java.util.jar.Manifest;

/**
 * Headless and interactive runner for launching Kindlet applications inside the simulator.
 */
public class EmulatorLauncher {

    public static void main(String[] args) {
        if (args.length < 1) {
            System.err.println("Usage: java -cp ... com.amazon.kindle.emulator.EmulatorLauncher <path-to-azw2-or-jar> [--headless] [--fake-proxy] [--width W] [--height H]");
            System.exit(1);
        }

        String archivePath = args[0];
        boolean headless   = false;
        boolean fakeProxy  = false;
        int width  = 600;
        int height = 800;

        for (int i = 1; i < args.length; i++) {
            if ("--headless".equals(args[i])) {
                headless = true;
            } else if ("--fake-proxy".equals(args[i])) {
                fakeProxy = true;
            } else if ("--width".equals(args[i]) && i + 1 < args.length) {
                width = Integer.parseInt(args[++i]);
            } else if ("--height".equals(args[i]) && i + 1 < args.length) {
                height = Integer.parseInt(args[++i]);
            }
        }

        FakeWhispernetProxy proxy = null;
        try {
            if (fakeProxy) {
                proxy = new FakeWhispernetProxy();
                proxy.start();
            }

            File file = new File(archivePath);
            if (!file.exists()) {
                System.err.println("Error: File not found: " + archivePath);
                System.exit(2);
            }

            JarFile jarFile = new JarFile(file);
            Manifest manifest = jarFile.getManifest();
            if (manifest == null) {
                System.err.println("Error: Missing META-INF/MANIFEST.MF in archive");
                System.exit(3);
            }

            String mainClassName = manifest.getMainAttributes().getValue("Main-Class");
            if (mainClassName == null || mainClassName.trim().length() == 0) {
                System.err.println("Error: Missing Main-Class in MANIFEST.MF");
                System.exit(4);
            }

            URLClassLoader classLoader = new URLClassLoader(
                new URL[]{file.toURI().toURL()},
                EmulatorLauncher.class.getClassLoader()
            );

            Class kindletClass = classLoader.loadClass(mainClassName);
            Object instance = kindletClass.newInstance();
            if (!(instance instanceof Kindlet)) {
                System.err.println("Error: Main-Class does not implement com.amazon.kindle.kindlet.Kindlet");
                System.exit(5);
            }

            Kindlet kindlet = (Kindlet) instance;
            System.out.println("Simulator loading Kindlet: " + mainClassName + " (" + width + "x" + height + ")");

            if (headless) {
                File simHome = new File(System.getProperty("java.io.tmpdir"), "kindle-sim-headless");
                SimulatorKindletContext context = new SimulatorKindletContext(simHome);
                kindlet.create(context);
                kindlet.start();
                System.out.println("Simulator headless execution verified: Kindlet started successfully!");
                kindlet.stop();
                kindlet.destroy();
                System.out.println("Simulator headless execution verified: Kindlet stopped and destroyed cleanly!");
            } else {
                KindleSimulatorWindow window = new KindleSimulatorWindow("Kindle Simulator - " + mainClassName, width, height);
                window.setVisible(true);
                window.launchKindlet(kindlet);
            }

        } catch (Throwable t) {
            t.printStackTrace();
            System.exit(10);
        } finally {
            if (proxy != null) {
                proxy.stop();
            }
        }
    }
}
