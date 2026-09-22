package com.amazon.kindle.bridge;

import java.io.File;
import java.io.IOException;

/**
 * Process launcher abstraction compatible with Java CDC 1.1 / PBP 1.1 runtime environments.
 * Facilitates dependency injection for native process execution and QEMU-based testing.
 */
public interface ProcessLauncher {

    /**
     * Spawns an external native process in the given working directory.
     *
     * @param command array containing the binary path and arguments
     * @param workingDirectory working directory for the process
     * @return active Process instance
     * @throws IOException if process spawning fails
     */
    Process launch(String[] command, File workingDirectory) throws IOException;
}
