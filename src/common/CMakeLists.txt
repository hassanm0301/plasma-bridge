add_library(plasma_bridge_common STATIC
    audio_state.cpp
)

target_include_directories(plasma_bridge_common
    PUBLIC
        ${PROJECT_SOURCE_DIR}/src
)

target_link_libraries(plasma_bridge_common
    PUBLIC
        Qt6::Core
)
