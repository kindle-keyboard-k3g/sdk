cmake_minimum_required(VERSION 3.16)
project(kindle_native CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

add_library(kindle_native STATIC
    src/ipc.cpp
    src/jni_bridge.cpp
    platform/fake/fake_procfs.cpp
    platform/fake/fake_eink.cpp
    platform/linux/procfs.cpp
    platform/linux/framebuffer_linux.cpp
    platform/linux/evdev_linux.cpp
)

enable_testing()

add_executable(test_hardware tests/test_hardware.cpp)
target_link_libraries(test_hardware kindle_native)
add_test(NAME test_hardware COMMAND test_hardware)

add_executable(test_eink tests/test_eink.cpp)
target_link_libraries(test_eink kindle_native)
add_test(NAME test_eink COMMAND test_eink)

add_executable(test_ipc tests/test_ipc.cpp)
target_link_libraries(test_ipc kindle_native)
add_test(NAME test_ipc COMMAND test_ipc)

add_executable(test_input tests/test_input.cpp)
target_link_libraries(test_input kindle_native)
add_test(NAME test_input COMMAND test_input)
