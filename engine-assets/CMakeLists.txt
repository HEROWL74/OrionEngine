# engine-assets/CMakeLists.txt

if(NOT WIN32)
    return()
endif()

set(SLANGC_EXECUTABLE "${CMAKE_SOURCE_DIR}/tools/slang/bin/slangc.exe")

if(NOT EXISTS "${SLANGC_EXECUTABLE}")
    message(FATAL_ERROR "slangc.exe not found at: ${SLANGC_EXECUTABLE}")
endif()
message(STATUS "Using Slang Compiler: ${SLANGC_EXECUTABLE}")

set(SHADER_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/shaders")
set(SHADER_OUT_DIR "${CMAKE_BINARY_DIR}/editor/engine-assets/shaders")

function(get_shader_stage SHADER_NAME OUT_STAGE)
    if(SHADER_NAME MATCHES "VS$" OR SHADER_NAME MATCHES "Vertex$")
        set(${OUT_STAGE} "vertex" PARENT_SCOPE)
    elseif(SHADER_NAME MATCHES "PS$" OR SHADER_NAME MATCHES "Pixel$")
        set(${OUT_STAGE} "fragment" PARENT_SCOPE)
    else()
        set(${OUT_STAGE} "" PARENT_SCOPE)
    endif()
endfunction()

file(GLOB SLANG_FILES "${SHADER_SRC_DIR}/*.slang")
set(CSO_OUTPUTS "")

foreach(SLANG_PATH ${SLANG_FILES})
    get_filename_component(SHADER_NAME ${SLANG_PATH} NAME_WE)
    get_shader_stage(${SHADER_NAME} SHADER_STAGE)

    if(NOT SHADER_STAGE)
        continue()
    endif()

    # シェーダーステージに基づいてプロファイルを厳密に指定
    if(SHADER_STAGE STREQUAL "vertex")
        set(SPECIFIC_PROFILE "vs_6_6")
    else()
        set(SPECIFIC_PROFILE "ps_6_6")
    endif()

    set(CSO_PATH "${SHADER_OUT_DIR}/${SHADER_NAME}.dxil")

    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(SLANG_OPT_FLAGS -g -O0)
    else()
        set(SLANG_OPT_FLAGS -O3)
    endif()

    add_custom_command(
            OUTPUT  "${CSO_PATH}"
            COMMAND "${CMAKE_COMMAND}" -E make_directory "${SHADER_OUT_DIR}"
            COMMAND "${SLANGC_EXECUTABLE}"
            "${SLANG_PATH}"
            ${SLANG_OPT_FLAGS}
            -target dxil
            -profile ${SPECIFIC_PROFILE}
            -entry main
            -matrix-layout-row-major
            -o "${CSO_PATH}"
            MAIN_DEPENDENCY "${SLANG_PATH}"
            VERBATIM
    )
    list(APPEND CSO_OUTPUTS "${CSO_PATH}")
endforeach()

add_custom_target(Shaders ALL DEPENDS ${CSO_OUTPUTS})