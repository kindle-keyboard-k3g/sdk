#!/bin/bash
set -euo pipefail

echo "=== Verifying Modern C++ & ARM Cross Toolchain ==="
arm-linux-gnueabi-g++ --version
cmake --version
qemu-arm-static --version

echo "Testing C++17 compilation for ARMv6 softfp..."
cat << 'EOF' > /tmp/test_arm.cpp
#include <iostream>
#include <string_view>

int main() {
    constexpr std::string_view greeting = "Hello from Kindle C++17 on ARMv6!";
    std::cout << greeting << std::endl;
    return 0;
}
EOF

arm-linux-gnueabi-g++ -std=c++17 -march=armv6j -mtune=arm1136jf-s -mfloat-abi=softfp -static /tmp/test_arm.cpp -o /tmp/test_arm
echo "Running cross-compiled binary with QEMU ARM..."
qemu-arm-static /tmp/test_arm
echo "SUCCESS: Modern C++ static ARM execution validated under QEMU!"
rm -f /tmp/test_arm.cpp /tmp/test_arm
