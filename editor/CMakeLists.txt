# ==============================================
# Orion Editor
# ==============================================

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

set_target_properties(OrionEditor PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY_DEBUG   "${CMAKE_BINARY_DIR}/editor"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/editor"
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_BINARY_DIR}/editor"
    RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${CMAKE_BINARY_DIR}/editor"
)

# ----------------------------------------------
# Engine に依存
# ----------------------------------------------
target_link_libraries(OrionEditor
    PRIVATE OrionEngine
    PRIVATE OrionBuildSystem
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
set(EDITOR_OUT_DIR "${CMAKE_BINARY_DIR}/editor")

# ----------------------------------------------
# engine-assets コピー
# ----------------------------------------------
add_custom_command(
    TARGET OrionEditor POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/engine-assets"
        "${EDITOR_OUT_DIR}/engine-assets"

    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/project-templates"
        "${EDITOR_OUT_DIR}/project-templates"

    COMMENT "Copying Editor assets"
)

# ----------------------------------------------
# MSVC 設定
# ----------------------------------------------
if (MSVC)
    target_compile_options(OrionEditor PRIVATE /W4 /EHsc)
    target_compile_definitions(OrionEditor PRIVATE UNICODE _UNICODE)
endif()