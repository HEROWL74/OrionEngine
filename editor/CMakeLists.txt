# editor/CMakeLists.txt

find_package(imgui CONFIG REQUIRED)

if(WIN32)
    find_package(WinPixEvent CONFIG REQUIRED)
    find_package(directx-headers CONFIG REQUIRED)
    find_package(directx12-agility CONFIG REQUIRED)
endif()

if(WIN32)
    # -------------------------------------------------------------------------
    # Windows
    # -------------------------------------------------------------------------
    file(GLOB_RECURSE EDITOR_SOURCES
        ${CMAKE_SOURCE_DIR}/editor/*.cpp
        ${CMAKE_SOURCE_DIR}/editor/*.hpp
    )

    add_executable(OrionEditor WIN32
        ${EDITOR_SOURCES}
    )

    target_link_libraries(OrionEditor
        PRIVATE Microsoft::DirectX-Headers
        PRIVATE Microsoft::DirectX12-Agility
        PRIVATE OrionEngine
        PRIVATE OrionRenderer
        PRIVATE OrionBuildSystem
        PRIVATE imgui::imgui
        PRIVATE Microsoft::WinPixEventRuntime
    )

    if(TARGET Shaders)
        add_dependencies(OrionEditor Shaders)
    endif()

    # アイコンパス
    set(ORION_ICON_PATH "${CMAKE_SOURCE_DIR}/engine-assets/images/OrionEngineIcon.ico")

    add_custom_command(
        TARGET OrionEditor POST_BUILD

        # engine-assets / player バンドル（従来通り）
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

        # アイコン注入（OrionEditor.exe 本体）
        COMMAND "$<TARGET_FILE:OrionResourceInjector>"
                "$<TARGET_FILE:OrionEditor>"
                "${ORION_ICON_PATH}"

        # アイコン注入（player/OrionGame.exe にも適用）
        COMMAND "$<TARGET_FILE:OrionResourceInjector>"
                "$<TARGET_FILE_DIR:OrionEditor>/player/OrionGame.exe"
                "${ORION_ICON_PATH}"

        COMMENT "Bundling assets and injecting icons"
    )

    set_target_properties(OrionEditor PROPERTIES
        VS_DEBUGGER_COMMAND_ARGUMENTS "--project \"${CMAKE_SOURCE_DIR}/project\""
    )

    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(OrionEditor PRIVATE /W4 /EHsc /arch:SSE2)
    else()
        target_compile_options(OrionEditor PRIVATE /W4 /EHsc)
    endif()

    target_compile_definitions(OrionEditor PRIVATE UNICODE _UNICODE)

else()
    # -------------------------------------------------------------------------
    # Linux
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