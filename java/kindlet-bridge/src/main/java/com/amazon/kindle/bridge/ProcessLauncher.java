package com.amazon.kindle.bridge;

import java.io.File;
import java.io.IOException;

/**
 * Process launcher abstraction compatible with Java CDC 1.1 / PBP 1.1.
 */
public interface ProcessLauncher {
    Process launch(String[] command, File workingDirectory) throws IOException;
}
