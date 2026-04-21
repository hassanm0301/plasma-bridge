cmake_minimum_required(VERSION 3.25)

project(plasma_bridge VERSION 0.2.0 LANGUAGES CXX)

include(CTest)

set(PLASMA_BRIDGE_DEFAULT_HOST "127.0.0.1" CACHE STRING "Default host address used by plasma_bridge.")
set(PLASMA_BRIDGE_DEFAULT_HTTP_PORT 8080 CACHE STRING "Default HTTP port used by plasma_bridge.")
set(PLASMA_BRIDGE_DEFAULT_WS_PORT 8081 CACHE STRING "Default WebSocket port used by plasma_bridge.")
set(PLASMA_BRIDGE_AUDIO_WS_PATH "/ws/audio" CACHE STRING "WebSocket endpoint path for audio updates.")
set(PLASMA_BRIDGE_AUDIO_PROTOCOL_VERSION 2 CACHE STRING "Supported WebSocket protocol version.")
if(PLASMA_BRIDGE_AUDIO_PROTOCOL_VERSION LESS 2)
    set(PLASMA_BRIDGE_AUDIO_PROTOCOL_VERSION 2 CACHE STRING "Supported WebSocket protocol version." FORCE)
endif()
set(PLASMA_BRIDGE_DEFAULT_PROBE_STARTUP_TIMEOUT_MS 5000 CACHE STRING "Default startup timeout used by probe tools.")
set(PLASMA_BRIDGE_APP_ID "org.plasma-remote-toolbar.plasma_bridge" CACHE STRING "PulseAudioQt application id for plasma_bridge.")
set(PLASMA_BRIDGE_AUDIO_PROBE_APP_ID "org.plasma-remote-toolbar.audio_probe" CACHE STRING "PulseAudioQt application id for audio_probe.")
set(PLASMA_BRIDGE_AUDIO_CONTROL_PROBE_APP_ID
    "org.plasma-remote-toolbar.audio_control_probe"
    CACHE STRING
    "PulseAudioQt application id for audio_control_probe.")

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

find_package(Qt6 REQUIRED COMPONENTS Core Network WebSockets)
find_package(KF6PulseAudioQt REQUIRED)

if(BUILD_TESTING)
    find_package(Qt6 REQUIRED COMPONENTS Test)
endif()

add_subdirectory(src)
add_subdirectory(tools)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
