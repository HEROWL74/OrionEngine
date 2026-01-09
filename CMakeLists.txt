cmake_minimum_required(VERSION 3.21)
project(OrionEngine LANGUAGES CXX RC)

# --------------------------------------------------
# VCPKG 統合
# --------------------------------------------------
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE AND EXISTS "${CMAKE_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake")
    set(CMAKE_TOOLCHAIN_FILE "${CMAKE_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(VCPKG_TARGET_TRIPLET "x64-windows")

# --------------------------------------------------
# 依存関係
# --------------------------------------------------
find_package(Lua REQUIRED)
find_package(sol2 CONFIG REQUIRED)
find_package(DirectXTex CONFIG REQUIRED)
find_package(lua CONFIG REQUIRED)

# --------------------------------------------------
# HLSL シェーダビルド（共通）
# --------------------------------------------------
file(GLOB_RECURSE HLSL_FILES ${CMAKE_SOURCE_DIR}/src/*.hlsl)
set(HLSL_OUTPUT_DIR "${CMAKE_BINARY_DIR}/$<CONFIG>/shaders")

foreach(HLSL ${HLSL_FILES})
    get_filename_component(FILE_NAME ${HLSL} NAME_WE)
    set(CSO_FILE "${HLSL_OUTPUT_DIR}/${FILE_NAME}.cso")
    add_custom_command(
        OUTPUT ${CSO_FILE}
        COMMAND ${CMAKE_COMMAND} -E make_directory "${HLSL_OUTPUT_DIR}"
        COMMAND fxc.exe /nologo /T ps_5_0 /E main /Fo ${CSO_FILE} ${HLSL}
        DEPENDS ${HLSL}
        COMMENT "Compiling shader: ${FILE_NAME}.hlsl"
    )
    list(APPEND COMPILED_SHADERS ${CSO_FILE})
endforeach()

add_custom_target(Shaders ALL DEPENDS ${COMPILED_SHADERS})

# --------------------------------------------------
# サブディレクトリ
# --------------------------------------------------
add_subdirectory(engine)
add_subdirectory(editor)
add_subdirectory(runtime)

# --------------------------------------------------
# 情報出力
# --------------------------------------------------
message(STATUS "===================================================")
message(STATUS "✅ OrionEngine Root Configuration Completed")
message(STATUS "🔹 Triplet: ${VCPKG_TARGET_TRIPLET}")
message(STATUS "🔹 Shaders: ${HLSL_OUTPUT_DIR}")
message(STATUS "===================================================")