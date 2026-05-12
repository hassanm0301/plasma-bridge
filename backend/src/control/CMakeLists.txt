add_library(plasma_bridge_control STATIC
    app_controller.cpp
    audio_device_controller.cpp
    audio_volume_controller.cpp
    favorite_apps_store.cpp
    media_controller.cpp
    window_control_controller.cpp
)

target_include_directories(plasma_bridge_control
    PUBLIC
        ${PROJECT_SOURCE_DIR}/src
)

target_link_libraries(plasma_bridge_control
    PUBLIC
        Qt6::Core
)
