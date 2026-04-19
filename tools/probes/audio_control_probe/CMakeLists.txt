add_executable(audio_control_probe
    main.cpp
)

target_link_libraries(audio_control_probe
    PRIVATE
        plasma_bridge_audio_control_adapter
        plasma_bridge_control
        Qt6::Core
)
