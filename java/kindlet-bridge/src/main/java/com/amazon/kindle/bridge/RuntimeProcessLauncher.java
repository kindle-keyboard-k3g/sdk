package com.amazon.kindle.bridge;

import java.io.File;
import java.io.IOException;

/**
 * Standard launcher using Runtime.getRuntime().exec() available in Java CDC 1.1 / PBP 1.1.
 */
public class RuntimeProcessLauncher implements ProcessLauncher {

    public Process launch(String[] command, File workingDirectory) throws IOException {
        return Runtime.getRuntime().exec(command, null, workingDirectory);
    }
}
