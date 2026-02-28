# engine-assets/CMakeLists.txt

# ‚Ü‚¸ PATH ã‚É‚ ‚é fxc ‚ð’T‚·imsvc-dev-cmd ‚ª’Ê‚µ‚Ä‚­‚ê‚Ä‚¢‚é‚Í‚¸j
find_program(FXC_EXECUTABLE fxc)

if(NOT FXC_EXECUTABLE)
    message(FATAL_ERROR "fxc.exe not found. Please ensure Windows SDK is installed.")
endif()

message(STATUS "Found fxc: ${FXC_EXECUTABLE}")

set(SHADER_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/shaders")
set(SHADER_OUT_DIR "${CMAKE_BINARY_DIR}/editor/engine-assets/shaders")

function(get_shader_profile SHADER_NAME OUT_PROFILE)
    if    (SHADER_NAME MATCHES "VS$" OR SHADER_NAME MATCHES "Vertex$")
        set(${OUT_PROFILE} "vs_5_1" PARENT_SCOPE)
    elseif(SHADER_NAME MATCHES "PS$" OR SHADER_NAME MATCHES "Pixel$")
        set(${OUT_PROFILE} "ps_5_1" PARENT_SCOPE)
    elseif(SHADER_NAME MATCHES "GS$")
        set(${OUT_PROFILE} "gs_5_1" PARENT_SCOPE)
    elseif(SHADER_NAME MATCHES "CS$")
        set(${OUT_PROFILE} "cs_5_1" PARENT_SCOPE)
    elseif(SHADER_NAME MATCHES "HS$")
        set(${OUT_PROFILE} "hs_5_1" PARENT_SCOPE)
    elseif(SHADER_NAME MATCHES "DS$")
        set(${OUT_PROFILE} "ds_5_1" PARENT_SCOPE)
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
        message(WARNING "Cannot determine shader type (skipping): ${HLSL_PATH}")
        continue()
    endif()

    set(CSO_PATH "${SHADER_OUT_DIR}/${SHADER_NAME}.cso")

    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(FXC_OPT_FLAGS /Od /Zi)
    else()
        set(FXC_OPT_FLAGS /O3)
    endif()

    add_custom_command(
        OUTPUT  "${CSO_PATH}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${SHADER_OUT_DIR}"
        COMMAND "${FXC_EXECUTABLE}"
                ${FXC_OPT_FLAGS}
                /WX /nologo
                /T "${SHADER_PROFILE}"
                /E main
                /Fo "${CSO_PATH}"
                "${HLSL_PATH}"
        MAIN_DEPENDENCY "${HLSL_PATH}"
        COMMENT "fxc: ${SHADER_NAME}.hlsl -> ${SHADER_NAME}.cso [${SHADER_PROFILE}]"
        VERBATIM
    )

    list(APPEND CSO_OUTPUTS "${CSO_PATH}")
endforeach()

add_custom_target(Shaders ALL DEPENDS ${CSO_OUTPUTS})