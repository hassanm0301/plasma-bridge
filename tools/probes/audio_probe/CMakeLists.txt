add_library(plasma_bridge_audio_probe_tool STATIC
    audio_probe_runner.cpp
)

add_executable(audio_probe
    main.cpp
)

target_include_directories(plasma_bridge_audio_probe_tool
    PUBLIC
        ${PROJECT_SOURCE_DIR}
)

target_link_libraries(plasma_bridge_audio_probe_tool
    PUBLIC
        plasma_bridge_common
        Qt6::Core
    PRIVATE
        plasma_bridge_audio_adapter
)

target_link_libraries(audio_probe
    PRIVATE
        plasma_bridge_audio_probe_tool
        plasma_bridge_audio_adapter
)
