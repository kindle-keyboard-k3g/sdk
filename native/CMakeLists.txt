cmake_minimum_required(VERSION 3.16)
project(kindle_native CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

# Placeholder option: OpenSSL is linkable, but native HTTPS/TLS is not implemented yet.
option(KINDLE_NETWORK_WITH_OPENSSL "Stub placeholder for future TLS support; does not implement TLS yet" OFF)

add_library(kindle_native STATIC
    src/ipc.cpp
    src/jni_bridge.cpp
    src/network.cpp
    src/network_ipc.cpp
    platform/fake/fake_procfs.cpp
    platform/fake/fake_eink.cpp
    platform/fake/fake_network.cpp
    platform/linux/procfs.cpp
    platform/linux/framebuffer_linux.cpp
    platform/linux/evdev_linux.cpp
)

if(KINDLE_NETWORK_WITH_OPENSSL)
    find_package(OpenSSL REQUIRED)
    target_link_libraries(kindle_native OpenSSL::SSL OpenSSL::Crypto)
    target_compile_definitions(kindle_native PUBLIC KINDLE_NETWORK_WITH_OPENSSL)
endif()

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

add_executable(test_network tests/test_network.cpp)
target_link_libraries(test_network kindle_native)
add_test(NAME test_network COMMAND test_network)

add_executable(kindle_daemon src/kindle_daemon_main.cpp)
target_link_libraries(kindle_daemon kindle_native)
