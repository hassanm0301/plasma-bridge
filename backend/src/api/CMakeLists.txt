set(DOCS_ASSETS_STAGE_DIR "${CMAKE_CURRENT_BINARY_DIR}/docs_assets")
set(GENERATED_DOCS_QRC "${CMAKE_CURRENT_BINARY_DIR}/docs_resources.qrc")
set(DOCS_ASSETS_STAMP "${DOCS_ASSETS_STAGE_DIR}/.prepared.stamp")
set(STAGED_OPENAPI_SPEC "${DOCS_ASSETS_STAGE_DIR}/specs/openapi.yaml")
set(STAGED_ASYNCAPI_SPEC "${DOCS_ASSETS_STAGE_DIR}/specs/asyncapi.yaml")

add_custom_command(
    OUTPUT
        "${GENERATED_DOCS_QRC}"
        "${DOCS_ASSETS_STAMP}"
    COMMAND
        "${CMAKE_COMMAND}"
        -DPROJECT_SOURCE_DIR=${PROJECT_SOURCE_DIR}
        -DSOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}
        -DSPECS_DIR=${PLASMA_BRIDGE_SPECS_DIR}
        -DDOCS_STAGE_DIR=${DOCS_ASSETS_STAGE_DIR}
        -DGENERATED_QRC=${GENERATED_DOCS_QRC}
        -DSTAMP_FILE=${DOCS_ASSETS_STAMP}
        -DSTAGED_OPENAPI_SPEC=${STAGED_OPENAPI_SPEC}
        -DSTAGED_ASYNCAPI_SPEC=${STAGED_ASYNCAPI_SPEC}
        -DPLASMA_BRIDGE_VERSION=${PROJECT_VERSION}
        -DPLASMA_BRIDGE_DEFAULT_HOST=${PLASMA_BRIDGE_DEFAULT_HOST}
        -DPLASMA_BRIDGE_DEFAULT_HTTP_PORT=${PLASMA_BRIDGE_DEFAULT_HTTP_PORT}
        -DPLASMA_BRIDGE_DEFAULT_WS_PORT=${PLASMA_BRIDGE_DEFAULT_WS_PORT}
        -DPLASMA_BRIDGE_WS_PATH=${PLASMA_BRIDGE_WS_PATH}
        -DPLASMA_BRIDGE_WS_PROTOCOL_VERSION=${PLASMA_BRIDGE_WS_PROTOCOL_VERSION}
        -P "${CMAKE_CURRENT_SOURCE_DIR}/prepare_docs_assets.cmake"
    DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/prepare_docs_assets.cmake"
        "${CMAKE_CURRENT_SOURCE_DIR}/docs_assets/index.html"
        "${CMAKE_CURRENT_SOURCE_DIR}/docs_assets/http.html"
        "${CMAKE_CURRENT_SOURCE_DIR}/docs_assets/ws.html"
        "${PLASMA_BRIDGE_SPECS_DIR}/openapi.yaml"
        "${PLASMA_BRIDGE_SPECS_DIR}/asyncapi.yaml"
    VERBATIM
)

set_source_files_properties("${GENERATED_DOCS_QRC}" PROPERTIES GENERATED TRUE)

add_library(plasma_bridge_api STATIC
    audio_control_http_helpers.cpp
    media_control_http_helpers.cpp
    window_control_http_helpers.cpp
    state_websocket_server.cpp
    json_envelope.cpp
    snapshot_http_server.cpp
    "${GENERATED_DOCS_QRC}"
)

set_target_properties(plasma_bridge_api PROPERTIES AUTORCC ON)

target_include_directories(plasma_bridge_api
    PUBLIC
        ${PROJECT_SOURCE_DIR}/src
)

target_link_libraries(plasma_bridge_api
    PUBLIC
        plasma_bridge_common
        plasma_bridge_control
        plasma_bridge_state
        Qt6::Core
        Qt6::Network
        Qt6::WebSockets
)
