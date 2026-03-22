# engine-assets/CMakeLists.txt

# Linux ではシェーダービルド不要のためスキップ
if(NOT WIN32)
    return()
endif()

set(DXC_EXECUTABLE "${CMAKE_SOURCE_DIR}/tools/dxc/bin/x64/dxc.exe")

if(NOT EXISTS "${DXC_EXECUTABLE}")
    message(FATAL_ERROR "dxc.exe not found at: ${DXC_EXECUTABLE}")
endif()
message(STATUS "Using custom DXC: ${DXC_EXECUTABLE}")

set(SHADER_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/shaders")
set(SHADER_OUT_DIR "${CMAKE_BINARY_DIR}/editor/engine-assets/shaders")

function(get_shader_profile SHADER_NAME OUT_PROFILE)
    if(SHADER_NAME MATCHES "VS$" OR SHADER_NAME MATCHES "Vertex$")
        set(${OUT_PROFILE} "vs_6_6" PARENT_SCOPE)
    elseif(SHADER_NAME MATCHES "PS$" OR SHADER_NAME MATCHES "Pixel$")
        set(${OUT_PROFILE} "ps_6_6" PARENT_SCOPE)
    else()
        set(${OUT_PROFILE} "" PARENT_SCOPE)
    endif()
endfunction()

file(GLOB HLSL_FILES "${SHADER_SRC_DIR}/*.hlsl")
set(CSO_OUTPUTS "")

foreach(HLSL_PATH ${HLSL_FILES})
    get_filename_component(SHADER_NAME ${HLSL_PATH} NAME_WE)
    get_shader_profile(${SHADER_NAME} SHADER_PROFILE)

    if(NOT SHADER_PROFILE)
        continue()
    endif()

    set(CSO_PATH "${SHADER_OUT_DIR}/${SHADER_NAME}.cso")

    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(DXC_OPT_FLAGS -Od -Zi -Qembed_debug)
    else()
        set(DXC_OPT_FLAGS -O3)
    endif()

    add_custom_command(
        OUTPUT  "${CSO_PATH}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${SHADER_OUT_DIR}"
        COMMAND "${DXC_EXECUTABLE}"
                ${DXC_OPT_FLAGS}
                -HV 2021
                -T "${SHADER_PROFILE}"
                -E main
                -Fo "${CSO_PATH}"
                "${HLSL_PATH}"
        MAIN_DEPENDENCY "${HLSL_PATH}"
        COMMENT "dxc: ${SHADER_NAME}.hlsl -> ${SHADER_NAME}.cso [${SHADER_PROFILE}]"
        VERBATIM
    )
    list(APPEND CSO_OUTPUTS "${CSO_PATH}")
endforeach()

add_custom_target(Shaders ALL DEPENDS ${CSO_OUTPUTS})