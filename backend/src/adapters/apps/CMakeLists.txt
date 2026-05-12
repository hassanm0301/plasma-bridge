add_library(plasma_bridge_app_adapter STATIC
    kde_app_controller.cpp
)

target_include_directories(plasma_bridge_app_adapter
    PUBLIC
        ${PROJECT_SOURCE_DIR}/src
)

target_link_libraries(plasma_bridge_app_adapter
    PUBLIC
        plasma_bridge_common
        plasma_bridge_control
        Qt6::Core
        KF6::KIOGui
        KF6::Service
)
