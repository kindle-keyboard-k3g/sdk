import shutil
from typing import Dict

def check_toolchains() -> Dict[str, bool]:
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
