package com.amazon.kindle.showcase;

import com.amazon.kindle.kindlet.AbstractKindlet;
import com.amazon.kindle.kindlet.KindletContext;
import com.amazon.kindle.kindlet.KindletExecutionException;
import com.amazon.kindle.kindlet.ui.KLabel;
import com.amazon.kindle.kindlet.ui.KMenu;
import com.amazon.kindle.kindlet.ui.KMenuItem;
import com.amazon.kindle.kindlet.ui.KProgressIndicator;

import com.amazon.kindle.bridge.NativeBridgeListener;
import com.amazon.kindle.bridge.NativeMessage;
import com.amazon.kindle.bridge.NativeProcessSupervisor;
import com.amazon.kindle.bridge.RuntimeProcessLauncher;

import java.awt.BorderLayout;
import java.awt.Container;
import java.awt.Panel;
import java.awt.event.KeyEvent;
import java.awt.event.KeyListener;
import java.io.File;
import java.io.IOException;

/**
 * Kindlet C++ Showcase Application.
 * Integrates an official Amazon Kindlet (Java CDC 1.1 / PBP 1.1) with an embedded
 * ARMv6 native C++ engine via framed bidirectional IPC streams.
 * Demonstrates:
 * - Kindlet canonical lifecycle (create, start, stop, destroy)
 * - Custom 16-level grayscale E-Ink AWT dashboard rendering
 * - E-Ink high contrast widgets (KLabel, KProgressIndicator, KMenu)
 * - Native process extraction, supervision, and command/response IPC
 * - Keyboard & D-Pad event handling (Up, Down, Left, Right, Enter/Select)
 * - Orientation toggling (portrait vs landscape)
 * - Isolated persistent storage I/O in context.getHomeDirectory()
 */
public class ShowcaseKindlet extends AbstractKindlet implements KeyListener, NativeBridgeListener {

    private KindletContext context;
    private ShowcaseSettingsManager settingsManager;
    private NativeProcessSupervisor supervisor;
    private File daemonBinaryFile;
    private ShowcaseDashboardCanvas canvas;
    private KProgressIndicator progressIndicator;
    private KLabel statusLabel;
    private TelemetryData telemetry;

    private int requestCounter = 1;
    private long commandStartTimeMs = 0;
    private int currentOrientation = 0;
    private int currentPatternIndex = 0;
    private boolean isSupervisorActive = false;

    public void create(KindletContext context) throws KindletExecutionException {
        super.create(context);
        this.context = context;
        this.telemetry = new TelemetryData();

        // 1. Initialize persistent storage manager
        File homeDir = context.getHomeDirectory();
        this.settingsManager = new ShowcaseSettingsManager(homeDir);
        this.settingsManager.load();
        this.currentOrientation = this.settingsManager.getPreferredOrientation();
        this.currentPatternIndex = this.settingsManager.getPreferredPatternType();

        // Apply saved orientation
        if (currentOrientation != 0) {
            context.setOrientation(currentOrientation);
        }

        // 2. Build UI Layout
        Container root = context.getRootContainer();
        root.setLayout(new BorderLayout());

        // Header label
        KLabel titleLabel = new KLabel("Kindle SDK Showcase | Launch #" + settingsManager.getLaunchCount());
        root.add(titleLabel, BorderLayout.NORTH);

        // Center Dashboard Canvas
        this.canvas = new ShowcaseDashboardCanvas();
        root.add(canvas, BorderLayout.CENTER);

        // Bottom panel with Progress Bar and Status Label
        Panel bottomPanel = new Panel(new BorderLayout());
        this.progressIndicator = new KProgressIndicator();
        this.progressIndicator.setProgress(10);
        this.statusLabel = new KLabel("Initializing Native C++ Engine...");
        bottomPanel.add(progressIndicator, BorderLayout.NORTH);
        bottomPanel.add(statusLabel, BorderLayout.SOUTH);
        root.add(bottomPanel, BorderLayout.SOUTH);

        // 3. Register Keypad & D-Pad listener
        root.addKeyListener(this);
        canvas.addKeyListener(this);

        // 4. Configure Application KMenu
        KMenu menu = new KMenu();
        menu.add(new KMenuItem("Query C++ Engine"));
        menu.add(new KMenuItem("Cycle 16-Level Pattern"));
        menu.add(new KMenuItem("Toggle Orientation"));
        menu.add(new KMenuItem("Save Settings"));

        // 5. Initialize Native Process Supervisor
        RuntimeProcessLauncher launcher = new RuntimeProcessLauncher();
        this.supervisor = new NativeProcessSupervisor(launcher, homeDir);

        // Extract native binary: check armv6 first, fallback to generic daemon
        try {
            try {
                this.daemonBinaryFile = supervisor.extractResource("/bin/armv6/showcase_daemon", "showcase_daemon");
            } catch (IOException e) {
                this.daemonBinaryFile = supervisor.extractResource("/bin/showcase_daemon", "showcase_daemon");
            }
        } catch (IOException e) {
            statusLabel.setText("Daemon extraction warning: " + e.getMessage());
        }
    }

    public void start() {
        super.start();
        progressIndicator.setProgress(30);
        statusLabel.setText("Starting Native Supervisor...");

        // Launch native daemon
        if (supervisor != null && daemonBinaryFile != null && daemonBinaryFile.exists()) {
            try {
                supervisor.start(daemonBinaryFile, this);
                isSupervisorActive = true;
                progressIndicator.setProgress(60);

                // Send initial ping to check engine responsiveness
                sendPing();

                // Request initial system hardware telemetry
                requestTelemetry();
            } catch (Exception e) {
                statusLabel.setText("Failed to start native process: " + e.getMessage());
                canvas.setStatusMessage("Native start error: " + e.getMessage());
            }
        } else {
            statusLabel.setText("Daemon binary not available");
        }
    }

    public void stop() {
        progressIndicator.setProgress(0);
        statusLabel.setText("Stopping Kindlet...");

        // Graceful shutdown of native daemon
        if (supervisor != null && isSupervisorActive) {
            supervisor.stop();
            isSupervisorActive = false;
        }

        // Persist preferences
        if (settingsManager != null) {
            settingsManager.setPreferredOrientation(currentOrientation);
            settingsManager.setPreferredPatternType(currentPatternIndex);
            settingsManager.save();
        }

        super.stop();
    }

    public void destroy() {
        if (supervisor != null && isSupervisorActive) {
            supervisor.stop();
            isSupervisorActive = false;
            supervisor = null;
        }
        super.destroy();
    }

    /**
     * Sends health-check PING frame to native daemon.
     */
    public void sendPing() {
        if (supervisor != null && isSupervisorActive) {
            commandStartTimeMs = System.currentTimeMillis();
            NativeMessage ping = new NativeMessage(NativeMessage.TYPE_PING, requestCounter++, null);
            try {
                supervisor.sendMessage(ping);
                canvas.setStatusMessage("Sent PING to C++ engine...");
            } catch (IOException e) {
                canvas.setStatusMessage("Ping error: " + e.getMessage());
            }
        }
    }

    /**
     * Requests SoC and memory telemetry via JSON command.
     */
    public void requestTelemetry() {
        if (supervisor != null && isSupervisorActive) {
            commandStartTimeMs = System.currentTimeMillis();
            byte[] payload = "get_telemetry".getBytes();
            NativeMessage cmd = new NativeMessage(NativeMessage.TYPE_COMMAND, requestCounter++, payload);
            try {
                supervisor.sendMessage(cmd);
                progressIndicator.setProgress(80);
                canvas.setStatusMessage("Requesting hardware telemetry...");
            } catch (IOException e) {
                canvas.setStatusMessage("Telemetry error: " + e.getMessage());
            }
        }
    }

    /**
     * Requests native engine to generate a 16-level grayscale test pattern.
     */
    public void requestPattern(int patternType) {
        if (supervisor != null && isSupervisorActive) {
            commandStartTimeMs = System.currentTimeMillis();
            String cmdStr = "generate_pattern pattern=" + patternType;
            NativeMessage cmd = new NativeMessage(NativeMessage.TYPE_COMMAND, requestCounter++, cmdStr.getBytes());
            try {
                supervisor.sendMessage(cmd);
                progressIndicator.setProgress(90);
                canvas.setStatusMessage("Requesting C++ 16-level pattern type " + patternType + "...");
            } catch (IOException e) {
                canvas.setStatusMessage("Pattern error: " + e.getMessage());
            }
        }
    }

    /**
     * Cycles screen orientation between portrait and landscape.
     */
    public void toggleOrientation() {
        currentOrientation = (currentOrientation == 0) ? 1 : 0;
        context.setOrientation(currentOrientation);
        settingsManager.setPreferredOrientation(currentOrientation);
        settingsManager.save();
        canvas.setStatusMessage("Orientation set to: " + (currentOrientation == 0 ? "Portrait (0)" : "Landscape (1)"));
    }

    // --- NativeBridgeListener Callbacks ---

    public void onMessageReceived(NativeMessage message) {
        long elapsed = System.currentTimeMillis() - commandStartTimeMs;
        telemetry.setIpcLatencyMs(elapsed);

        if (message.getType() == NativeMessage.TYPE_PONG) {
            statusLabel.setText("Received PONG (" + elapsed + " ms)");
            canvas.setStatusMessage("C++ Engine Alive! (Round-trip: " + elapsed + " ms)");
            progressIndicator.setProgress(100);
        } else if (message.getType() == NativeMessage.TYPE_RESPONSE) {
            String json = new String(message.getPayload());

            if (json.indexOf("\"cpu\":") != -1) {
                telemetry.parseTelemetryJson(json);
                canvas.updateTelemetry(telemetry);
                statusLabel.setText("Telemetry updated (" + elapsed + " ms)");
                canvas.setStatusMessage("Telemetry loaded from C++ /proc probe.");
                progressIndicator.setProgress(100);
            } else if (json.indexOf("\"hex_pixels\":") != -1) {
                telemetry.parsePatternJson(json);
                canvas.updateTelemetry(telemetry);
                statusLabel.setText("Pattern updated (" + elapsed + " ms)");
                canvas.setStatusMessage("C++ generated 16-level grayscale pattern.");
                progressIndicator.setProgress(100);
            } else {
                statusLabel.setText("Response: " + json);
            }
        }
    }

    public void onProcessTerminated(int exitCode) {
        isSupervisorActive = false;
        statusLabel.setText("Native process terminated (code: " + exitCode + ")");
        canvas.setStatusMessage("Native C++ engine exited with code " + exitCode);
        progressIndicator.setProgress(0);
    }

    // --- KeyListener Callbacks (D-Pad and Keyboard navigation) ---

    public void keyPressed(KeyEvent e) {
        int code = e.getKeyCode();

        if (code == KeyEvent.VK_UP) {
            canvas.selectPreviousAction();
        } else if (code == KeyEvent.VK_DOWN) {
            canvas.selectNextAction();
        } else if (code == KeyEvent.VK_ENTER || code == KeyEvent.VK_SPACE) {
            executeSelectedAction();
        } else if (code == KeyEvent.VK_PAGE_UP || code == KeyEvent.VK_PAGE_DOWN) {
            toggleOrientation();
        } else if (code == KeyEvent.VK_1) {
            requestTelemetry();
        } else if (code == KeyEvent.VK_2) {
            cyclePattern();
        } else if (code == KeyEvent.VK_3) {
            toggleOrientation();
        } else if (code == KeyEvent.VK_4) {
            saveSettings();
        }
    }

    private void executeSelectedAction() {
        int selected = canvas.getSelectedMenuIndex();
        if (selected == 0) {
            requestTelemetry();
        } else if (selected == 1) {
            cyclePattern();
        } else if (selected == 2) {
            toggleOrientation();
        } else if (selected == 3) {
            saveSettings();
        }
    }

    private void cyclePattern() {
        currentPatternIndex = (currentPatternIndex + 1) % 3;
        requestPattern(currentPatternIndex);
    }

    private void saveSettings() {
        settingsManager.setPreferredOrientation(currentOrientation);
        settingsManager.setPreferredPatternType(currentPatternIndex);
        settingsManager.save();
        canvas.setStatusMessage("Settings successfully saved to " + settingsManager.getLaunchCount() + " launches.");
    }

    public void keyReleased(KeyEvent e) {}
    public void keyTyped(KeyEvent e) {}
}
