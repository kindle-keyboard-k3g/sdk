"""
System toolchain detection and environment validation.

Checks availability of compilers, cross-compilers, build tools, packaging utilities,
and emulation layers required for Kindle development.
"""

import shutil
from typing import Dict

def check_toolchains() -> Dict[str, bool]:
    """
    Scans the system PATH for required and optional Kindle SDK build tools.

    Returns:
        Dictionary mapping tool names to boolean availability flags:
        - docker: Container runtime for isolated builds
        - python3: Python interpreter
        - cmake: Build configuration system
        - g++: Host C++ compiler
        - arm-linux-gnueabi-g++: ARMv6 cross compiler
        - ant: Apache Ant build tool for Java
        - javac: Java compiler
        - java: Java runtime environment
        - jarsigner: JAR cryptographic signature tool
        - keytool: Java keystore certificate management tool
        - qemu-arm-static: ARM user-mode emulator
    """
    tools = [
        "docker",
        "python3",
        "cmake",
        "g++",
        "arm-linux-gnueabi-g++",
        "ant",
        "javac",
        "java",
        "jarsigner",
        "keytool",
        "qemu-arm-static"
    ]
    status = {}
    for t in tools:
        if t == "qemu-arm-static":
            status[t] = (shutil.which("qemu-arm-static") is not None) or (shutil.which("qemu-arm") is not None)
        else:
            status[t] = shutil.which(t) is not None
    return status
