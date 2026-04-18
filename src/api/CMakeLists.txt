add_library(plasma_bridge_api STATIC
    audio_websocket_server.cpp
    snapshot_http_server.cpp
)

target_include_directories(plasma_bridge_api
    PUBLIC
        ${PROJECT_SOURCE_DIR}/src
)

target_link_libraries(plasma_bridge_api
    PUBLIC
        plasma_bridge_common
        plasma_bridge_state
        Qt6::Core
        Qt6::Network
        Qt6::WebSockets
)
