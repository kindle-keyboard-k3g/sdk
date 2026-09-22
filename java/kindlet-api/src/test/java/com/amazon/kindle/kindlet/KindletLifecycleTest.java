package com.amazon.kindle.kindlet;

import java.awt.Container;
import java.io.File;

public class KindletLifecycleTest {

    static class MockKindletContext implements KindletContext {
        private final Container container = new Container();
        private final File home = new File("/tmp/mock-home");
        private int orientation = 0;

        public Container getRootContainer() { return container; }
        public File getHomeDirectory() { return home; }
        public int getOrientation() { return orientation; }
        public void setOrientation(int orientation) { this.orientation = orientation; }
    }

    static class TestApp extends AbstractKindlet {
        boolean created = false;
        boolean started = false;
        boolean stopped = false;
        boolean destroyed = false;

        public void create(KindletContext context) throws KindletExecutionException {
            super.create(context);
            this.created = true;
        }

        public void start() {
            super.start();
            this.started = true;
        }

        public void stop() {
            super.stop();
            this.stopped = true;
        }

        public void destroy() {
            super.destroy();
            this.destroyed = true;
        }
    }

    public static void main(String[] args) {
        System.out.println("Running KindletLifecycleTest...");
        TestApp app = new TestApp();
        MockKindletContext ctx = new MockKindletContext();

        try {
            app.create(ctx);
            if (!app.created || app.getContext() != ctx) {
                System.err.println("FAIL: create() lifecycle failed");
                System.exit(1);
            }

            app.start();
            if (!app.started) {
                System.err.println("FAIL: start() lifecycle failed");
                System.exit(1);
            }

            app.stop();
            if (!app.stopped) {
                System.err.println("FAIL: stop() lifecycle failed");
                System.exit(1);
            }

            app.destroy();
            if (!app.destroyed || app.getContext() != null) {
                System.err.println("FAIL: destroy() lifecycle failed");
                System.exit(1);
            }

            System.out.println("PASS: Kindlet lifecycle verified successfully!");
        } catch (KindletExecutionException e) {
            e.printStackTrace();
            System.exit(1);
        }
    }
}
