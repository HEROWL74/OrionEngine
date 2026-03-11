# editor/CMakeLists.txt

find_package(imgui CONFIG REQUIRED)
if(WIN32)
    # -------------------------------------------------------------------------
    # Windows: 既存の全ソースをそのまま使う（変更なし）
    # -------------------------------------------------------------------------
    file(GLOB_RECURSE EDITOR_SOURCES
        ${CMAKE_SOURCE_DIR}/editor/*.cpp
        ${CMAKE_SOURCE_DIR}/editor/*.hpp
    )

    add_executable(OrionEditor WIN32
        ${EDITOR_SOURCES}
        ${CMAKE_SOURCE_DIR}/resources/app.rc
    )

    target_link_libraries(OrionEditor
        PRIVATE OrionEngine
        PRIVATE OrionRenderer
        PRIVATE OrionBuildSystem
        PRIVATE imgui::imgui
    )

    if(TARGET Shaders)
        add_dependencies(OrionEditor Shaders)
    endif()

    add_custom_command(
        TARGET OrionEditor POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:OrionEditor>/player"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/engine-assets/fonts"
            "$<TARGET_FILE_DIR:OrionEditor>/engine-assets/fonts"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/engine-assets/skybox"
            "$<TARGET_FILE_DIR:OrionEditor>/engine-assets/skybox"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/engine-assets/images"
            "$<TARGET_FILE_DIR:OrionEditor>/engine-assets/images"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:OrionGame>"    "$<TARGET_FILE_DIR:OrionEditor>/player/OrionGame.exe"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:OrionRuntime>" "$<TARGET_FILE_DIR:OrionEditor>/OrionRuntime.dll"
        COMMENT "Bundling engine-assets and player runtime into editor directory"
    )

    set_target_properties(OrionEditor PROPERTIES
        VS_DEBUGGER_COMMAND_ARGUMENTS "--project \"${CMAKE_SOURCE_DIR}/project\""
    )

    target_compile_options(OrionEditor     PRIVATE /W4 /EHsc)
    target_compile_definitions(OrionEditor PRIVATE UNICODE _UNICODE)

else()
    # -------------------------------------------------------------------------
    # Linux: EditorMain / LinuxApp のみ
    #   Phase 2 以降: Vulkan描画・ImGui等をここに追加していく
    # -------------------------------------------------------------------------
    add_executable(OrionEditor
        ${CMAKE_SOURCE_DIR}/editor/EditorMain.cpp
        ${CMAKE_SOURCE_DIR}/editor/LinuxApp.hpp
        ${CMAKE_SOURCE_DIR}/editor/LinuxApp.cpp
    )

    target_link_libraries(OrionEditor
        PRIVATE OrionEngine
    )

endif()

set_target_properties(OrionEditor PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY                "${CMAKE_BINARY_DIR}/editor"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG          "${CMAKE_BINARY_DIR}/editor"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE        "${CMAKE_BINARY_DIR}/editor"
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_BINARY_DIR}/editor"
    RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL     "${CMAKE_BINARY_DIR}/editor"
)