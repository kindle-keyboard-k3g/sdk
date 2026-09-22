#!/bin/bash
set -euo pipefail

echo "=== Verifying Docker Build Files Existence and Executability ==="

for file in docker/j2me/Dockerfile docker/j2me/entrypoint.sh docker/j2me/verify-toolchain.sh \
            docker/native/Dockerfile docker/native/entrypoint.sh docker/native/verify-toolchain.sh \
            docker/compose.yaml; do
    if [ ! -f "$file" ]; then
        echo "FAIL: Missing file $file"
        exit 1
    fi
    echo "PASS: $file exists"
done

test -x docker/j2me/entrypoint.sh
test -x docker/j2me/verify-toolchain.sh
test -x docker/native/entrypoint.sh
test -x docker/native/verify-toolchain.sh

echo "All Docker configuration files are verified!"

echo "=== Building and Running Hermetic Docker Containers ==="
echo "1. Building and verifying kindle-j2me-builder container..."
docker build -f docker/j2me/Dockerfile -t kindle-j2me-builder .
docker run --rm kindle-j2me-builder /verify-toolchain.sh

echo "2. Building and verifying kindle-cpp-builder container..."
docker build -f docker/native/Dockerfile -t kindle-cpp-builder .
docker run --rm kindle-cpp-builder /verify-toolchain.sh

echo "=== All Docker Containers Successfully Built and Verified! ==="
