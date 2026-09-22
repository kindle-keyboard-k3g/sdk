# Amazon Kindlet (J2ME CDC / PBP) Development Guide

This guide describes developing interactive active content (Kindlets) targeting the Sun CVM runtime on Kindle Keyboard (K3) and Kindle DX.

## Java Profile & Bytecode Specification

- **Java Specification:** Connected Device Configuration (CDC 1.1 / JSR 218) with Personal Basis Profile (PBP 1.1 / JSR 217) and Foundation Profile 1.1 (JSR 219).
- **Target Bytecode:** Java 1.4 bytecode (major version 48).
- **Compiler Options:** `javac -source 1.4 -target 1.4` (using JDK 8 with compliant source/target flags or ECJ compiler).

## Kindlet Lifecycle

The official Amazon Kindlet interface defines four lifecycle methods:

```java
package com.amazon.kindle.kindlet;

public interface Kindlet {
    void create(KindletContext context) throws KindletExecutionException;
    void start();
    void stop();
    void destroy();
}
```

### Lifecycle Rules:
1. `create(KindletContext context)`: Invoked when the Kindlet is instantiated. Initialize non-blocking UI components here. Must return quickly.
2. `start()`: Invoked when the Kindlet becomes active and visible on screen. Resume animation threads or listeners here. May be called multiple times if the user backgrounds and foregrounds the application.
3. `stop()`: Invoked when the Kindlet loses focus or goes to sleep. Suspend CPU-intensive tasks and timers.
4. `destroy()`: Invoked when the Kindlet is terminated. Clean up native processes, close files, and release resources.

## AWT User Interface & Graphics

- PBP 1.1 provides an AWT subset (`java.awt.Container`, `java.awt.Graphics`, `java.awt.Color`, `java.awt.Font`, etc.).
- Heavyweight Swing components (`javax.swing.*`) are **not** present in the CVM.
- Amazon supplies lightweight Kindle UI components under `com.amazon.kindle.kindlet.ui.*` (`KComponent`, `KLabel`, `KMenu`, `KProgressIndicator`).
- All custom rendering is performed via double-buffered lightweight `java.awt.Component` paints.

## Process Execution Constraints

- `java.lang.ProcessBuilder` was introduced in Java 5 and is **not** available in CDC 1.1.
- Use `Runtime.getRuntime().exec(String[])` for process launching within supervised native Kindlets.
