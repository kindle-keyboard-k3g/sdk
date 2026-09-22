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
