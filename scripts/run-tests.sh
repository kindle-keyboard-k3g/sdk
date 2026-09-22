#!/bin/bash
set -euo pipefail

echo "============================================="
echo "       Kindle SDK Full Test Suite            "
echo "============================================="

echo "--- 1. Python Unit & Packaging Tests ---"
PYTHONPATH=python/src python3 -m unittest discover -s python/tests -v

echo "--- 2. Native C++ Unit Tests ---"
cmake -S native -B native/build
cmake --build native/build
ctest --test-dir native/build --output-on-failure

echo "--- 3. Docker Configuration Verification ---"
./tests/docker/test_docker_builds.sh

echo "--- 4. Security Verification ---"
./scripts/check-no-private-keys.sh

echo "============================================="
echo "       All Test Stages Passed Successfully!  "
echo "============================================="
