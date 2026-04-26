add_library(plasma_bridge_audio_control_probe_tool STATIC
    audio_control_probe_runner.cpp
)

add_executable(audio_control_probe
    main.cpp
)

target_include_directories(plasma_bridge_audio_control_probe_tool
    PUBLIC
        ${PROJECT_SOURCE_DIR}
)

target_link_libraries(plasma_bridge_audio_control_probe_tool
    PUBLIC
        plasma_bridge_control
        Qt6::Core
    PRIVATE
        plasma_bridge_audio_control_adapter
        KF6::PulseAudioQt
)

target_link_libraries(audio_control_probe
    PRIVATE
        plasma_bridge_audio_control_probe_tool
        plasma_bridge_audio_control_adapter
)
