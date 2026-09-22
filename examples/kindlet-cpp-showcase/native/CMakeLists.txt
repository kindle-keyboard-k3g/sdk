cmake_minimum_required(VERSION 3.16)
project(showcase_daemon CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find root of the git repo / worktree
get_filename_component(KINDLE_SDK_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../.." ABSOLUTE)

include_directories(
    ${KINDLE_SDK_ROOT}/native/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

set(SHOWCASE_DAEMON_SRCS
    src/system_telemetry.cpp
    src/showcase_daemon.cpp
    ${KINDLE_SDK_ROOT}/native/src/ipc.cpp
)

add_executable(showcase_daemon ${SHOWCASE_DAEMON_SRCS})
