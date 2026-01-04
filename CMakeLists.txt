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
# ソースファイル収集
# --------------------------------------------------
file(GLOB_RECURSE ENGINE_SOURCES
    ${CMAKE_SOURCE_DIR}/src/*.cpp
    ${CMAKE_SOURCE_DIR}/src/*.hpp
    ${CMAKE_SOURCE_DIR}/src/*.h
)

set(ENGINE_RESOURCES
    ${CMAKE_SOURCE_DIR}/resources/app.rc
)

add_executable(OrionEngine WIN32
    ${ENGINE_SOURCES}
    ${ENGINE_RESOURCES}
)

# --------------------------------------------------
# インクルードパス
# --------------------------------------------------
include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/ThirdParty
    ${CMAKE_SOURCE_DIR}/resources
    "C:/vcpkg/installed/x64-windows/include"
)

# --------------------------------------------------
# リンク設定（Lua, DirectXなど）
# --------------------------------------------------

find_package(Lua REQUIRED)
find_package(sol2 CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(DirectXTex CONFIG REQUIRED)
find_package(lua CONFIG REQUIRED)

target_include_directories(OrionEngine PRIVATE
    ${LUA_INCLUDE_DIR}
)

target_link_libraries(OrionEngine PRIVATE
    ${LUA_LIBRARIES}
    d3d12
    dxgi
    dxguid
    sol2
    nlohmann_json::nlohmann_json
    Microsoft::DirectXTex
    lua
)

# --------------------------------------------------
# コンパイラ設定
# --------------------------------------------------
if(MSVC)
    target_compile_options(OrionEngine PRIVATE /W4 /EHsc)
    add_definitions(-DUNICODE -D_UNICODE)
endif()

# --------------------------------------------------
# アセットコピー（assets + engine-assets + resources）
# --------------------------------------------------
set(OUT_DIR "$<TARGET_FILE_DIR:OrionEngine>")
set(SRC_ASSETS_DIR         "${CMAKE_SOURCE_DIR}/assets")
set(SRC_ENGINE_ASSETS_DIR  "${CMAKE_SOURCE_DIR}/engine-assets")
set(SRC_RESOURCES_DIR  "${CMAKE_SOURCE_DIR}/resources")

add_custom_command(TARGET OrionEngine POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo "📦 Copying assets to ${OUT_DIR}"
    COMMAND ${CMAKE_COMMAND} -E rm -rf "${OUT_DIR}/assets"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${OUT_DIR}/assets"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${SRC_ASSETS_DIR}" "${OUT_DIR}/assets"

    COMMAND ${CMAKE_COMMAND} -E echo "📦 Copying assets to ${OUT_DIR}"
    COMMAND ${CMAKE_COMMAND} -E rm -rf "${OUT_DIR}/resources"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${OUT_DIR}/resources"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${SRC_ASSETS_DIR}" "${OUT_DIR}/resources"

    COMMAND ${CMAKE_COMMAND} -E echo "📦 Copying engine-assets to ${OUT_DIR}"
    COMMAND ${CMAKE_COMMAND} -E rm -rf "${OUT_DIR}/engine-assets"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${OUT_DIR}/engine-assets"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${SRC_ENGINE_ASSETS_DIR}" "${OUT_DIR}/engine-assets"

    COMMENT "Copying all runtime assets"
)

# --------------------------------------------------
# HLSLシェーダ自動ビルド
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
add_dependencies(OrionEngine Shaders)

# --------------------------------------------------
# 情報出力
# --------------------------------------------------
message(STATUS "===================================================")
message(STATUS "✅  OrionEngine Configuration Completed")
message(STATUS "🔹  Triplet: ${VCPKG_TARGET_TRIPLET}")
message(STATUS "🔹  Output Dir: ${OUT_DIR}")
message(STATUS "🔹  Shaders: ${HLSL_OUTPUT_DIR}")
message(STATUS "===================================================")