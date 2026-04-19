add_library(plasma_bridge_control STATIC
    audio_volume_controller.cpp
)

target_include_directories(plasma_bridge_control
    PUBLIC
        ${PROJECT_SOURCE_DIR}/src
)

target_link_libraries(plasma_bridge_control
    PUBLIC
        Qt6::Core
)
