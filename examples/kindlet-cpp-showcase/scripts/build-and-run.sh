#!/usr/bin/env bash
set -e

# ==============================================================================
# build-and-run.sh
# End-to-end automation for compiling, signing, packaging, and emulating
# the Kindlet C++ Showcase Application.
# ==============================================================================

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
APP_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
SDK_ROOT="$(cd "${APP_DIR}/../.." && pwd)"

echo "=== Building Kindlet C++ Showcase Application ==="
echo "App Directory: ${APP_DIR}"
echo "SDK Root:      ${SDK_ROOT}"

# Step 1: Compile Native C++ Daemon
echo "--- [1/5] Compiling Native C++ Daemon ---"
mkdir -p "${APP_DIR}/build"
cd "${APP_DIR}/build"
cmake ..
make showcase_daemon
echo "Native daemon compiled successfully."

# Step 2: Ensure SDK Core JARs exist
echo "--- [2/5] Compiling SDK Core JARs ---"
JAVA_8_HOME="/usr/lib/jvm/java-8-openjdk-amd64"
if [ -d "${JAVA_8_HOME}" ]; then
    export JAVA_HOME="${JAVA_8_HOME}"
    export PATH="${JAVA_HOME}/bin:${PATH}"
fi

cd "${SDK_ROOT}/java/kindlet-api"
ant jar
cd "${SDK_ROOT}/java/kindlet-bridge"
ant jar
cd "${SDK_ROOT}/java/emulator"
ant jar

# Step 3: Compile Java Kindlet & Assemble JAR
echo "--- [3/5] Building Kindlet JAR with Ant ---"
cd "${APP_DIR}"
ant jar

# Step 4: Triple Code Signing & Packaging .azw2
echo "--- [4/5] Triple-Signing and Packaging AZW2 Container ---"
KEYSTORE_PATH="${APP_DIR}/build/developer.keystore"
export PYTHONPATH="${SDK_ROOT}/python/src:${PYTHONPATH}"

python3 -m kindle_sdk.cli sign "${APP_DIR}/dist/kindlet-cpp-showcase.jar" --keystore "${KEYSTORE_PATH}" --password "password123"

# Produce official .azw2 package copy
cp "${APP_DIR}/dist/kindlet-cpp-showcase.jar" "${APP_DIR}/dist/kindlet-cpp-showcase.azw2"
echo "Packaged: ${APP_DIR}/dist/kindlet-cpp-showcase.azw2"

# Step 5: Emulate / Run
echo "--- [5/5] Emulating Showcase Application ---"
MODE="${1:-headless}"
if [ "${MODE}" = "--gui" ] || [ "${MODE}" = "gui" ]; then
    echo "Launching interactive Kindle Desktop Simulator..."
    python3 -m kindle_sdk.cli emulate "${APP_DIR}/dist/kindlet-cpp-showcase.azw2"
else
    echo "Running headless simulator smoke test..."
    python3 -m kindle_sdk.cli emulate "${APP_DIR}/dist/kindlet-cpp-showcase.azw2" --headless
fi

echo "=== Showcase build-and-run completed successfully! ==="
