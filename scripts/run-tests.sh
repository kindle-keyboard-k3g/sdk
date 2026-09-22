#!/bin/bash
set -euo pipefail

echo "============================================="
echo "       Kindle SDK Full Test Suite            "
echo "============================================="

echo "--- 1. Python Unit & Packaging Tests ---"
PYTHONPATH=python/src python3 -m unittest discover -s python/tests -v

echo "--- 2. Java Kindlet, Bridge & Simulator Ant Tests ---"
export JAVA_HOME=/usr/lib/jvm/java-8-openjdk-amd64
ant -f java/kindlet-api/build.xml clean test jar
ant -f java/kindlet-bridge/build.xml clean jar
ant -f java/emulator/build.xml clean test jar

echo "--- 3. Native C++ Unit Tests ---"
cmake -S native -B native/build
cmake --build native/build
ctest --test-dir native/build --output-on-failure

echo "--- 4. Docker Configuration Verification ---"
./tests/docker/test_docker_builds.sh

echo "--- 5. Security Verification ---"
./scripts/check-no-private-keys.sh

echo "============================================="
echo "       All Test Stages Passed Successfully!  "
echo "============================================="
