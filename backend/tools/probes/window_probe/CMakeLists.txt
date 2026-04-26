add_library(plasma_bridge_window_probe_tool STATIC
    window_probe_runner.cpp
)

add_executable(window_probe
    main.cpp
)

add_executable(window_probe_helper
    window_probe_helper.cpp
)

target_include_directories(plasma_bridge_window_probe_tool
    PUBLIC
        ${PROJECT_SOURCE_DIR}
)

target_link_libraries(plasma_bridge_window_probe_tool
    PUBLIC
        plasma_bridge_window_adapter
        plasma_bridge_common
        Qt6::Core
)

target_link_libraries(window_probe
    PRIVATE
        plasma_bridge_window_probe_tool
)
add_dependencies(window_probe window_probe_helper)

target_link_libraries(window_probe_helper
    PRIVATE
        plasma_bridge_common
        Qt6::Core
        Qt6::DBus
)

if(TARGET plasma_bridge)
    add_dependencies(plasma_bridge window_probe_helper)
    add_custom_command(TARGET window_probe_helper POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:window_probe_helper>
            $<TARGET_FILE_DIR:plasma_bridge>
    )
endif()
