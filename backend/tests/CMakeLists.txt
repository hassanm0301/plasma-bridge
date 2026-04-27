add_library(plasma_bridge_test_support STATIC
    support/test_support.cpp
)

target_include_directories(plasma_bridge_test_support
    PUBLIC
        ${PROJECT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(plasma_bridge_test_support
    PUBLIC
        plasma_bridge_api
        plasma_bridge_audio_control_probe_tool
        plasma_bridge_audio_probe_tool
        plasma_bridge_window_probe_tool
        plasma_bridge_common
        plasma_bridge_control
        Qt6::Core
        Qt6::Network
        Qt6::WebSockets
        Qt6::Test
)

add_executable(fake_audio_probe_cli
    helpers/fake_audio_probe_cli.cpp
)

add_executable(fake_window_probe_cli
    helpers/fake_window_probe_cli.cpp
)

target_include_directories(fake_audio_probe_cli
    PRIVATE
        ${PROJECT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(fake_audio_probe_cli
    PRIVATE
        plasma_bridge_audio_probe_tool
        plasma_bridge_test_support
        Qt6::Core
)

target_include_directories(fake_window_probe_cli
    PRIVATE
        ${PROJECT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(fake_window_probe_cli
    PRIVATE
        plasma_bridge_test_support
        plasma_bridge_window_probe_tool
        Qt6::Core
)

function(plasma_bridge_add_qt_test target)
    add_executable(${target} ${ARGN})
    target_include_directories(${target}
        PRIVATE
            ${PROJECT_SOURCE_DIR}
            ${CMAKE_CURRENT_SOURCE_DIR}
    )
    target_link_libraries(${target}
        PRIVATE
            plasma_bridge_test_support
            Qt6::Core
            Qt6::Network
            Qt6::WebSockets
            Qt6::Test
    )
    add_test(NAME ${target} COMMAND $<TARGET_FILE:${target}>)
endfunction()

plasma_bridge_add_qt_test(test_common
    unit/test_common.cpp
)
target_link_libraries(test_common PRIVATE plasma_bridge_common)

plasma_bridge_add_qt_test(test_window_state
    unit/test_window_state.cpp
)
target_link_libraries(test_window_state PRIVATE plasma_bridge_common)

plasma_bridge_add_qt_test(test_control
    unit/test_control.cpp
)
target_link_libraries(test_control PRIVATE plasma_bridge_control)

plasma_bridge_add_qt_test(test_audio_control_http_helpers
    unit/test_audio_control_http_helpers.cpp
)
target_link_libraries(test_audio_control_http_helpers PRIVATE plasma_bridge_api)

plasma_bridge_add_qt_test(test_window_control_http_helpers
    unit/test_window_control_http_helpers.cpp
)
target_link_libraries(test_window_control_http_helpers PRIVATE plasma_bridge_api)

plasma_bridge_add_qt_test(test_json_envelope
    unit/test_json_envelope.cpp
)
target_link_libraries(test_json_envelope PRIVATE plasma_bridge_api)

plasma_bridge_add_qt_test(test_state
    unit/test_state.cpp
)
target_link_libraries(test_state PRIVATE plasma_bridge_state)

plasma_bridge_add_qt_test(test_audio_probe_runner
    unit/test_audio_probe_runner.cpp
)
target_link_libraries(test_audio_probe_runner PRIVATE plasma_bridge_audio_probe_tool)

plasma_bridge_add_qt_test(test_audio_control_probe_runner
    unit/test_audio_control_probe_runner.cpp
)
target_link_libraries(test_audio_control_probe_runner PRIVATE plasma_bridge_audio_control_probe_tool)

plasma_bridge_add_qt_test(test_window_probe_runner
    unit/test_window_probe_runner.cpp
)
target_link_libraries(test_window_probe_runner PRIVATE plasma_bridge_window_probe_tool)

plasma_bridge_add_qt_test(test_snapshot_http_server
    feature/test_snapshot_http_server.cpp
)
target_link_libraries(test_snapshot_http_server
    PRIVATE
        plasma_bridge_api
        plasma_bridge_state
)

plasma_bridge_add_qt_test(test_state_websocket_server
    feature/test_state_websocket_server.cpp
)
target_link_libraries(test_state_websocket_server
    PRIVATE
        plasma_bridge_api
        plasma_bridge_state
)

plasma_bridge_add_qt_test(test_cli_binaries
    feature/test_cli_binaries.cpp
)
target_compile_definitions(test_cli_binaries
    PRIVATE
        TEST_PLASMA_BRIDGE_BIN="$<TARGET_FILE:plasma_bridge>"
        TEST_AUDIO_PROBE_BIN="$<TARGET_FILE:audio_probe>"
        TEST_AUDIO_CONTROL_PROBE_BIN="$<TARGET_FILE:audio_control_probe>"
        TEST_WINDOW_PROBE_BIN="$<TARGET_FILE:window_probe>"
        TEST_FAKE_AUDIO_PROBE_CLI_BIN="$<TARGET_FILE:fake_audio_probe_cli>"
        TEST_FAKE_WINDOW_PROBE_CLI_BIN="$<TARGET_FILE:fake_window_probe_cli>"
)
