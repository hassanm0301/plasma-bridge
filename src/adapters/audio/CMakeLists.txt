add_library(plasma_bridge_audio_adapter STATIC
    pulse_audio_state_observer.cpp
)

add_library(plasma_bridge_audio_control_adapter STATIC
    pulse_audio_device_controller.cpp
    pulse_audio_volume_controller.cpp
)

target_include_directories(plasma_bridge_audio_adapter
    PUBLIC
        ${PROJECT_SOURCE_DIR}/src
)

target_include_directories(plasma_bridge_audio_control_adapter
    PUBLIC
        ${PROJECT_SOURCE_DIR}/src
)

target_link_libraries(plasma_bridge_audio_adapter
    PUBLIC
        plasma_bridge_common
        Qt6::Core
        KF6::PulseAudioQt
)

target_link_libraries(plasma_bridge_audio_control_adapter
    PUBLIC
        plasma_bridge_control
        Qt6::Core
        KF6::PulseAudioQt
)
