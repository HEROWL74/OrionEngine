# editor/CMakeLists.txt
file(GLOB_RECURSE EDITOR_SOURCES
    ${CMAKE_SOURCE_DIR}/src/editor/*.cpp
    ${CMAKE_SOURCE_DIR}/src/editor/*.hpp
)

add_executable(OrionEditor WIN32
    ${EDITOR_SOURCES}
    ${CMAKE_SOURCE_DIR}/resources/app.rc
)

set_target_properties(OrionEditor PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY                "${CMAKE_BINARY_DIR}/editor"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG          "${CMAKE_BINARY_DIR}/editor"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE        "${CMAKE_BINARY_DIR}/editor"
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_BINARY_DIR}/editor"
    RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL     "${CMAKE_BINARY_DIR}/editor"
)

target_link_libraries(OrionEditor
    PRIVATE OrionEngine
    PRIVATE OrionBuildSystem
)

if(TARGET Shaders)
    add_dependencies(OrionEditor Shaders)
endif()

add_custom_command(
    TARGET OrionEditor POST_BUILD

    # player 用ディレクトリ
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:OrionEditor>/player"

    # engine-assets はエディタ実行ファイルの隣に置く（engineRoot = exe の場所）
    # shaders は engine-assets/CMakeLists.txt で直接ここへ出力されるためコピー不要
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/engine-assets/fonts"
        "$<TARGET_FILE_DIR:OrionEditor>/engine-assets/fonts"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/engine-assets/skybox"
        "$<TARGET_FILE_DIR:OrionEditor>/engine-assets/skybox"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/engine-assets/images"
        "$<TARGET_FILE_DIR:OrionEditor>/engine-assets/images"

    # project は --project 引数で指定する運用のためコピーしない

    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:OrionGame>"    "$<TARGET_FILE_DIR:OrionEditor>/player/OrionGame.exe"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:OrionRuntime>" "$<TARGET_FILE_DIR:OrionEditor>/OrionRuntime.dll"

    COMMENT "Bundling engine-assets and player runtime into editor directory"
)

# 開発中: VS デバッガから F5 で起動したとき --project を自動付与
set_target_properties(OrionEditor PROPERTIES
    VS_DEBUGGER_COMMAND_ARGUMENTS
    "--project \"${CMAKE_SOURCE_DIR}/project\""
)

if(MSVC)
    target_compile_options(OrionEditor PRIVATE /W4 /EHsc)
    target_compile_definitions(OrionEditor PRIVATE UNICODE _UNICODE)
endif()