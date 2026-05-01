add_library(plasma_bridge_state STATIC
    ${PROJECT_SOURCE_DIR}/src/adapters/media/media_observer.h
    audio_state_store.cpp
    media_state_store.cpp
    window_state_store.cpp
)

target_include_directories(plasma_bridge_state
    PUBLIC
        ${PROJECT_SOURCE_DIR}/src
)

target_link_libraries(plasma_bridge_state
    PUBLIC
        plasma_bridge_common
        Qt6::Core
    PRIVATE
        plasma_bridge_audio_adapter
        plasma_bridge_window_adapter
)
