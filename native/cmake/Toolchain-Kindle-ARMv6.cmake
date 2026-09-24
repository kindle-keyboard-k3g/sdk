# Kindle ARMv6 Toolchain configuration for Modern C++20
# Targets: Kindle Keyboard (K3 / i.MX353) and Kindle DX (i.MX31)

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR arm)

set(CMAKE_C_COMPILER arm-linux-gnueabi-gcc)
set(CMAKE_CXX_COMPILER arm-linux-gnueabi-g++)

# ARMv6 softfp flags matching Kindle K3 & DX Linux 2.6 kernels
set(KINDLE_ARCH_FLAGS "-march=armv6j -mtune=arm1136jf-s -mfpu=vfp -mfloat-abi=softfp")
set(CMAKE_C_FLAGS_INIT "${KINDLE_ARCH_FLAGS}")
set(CMAKE_CXX_FLAGS_INIT "${KINDLE_ARCH_FLAGS} -static-libgcc -static-libstdc++")

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
