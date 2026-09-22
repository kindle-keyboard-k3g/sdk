#!/bin/bash
set -euo pipefail

echo "=== Verifying J2ME / CDC Toolchain ==="
java -version
javac -version
ant -version
which jarsigner
which keytool

echo "Testing Java 1.4 target emission..."
cat << 'EOF' > /tmp/TestTarget.java
public class TestTarget {
    public static void main(String[] args) {
        System.out.println("Java 1.4 bytecode check");
    }
}
EOF

javac -source 1.4 -target 1.4 /tmp/TestTarget.java
MAJOR_VER=$(od -j 7 -N 1 -t u1 /tmp/TestTarget.class | head -n 1 | awk '{print $2}')
echo "Generated class major version: ${MAJOR_VER}"
if [ "${MAJOR_VER}" -ne 48 ]; then
    echo "ERROR: Expected major version 48 (Java 1.4), got ${MAJOR_VER}"
    exit 1
fi
echo "SUCCESS: Java 1.4 (major 48) bytecode confirmed!"
rm -f /tmp/TestTarget.*
