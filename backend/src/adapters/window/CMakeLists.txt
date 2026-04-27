add_library(plasma_bridge_window_adapter STATIC
    kwin_script_window_backend.cpp
    kwin_script_window_backend.h
    window_observer.h
)

target_include_directories(plasma_bridge_window_adapter
    PUBLIC
        ${PROJECT_SOURCE_DIR}/src
)

target_link_libraries(plasma_bridge_window_adapter
    PUBLIC
        plasma_bridge_common
        plasma_bridge_control
        Qt6::Core
        Qt6::DBus
)
