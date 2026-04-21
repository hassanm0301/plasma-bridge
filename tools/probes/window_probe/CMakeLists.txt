add_library(plasma_bridge_window_probe_tool STATIC
    window_probe_runner.cpp
)

add_executable(window_probe
    main.cpp
)

target_include_directories(plasma_bridge_window_probe_tool
    PUBLIC
        ${PROJECT_SOURCE_DIR}
)

target_link_libraries(plasma_bridge_window_probe_tool
    PUBLIC
        plasma_bridge_common
        Qt6::Core
    PRIVATE
        Plasma::KWaylandClient
)

target_link_libraries(window_probe
    PRIVATE
        plasma_bridge_window_probe_tool
)
