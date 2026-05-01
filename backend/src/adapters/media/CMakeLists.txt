add_library(plasma_bridge_media_adapter STATIC
    mpris_media_controller.cpp
    mpris_media_observer.cpp
)

target_include_directories(plasma_bridge_media_adapter
    PUBLIC
        ${PROJECT_SOURCE_DIR}/src
)

target_link_libraries(plasma_bridge_media_adapter
    PUBLIC
        plasma_bridge_common
        plasma_bridge_control
        plasma_bridge_state
        Qt6::Core
        Qt6::DBus
)
