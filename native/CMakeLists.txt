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
    src/pixmap.cpp
    src/dirty.cpp
    src/frame_differ.cpp
    src/refresh_policy.cpp
    src/ghosting.cpp
    src/bitmap_font.cpp
    src/keyboard_translator.cpp
    src/input_debouncer.cpp
    src/modifier_tracker.cpp
    src/shortcut_registry.cpp
    src/audio.cpp
    src/audio_loader.cpp
    platform/fake/fake_procfs.cpp
    platform/fake/fake_eink.cpp
    platform/fake/fake_network.cpp
    platform/fake/fake_input.cpp
    platform/linux/procfs.cpp
    platform/linux/framebuffer_linux.cpp
    platform/linux/evdev_linux.cpp
    platform/linux/oss_audio_linux.cpp
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

add_executable(test_keyboard_translator tests/test_keyboard_translator.cpp)
target_link_libraries(test_keyboard_translator kindle_native)
add_test(NAME test_keyboard_translator COMMAND test_keyboard_translator)

add_executable(test_network tests/test_network.cpp)
target_link_libraries(test_network kindle_native)
add_test(NAME test_network COMMAND test_network)

add_executable(test_pixmap tests/test_pixmap.cpp)
target_link_libraries(test_pixmap kindle_native)
add_test(NAME test_pixmap COMMAND test_pixmap)

add_executable(test_dirty tests/test_dirty.cpp)
target_link_libraries(test_dirty kindle_native)
add_test(NAME test_dirty COMMAND test_dirty)

add_executable(test_frame_differ tests/test_frame_differ.cpp)
target_link_libraries(test_frame_differ kindle_native)
add_test(NAME test_frame_differ COMMAND test_frame_differ)

add_executable(test_refresh_policy tests/test_refresh_policy.cpp)
target_link_libraries(test_refresh_policy kindle_native)
add_test(NAME test_refresh_policy COMMAND test_refresh_policy)

add_executable(test_ghosting tests/test_ghosting.cpp)
target_link_libraries(test_ghosting kindle_native)
add_test(NAME test_ghosting COMMAND test_ghosting)

add_executable(test_bitmap_font tests/test_bitmap_font.cpp)
target_link_libraries(test_bitmap_font kindle_native)
add_test(NAME test_bitmap_font COMMAND test_bitmap_font)

add_executable(test_audio tests/test_audio.cpp)
target_link_libraries(test_audio kindle_native)
add_test(NAME test_audio COMMAND test_audio)

add_executable(test_keyboard_manager tests/test_keyboard_manager.cpp)
target_link_libraries(test_keyboard_manager kindle_native)
add_test(NAME test_keyboard_manager COMMAND test_keyboard_manager)

add_executable(kindle_daemon src/kindle_daemon_main.cpp)
target_link_libraries(kindle_daemon kindle_native)

install(DIRECTORY include/kindle DESTINATION include)
install(TARGETS kindle_native ARCHIVE DESTINATION lib)
