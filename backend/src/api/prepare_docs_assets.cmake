cmake_minimum_required(VERSION 3.25)

foreach(required_var
        PROJECT_SOURCE_DIR
        SOURCE_DIR
        SPECS_DIR
        DOCS_STAGE_DIR
        GENERATED_QRC
        STAMP_FILE)
    if(NOT DEFINED ${required_var} OR "${${required_var}}" STREQUAL "")
        message(FATAL_ERROR "Missing required variable: ${required_var}")
    endif()
endforeach()

if(NOT DEFINED STAGED_OPENAPI_SPEC OR "${STAGED_OPENAPI_SPEC}" STREQUAL "")
    message(FATAL_ERROR "Missing required variable: STAGED_OPENAPI_SPEC")
endif()

if(NOT DEFINED STAGED_ASYNCAPI_SPEC OR "${STAGED_ASYNCAPI_SPEC}" STREQUAL "")
    message(FATAL_ERROR "Missing required variable: STAGED_ASYNCAPI_SPEC")
endif()

if(NOT DEFINED PLASMA_BRIDGE_VERSION OR "${PLASMA_BRIDGE_VERSION}" STREQUAL "")
    message(FATAL_ERROR "Missing required variable: PLASMA_BRIDGE_VERSION")
endif()

foreach(required_var
        PLASMA_BRIDGE_DEFAULT_HOST
        PLASMA_BRIDGE_DEFAULT_HTTP_PORT
        PLASMA_BRIDGE_DEFAULT_WS_PORT
        PLASMA_BRIDGE_WS_PATH
        PLASMA_BRIDGE_WS_PROTOCOL_VERSION)
    if(NOT DEFINED ${required_var} OR "${${required_var}}" STREQUAL "")
        message(FATAL_ERROR "Missing required variable: ${required_var}")
    endif()
endforeach()

set(swagger_version "5.32.4")
set(asyncapi_version "3.1.0")

set(swagger_archive "${DOCS_STAGE_DIR}/cache/swagger-ui-dist-${swagger_version}.tgz")
set(asyncapi_archive "${DOCS_STAGE_DIR}/cache/asyncapi-react-component-${asyncapi_version}.tgz")

set(swagger_url "https://registry.npmjs.org/swagger-ui-dist/-/swagger-ui-dist-${swagger_version}.tgz")
set(asyncapi_url "https://registry.npmjs.org/%40asyncapi%2Freact-component/-/react-component-${asyncapi_version}.tgz")

set(swagger_extract_dir "${DOCS_STAGE_DIR}/extract/swagger-ui")
set(asyncapi_extract_dir "${DOCS_STAGE_DIR}/extract/asyncapi")

set(swagger_stage_dir "${DOCS_STAGE_DIR}/assets/swagger-ui")
set(asyncapi_stage_dir "${DOCS_STAGE_DIR}/assets/asyncapi")
set(notices_file "${DOCS_STAGE_DIR}/assets/THIRD_PARTY_NOTICES.txt")

function(download_if_missing url destination)
    if(EXISTS "${destination}")
        return()
    endif()

    get_filename_component(destination_dir "${destination}" DIRECTORY)
    file(MAKE_DIRECTORY "${destination_dir}")

    file(DOWNLOAD "${url}" "${destination}" STATUS status LOG log SHOW_PROGRESS TLS_VERIFY ON)
    list(GET status 0 status_code)
    list(GET status 1 status_message)

    if(NOT status_code EQUAL 0)
        file(REMOVE "${destination}")
        message(FATAL_ERROR
            "Failed to download ${url}\n"
            "Status: ${status_message}\n"
            "Log:\n${log}\n"
            "The browser assets are required for the hosted docs and were not found in the local build cache.")
    endif()
endfunction()

function(extract_archive archive destination)
    file(REMOVE_RECURSE "${destination}")
    file(MAKE_DIRECTORY "${destination}")

    execute_process(
        COMMAND "${CMAKE_COMMAND}" -E tar xzf "${archive}"
        WORKING_DIRECTORY "${destination}"
        RESULT_VARIABLE result
        ERROR_VARIABLE error_output
    )

    if(NOT result EQUAL 0)
        message(FATAL_ERROR "Failed to extract ${archive}\n${error_output}")
    endif()
endfunction()

function(copy_required source_file destination_file)
    if(NOT EXISTS "${source_file}")
        message(FATAL_ERROR "Expected file does not exist: ${source_file}")
    endif()

    get_filename_component(destination_dir "${destination_file}" DIRECTORY)
    file(MAKE_DIRECTORY "${destination_dir}")
    file(COPY_FILE "${source_file}" "${destination_file}" ONLY_IF_DIFFERENT)
endfunction()

function(stage_versioned_spec source_file destination_file)
    if(NOT EXISTS "${source_file}")
        message(FATAL_ERROR "Expected file does not exist: ${source_file}")
    endif()

    file(READ "${source_file}" spec_content)
    string(REPLACE "@PLASMA_BRIDGE_VERSION@" "${PLASMA_BRIDGE_VERSION}" spec_content "${spec_content}")
    string(REPLACE
        "@PLASMA_BRIDGE_DEFAULT_HTTP_SERVER_URL@"
        "http://${PLASMA_BRIDGE_DEFAULT_HOST}:${PLASMA_BRIDGE_DEFAULT_HTTP_PORT}"
        spec_content
        "${spec_content}")
    string(REPLACE
        "@PLASMA_BRIDGE_DEFAULT_WS_HOSTPORT@"
        "${PLASMA_BRIDGE_DEFAULT_HOST}:${PLASMA_BRIDGE_DEFAULT_WS_PORT}"
        spec_content
        "${spec_content}")
    string(REPLACE "@PLASMA_BRIDGE_WS_PATH@" "${PLASMA_BRIDGE_WS_PATH}" spec_content "${spec_content}")
    string(REPLACE
        "'@PLASMA_BRIDGE_WS_PROTOCOL_VERSION@'"
        "${PLASMA_BRIDGE_WS_PROTOCOL_VERSION}"
        spec_content
        "${spec_content}")
    string(REPLACE
        "@PLASMA_BRIDGE_WS_PROTOCOL_VERSION@"
        "${PLASMA_BRIDGE_WS_PROTOCOL_VERSION}"
        spec_content
        "${spec_content}")

    get_filename_component(destination_dir "${destination_file}" DIRECTORY)
    file(MAKE_DIRECTORY "${destination_dir}")
    file(WRITE "${destination_file}" "${spec_content}")
endfunction()

file(MAKE_DIRECTORY "${DOCS_STAGE_DIR}/cache")
file(MAKE_DIRECTORY "${DOCS_STAGE_DIR}/assets")

download_if_missing("${swagger_url}" "${swagger_archive}")
download_if_missing("${asyncapi_url}" "${asyncapi_archive}")

extract_archive("${swagger_archive}" "${swagger_extract_dir}")
extract_archive("${asyncapi_archive}" "${asyncapi_extract_dir}")

copy_required("${swagger_extract_dir}/package/swagger-ui.css" "${swagger_stage_dir}/swagger-ui.css")
copy_required("${swagger_extract_dir}/package/swagger-ui-bundle.js" "${swagger_stage_dir}/swagger-ui-bundle.js")
copy_required("${swagger_extract_dir}/package/swagger-ui-standalone-preset.js" "${swagger_stage_dir}/swagger-ui-standalone-preset.js")
copy_required("${swagger_extract_dir}/package/LICENSE" "${swagger_stage_dir}/LICENSE")
copy_required("${swagger_extract_dir}/package/NOTICE" "${swagger_stage_dir}/NOTICE")
copy_required("${swagger_extract_dir}/package/swagger-ui-bundle.js.LICENSE.txt" "${swagger_stage_dir}/swagger-ui-bundle.js.LICENSE.txt")
copy_required("${swagger_extract_dir}/package/swagger-ui-standalone-preset.js.LICENSE.txt" "${swagger_stage_dir}/swagger-ui-standalone-preset.js.LICENSE.txt")

copy_required("${asyncapi_extract_dir}/package/styles/default.min.css" "${asyncapi_stage_dir}/default.min.css")
copy_required("${asyncapi_extract_dir}/package/browser/standalone/index.js" "${asyncapi_stage_dir}/asyncapi-standalone.js")
copy_required("${asyncapi_extract_dir}/package/LICENSE" "${asyncapi_stage_dir}/LICENSE")
copy_required("${asyncapi_extract_dir}/package/browser/standalone/index.js.LICENSE.txt" "${asyncapi_stage_dir}/asyncapi-standalone.js.LICENSE.txt")

stage_versioned_spec("${SPECS_DIR}/openapi.yaml" "${STAGED_OPENAPI_SPEC}")
stage_versioned_spec("${SPECS_DIR}/asyncapi.yaml" "${STAGED_ASYNCAPI_SPEC}")

string(CONCAT notices_content
    "Hosted documentation assets bundled with plasma_bridge\n"
    "===============================================\n"
    "\n"
    "Swagger UI\n"
    "----------\n"
    "Package: swagger-ui-dist\n"
    "Version: ${swagger_version}\n"
    "Project: https://github.com/swagger-api/swagger-ui\n"
    "License: Apache-2.0\n"
    "Bundled files:\n"
    "- assets/swagger-ui/swagger-ui.css\n"
    "- assets/swagger-ui/swagger-ui-bundle.js\n"
    "- assets/swagger-ui/swagger-ui-standalone-preset.js\n"
    "- assets/swagger-ui/LICENSE\n"
    "- assets/swagger-ui/NOTICE\n"
    "- assets/swagger-ui/swagger-ui-bundle.js.LICENSE.txt\n"
    "- assets/swagger-ui/swagger-ui-standalone-preset.js.LICENSE.txt\n"
    "\n"
    "AsyncAPI React standalone bundle\n"
    "--------------------------------\n"
    "Package: @asyncapi/react-component\n"
    "Version: ${asyncapi_version}\n"
    "Project: https://github.com/asyncapi/asyncapi-react\n"
    "License: Apache-2.0\n"
    "Bundled files:\n"
    "- assets/asyncapi/default.min.css\n"
    "- assets/asyncapi/asyncapi-standalone.js\n"
    "- assets/asyncapi/LICENSE\n"
    "- assets/asyncapi/asyncapi-standalone.js.LICENSE.txt\n"
)

file(WRITE "${notices_file}" "${notices_content}")

set(qrc_content "<RCC>\n")
string(APPEND qrc_content "  <qresource prefix=\"/\">\n")
string(APPEND qrc_content "    <file alias=\"docs/index.html\">${SOURCE_DIR}/docs_assets/index.html</file>\n")
string(APPEND qrc_content "    <file alias=\"docs/http.html\">${SOURCE_DIR}/docs_assets/http.html</file>\n")
string(APPEND qrc_content "    <file alias=\"docs/ws.html\">${SOURCE_DIR}/docs_assets/ws.html</file>\n")
string(APPEND qrc_content "    <file alias=\"docs/specs/openapi.yaml\">${STAGED_OPENAPI_SPEC}</file>\n")
string(APPEND qrc_content "    <file alias=\"docs/specs/asyncapi.yaml\">${STAGED_ASYNCAPI_SPEC}</file>\n")
string(APPEND qrc_content "    <file alias=\"docs/assets/THIRD_PARTY_NOTICES.txt\">${notices_file}</file>\n")
string(APPEND qrc_content "    <file alias=\"docs/assets/swagger-ui/swagger-ui.css\">${swagger_stage_dir}/swagger-ui.css</file>\n")
string(APPEND qrc_content "    <file alias=\"docs/assets/swagger-ui/swagger-ui-bundle.js\">${swagger_stage_dir}/swagger-ui-bundle.js</file>\n")
string(APPEND qrc_content "    <file alias=\"docs/assets/swagger-ui/swagger-ui-standalone-preset.js\">${swagger_stage_dir}/swagger-ui-standalone-preset.js</file>\n")
string(APPEND qrc_content "    <file alias=\"docs/assets/swagger-ui/LICENSE\">${swagger_stage_dir}/LICENSE</file>\n")
string(APPEND qrc_content "    <file alias=\"docs/assets/swagger-ui/NOTICE\">${swagger_stage_dir}/NOTICE</file>\n")
string(APPEND qrc_content "    <file alias=\"docs/assets/swagger-ui/swagger-ui-bundle.js.LICENSE.txt\">${swagger_stage_dir}/swagger-ui-bundle.js.LICENSE.txt</file>\n")
string(APPEND qrc_content "    <file alias=\"docs/assets/swagger-ui/swagger-ui-standalone-preset.js.LICENSE.txt\">${swagger_stage_dir}/swagger-ui-standalone-preset.js.LICENSE.txt</file>\n")
string(APPEND qrc_content "    <file alias=\"docs/assets/asyncapi/default.min.css\">${asyncapi_stage_dir}/default.min.css</file>\n")
string(APPEND qrc_content "    <file alias=\"docs/assets/asyncapi/asyncapi-standalone.js\">${asyncapi_stage_dir}/asyncapi-standalone.js</file>\n")
string(APPEND qrc_content "    <file alias=\"docs/assets/asyncapi/LICENSE\">${asyncapi_stage_dir}/LICENSE</file>\n")
string(APPEND qrc_content "    <file alias=\"docs/assets/asyncapi/asyncapi-standalone.js.LICENSE.txt\">${asyncapi_stage_dir}/asyncapi-standalone.js.LICENSE.txt</file>\n")
string(APPEND qrc_content "  </qresource>\n")
string(APPEND qrc_content "</RCC>\n")

file(WRITE "${GENERATED_QRC}" "${qrc_content}")
file(WRITE "${STAMP_FILE}" "prepared\n")
