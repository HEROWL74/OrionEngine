cmake_minimum_required(VERSION 3.21)
project(DX12GameEngine LANGUAGES CXX)

# --------------------------------------------------
# VCPKG 統合
# --------------------------------------------------
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE AND EXISTS "${CMAKE_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake")
    set(CMAKE_TOOLCHAIN_FILE "${CMAKE_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
endif()

# ビルド設定
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(VCPKG_TARGET_TRIPLET "x64-windows")

# --------------------------------------------------
# インクルードパス
# --------------------------------------------------
include_directories(${CMAKE_SOURCE_DIR}/src)
include_directories(${CMAKE_SOURCE_DIR}/ThirdParty)
include_directories("C:/vcpkg/installed/x64-windows/include")

# --------------------------------------------------
# ソースコードを再帰的に収集
# --------------------------------------------------
file(GLOB_RECURSE ENGINE_SOURCES
    ${CMAKE_SOURCE_DIR}/src/*.cpp
    ${CMAKE_SOURCE_DIR}/src/*.hpp
    ${CMAKE_SOURCE_DIR}/src/*.h
)

# --------------------------------------------------
# 実行ファイルターゲット
# --------------------------------------------------
add_executable(DX12GameEngine ${ENGINE_SOURCES})

# WinMain を使うため有効化
set_target_properties(DX12GameEngine PROPERTIES WIN32_EXECUTABLE TRUE)

# --------------------------------------------------
# 依存ライブラリ
# --------------------------------------------------
target_link_libraries(DX12GameEngine PRIVATE
    "C:/vcpkg/installed/x64-windows/lib/lua.lib"
)

# --------------------------------------------------
# Windows 用コンパイラ設定
# --------------------------------------------------
if(MSVC)
    target_compile_options(DX12GameEngine PRIVATE /W4 /EHsc)
    add_definitions(-DUNICODE -D_UNICODE)
endif()

# --------------------------------------------------
# 🔹 Engine Assets コピー設定
# --------------------------------------------------
set(OUT_DIR "$<TARGET_FILE_DIR:DX12GameEngine>")
set(SRC_ASSETS_DIR         "${CMAKE_SOURCE_DIR}/assets")
set(SRC_ENGINE_ASSETS_DIR  "${CMAKE_SOURCE_DIR}/engine-assets")

add_custom_command(TARGET DX12GameEngine POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo "📦 Copying user assets → ${OUT_DIR}/assets"
    COMMAND ${CMAKE_COMMAND} -E rm -rf "${OUT_DIR}/assets"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${OUT_DIR}/assets"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${SRC_ASSETS_DIR}" "${OUT_DIR}/assets"

    COMMAND ${CMAKE_COMMAND} -E echo "📦 Copying engine-assets → ${OUT_DIR}/engine-assets"
    COMMAND ${CMAKE_COMMAND} -E rm -rf "${OUT_DIR}/engine-assets"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${OUT_DIR}/engine-assets"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${SRC_ENGINE_ASSETS_DIR}" "${OUT_DIR}/engine-assets"

    COMMENT "Copying assets and engine-assets to runtime output"
)

# --------------------------------------------------
# 🔹 HLSL シェーダ自動ビルド設定
# --------------------------------------------------
# HLSL ソースを全検索
file(GLOB_RECURSE HLSL_FILES ${CMAKE_SOURCE_DIR}/src/*.hlsl)

set(HLSL_OUTPUT_DIR "${CMAKE_BINARY_DIR}/$<CONFIG>/shaders")

foreach(HLSL ${HLSL_FILES})
    get_filename_component(FILE_NAME ${HLSL} NAME_WE)
    set(CSO_FILE "${HLSL_OUTPUT_DIR}/${FILE_NAME}.cso")

    # fxc.exe でコンパイル（DirectX 12 用）
    add_custom_command(
        OUTPUT ${CSO_FILE}
        COMMAND ${CMAKE_COMMAND} -E make_directory "${HLSL_OUTPUT_DIR}"
        COMMAND fxc.exe /nologo /T ps_5_0 /E main /Fo ${CSO_FILE} ${HLSL}
        DEPENDS ${HLSL}
        COMMENT "Compiling HLSL shader: ${FILE_NAME}.hlsl → ${CSO_FILE}"
    )

    list(APPEND COMPILED_SHADERS ${CSO_FILE})
endforeach()

add_custom_target(Shaders ALL DEPENDS ${COMPILED_SHADERS})
add_dependencies(DX12GameEngine Shaders)

# --------------------------------------------------
# ログ出力
# --------------------------------------------------
message(STATUS "===================================================")
message(STATUS "✅  CMake Configuration Completed for DX12GameEngine")
message(STATUS "🔹  Toolchain: ${CMAKE_TOOLCHAIN_FILE}")
message(STATUS "🔹  Triplet: ${VCPKG_TARGET_TRIPLET}")
message(STATUS "🔹  Linked: C:/vcpkg/installed/x64-windows/lib/lua.lib")
message(STATUS "🔹  Assets: ${ENGINE_ASSET_DIR}")
message(STATUS "🔹  Shaders: ${HLSL_OUTPUT_DIR}")
message(STATUS "===================================================")
