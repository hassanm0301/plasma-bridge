add_library(plasma_bridge_audio_adapter STATIC
    pulse_audio_sink_observer.cpp
)

target_include_directories(plasma_bridge_audio_adapter
    PUBLIC
        ${PROJECT_SOURCE_DIR}/src
)

target_link_libraries(plasma_bridge_audio_adapter
    PUBLIC
        plasma_bridge_common
        Qt6::Core
        KF6::PulseAudioQt
)
