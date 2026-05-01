add_library(plasma_bridge_media_probe_tool STATIC
    media_probe_runner.cpp
)

target_include_directories(plasma_bridge_media_probe_tool
    PUBLIC
        ${PROJECT_SOURCE_DIR}
)

target_link_libraries(plasma_bridge_media_probe_tool
    PUBLIC
        plasma_bridge_common
        plasma_bridge_control
        plasma_bridge_media_adapter
        plasma_bridge_state
        Qt6::Core
        Qt6::DBus
)

add_executable(media_probe
    main.cpp
)

target_include_directories(media_probe
    PRIVATE
        ${PROJECT_SOURCE_DIR}
)

target_link_libraries(media_probe
    PRIVATE
        plasma_bridge_media_probe_tool
        Qt6::Core
        Qt6::DBus
)
