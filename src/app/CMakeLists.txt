add_executable(plasma_bridge
    main.cpp
)

target_link_libraries(plasma_bridge
    PRIVATE
        plasma_bridge_api
        plasma_bridge_audio_adapter
        plasma_bridge_audio_control_adapter
        plasma_bridge_common
        plasma_bridge_state
        Qt6::Core
        Qt6::Network
        Qt6::WebSockets
)
