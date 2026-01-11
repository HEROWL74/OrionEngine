# ==============================================
# Orion Editor
# ==============================================

cmake_minimum_required(VERSION 3.21)

project(OrionEditor LANGUAGES CXX RC)

# ----------------------------------------------
# ソース収集
# ----------------------------------------------
file(GLOB_RECURSE EDITOR_SOURCES
    ${CMAKE_SOURCE_DIR}/src/editor/*.cpp
    ${CMAKE_SOURCE_DIR}/src/editor/*.hpp
)

# ----------------------------------------------
# 実行ファイル
# ----------------------------------------------
add_executable(OrionEditor WIN32
    ${EDITOR_SOURCES}
    ${CMAKE_SOURCE_DIR}/resources/app.rc
)

# ----------------------------------------------
# Engine に依存
# ----------------------------------------------
target_link_libraries(OrionEditor
    PRIVATE OrionEngine
)

# ----------------------------------------------
# シェーダ
# ----------------------------------------------
if (TARGET Shaders)
    add_dependencies(OrionEditor Shaders)
endif()

# ----------------------------------------------
# 出力ディレクトリ
# ----------------------------------------------
set(EDITOR_OUT_DIR $<TARGET_FILE_DIR:OrionEditor>)

# ----------------------------------------------
# assets / engine-assets コピー
# ----------------------------------------------
add_custom_command(
    TARGET OrionEditor POST_BUILD

    # Game assets
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/assets"
        "${EDITOR_OUT_DIR}/assets"

    # Engine assets（shader 含む）
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/engine-assets"
        "${EDITOR_OUT_DIR}/engine-assets"

    COMMENT "Copying Editor assets"
)

# ----------------------------------------------
# MSVC 設定
# ----------------------------------------------
if (MSVC)
    target_compile_options(OrionEditor PRIVATE /W4 /EHsc)
    target_compile_definitions(OrionEditor PRIVATE UNICODE _UNICODE)
endif()
