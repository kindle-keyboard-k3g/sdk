package com.amazon.kindle.bridge;

import java.io.File;
import java.io.IOException;

/**
 * Standard launcher using {@link Runtime#exec(String[], String[], File)} available in Java CDC 1.1 / PBP 1.1.
 */
public class RuntimeProcessLauncher implements ProcessLauncher {

    /**
     * Executes the target native command under the Sun CVM runtime.
     *
     * @param command command line arguments
     * @param workingDirectory process working directory
     * @return launched Process object
     * @throws IOException if execution fails
     */
    public Process launch(String[] command, File workingDirectory) throws IOException {
        return Runtime.getRuntime().exec(command, null, workingDirectory);
    }
}
