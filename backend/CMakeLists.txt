cmake_minimum_required(VERSION 3.25)

file(READ "${CMAKE_CURRENT_SOURCE_DIR}/../VERSION" PLASMA_REMOTE_TOOLBAR_VERSION)
string(STRIP "${PLASMA_REMOTE_TOOLBAR_VERSION}" PLASMA_REMOTE_TOOLBAR_VERSION)

project(plasma_bridge VERSION "${PLASMA_REMOTE_TOOLBAR_VERSION}" LANGUAGES CXX)

include(CTest)

get_filename_component(PLASMA_REMOTE_TOOLBAR_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)
set(PLASMA_BRIDGE_SPECS_DIR
    "${PLASMA_REMOTE_TOOLBAR_ROOT}/specs"
    CACHE PATH
    "Directory containing the checked-in OpenAPI and AsyncAPI specs.")

set(PLASMA_BRIDGE_DEFAULT_HOST "127.0.0.1" CACHE STRING "Default host address used by plasma_bridge.")
set(PLASMA_BRIDGE_DEFAULT_HTTP_PORT 8080 CACHE STRING "Default HTTP port used by plasma_bridge.")
set(PLASMA_BRIDGE_DEFAULT_WS_PORT 8081 CACHE STRING "Default WebSocket port used by plasma_bridge.")
set(PLASMA_BRIDGE_WS_PATH "/ws" CACHE STRING "WebSocket endpoint path for state updates.")
set(PLASMA_BRIDGE_WS_PROTOCOL_VERSION 2 CACHE STRING "Supported WebSocket protocol version.")
if(PLASMA_BRIDGE_WS_PROTOCOL_VERSION LESS 2)
    set(PLASMA_BRIDGE_WS_PROTOCOL_VERSION 2 CACHE STRING "Supported WebSocket protocol version." FORCE)
endif()
set(PLASMA_BRIDGE_DEFAULT_PROBE_STARTUP_TIMEOUT_MS 5000 CACHE STRING "Default startup timeout used by probe tools.")
set(PLASMA_BRIDGE_APP_ID "org.plasma-remote-toolbar.plasma_bridge" CACHE STRING "PulseAudioQt application id for plasma_bridge.")
set(PLASMA_BRIDGE_AUDIO_PROBE_APP_ID "org.plasma-remote-toolbar.audio_probe" CACHE STRING "PulseAudioQt application id for audio_probe.")
set(PLASMA_BRIDGE_AUDIO_CONTROL_PROBE_APP_ID
    "org.plasma-remote-toolbar.audio_control_probe"
    CACHE STRING
    "PulseAudioQt application id for audio_control_probe.")
set(PLASMA_BRIDGE_WINDOW_PROBE_SCRIPT_ID
    "org.plasma_remote_toolbar.window_probe"
    CACHE STRING
    "KWin script package id used by window_probe.")
set(PLASMA_BRIDGE_WINDOW_PROBE_HELPER_SERVICE
    "org.plasma_remote_toolbar.WindowProbe"
    CACHE STRING
    "DBus session service used by the window probe helper.")
set(PLASMA_BRIDGE_WINDOW_PROBE_HELPER_PATH
    "/org/plasma_remote_toolbar/WindowProbe"
    CACHE STRING
    "DBus object path used by the window probe helper.")
set(PLASMA_BRIDGE_WINDOW_PROBE_HELPER_INTERFACE
    "org.plasma_remote_toolbar.WindowProbe"
    CACHE STRING
    "DBus interface used by the window probe helper.")

set(PLASMA_BRIDGE_GENERATED_INCLUDE_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
file(MAKE_DIRECTORY "${PLASMA_BRIDGE_GENERATED_INCLUDE_DIR}")
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/src/common/plasma_bridge_build_config.h.in"
    "${PLASMA_BRIDGE_GENERATED_INCLUDE_DIR}/plasma_bridge_build_config.h"
    @ONLY
)

include_directories("${PLASMA_BRIDGE_GENERATED_INCLUDE_DIR}")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC OFF)
set(CMAKE_AUTORCC OFF)

find_package(Qt6 REQUIRED COMPONENTS Core DBus Network WebSockets)
find_package(KF6PulseAudioQt REQUIRED)

if(BUILD_TESTING)
    find_package(Qt6 REQUIRED COMPONENTS Test)
endif()

add_subdirectory(src)
add_subdirectory(tools)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
