add_executable(audio_probe
    main.cpp
)

target_link_libraries(audio_probe
    PRIVATE
        plasma_bridge_audio_adapter
        plasma_bridge_common
        Qt6::Core
)
