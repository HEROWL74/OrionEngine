# -------------------------------------------------------------------------
# editor/CMakeLists.txt
# -------------------------------------------------------------------------

find_package(imgui CONFIG REQUIRED)

if(WIN32)
    find_package(WinPixEvent CONFIG REQUIRED)
    find_package(directx-headers CONFIG REQUIRED)
    find_package(directx12-agility CONFIG REQUIRED)
endif()

# -------------------------------------------------------------------------
# Rust Build Settings
# -------------------------------------------------------------------------

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(RUST_BUILD_TYPE "debug")
    set(RUST_FLAG "")
else()
    set(RUST_BUILD_TYPE "release")
    set(RUST_FLAG "--release")
endif()

# OrionAssetsBuilder
add_custom_target(OrionAssetsBuilderTool
    COMMAND cargo build ${RUST_FLAG}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tools/OrionAssetsBuilder
    COMMENT "Building OrionAssetsBuilder (Rust)"
)

# OrionBuild
add_custom_target(OrionBuildTool
    COMMAND cargo build ${RUST_FLAG}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tools/OrionBuild
    COMMENT "Building OrionBuild (Rust)"
)

# 実行ファイルパス
set(ASSET_BUILDER_EXE
    "${CMAKE_SOURCE_DIR}/tools/OrionAssetsBuilder/target/${RUST_BUILD_TYPE}/OrionAssetsBuilder.exe"
)

set(ORION_BUILD_EXE
    "${CMAKE_SOURCE_DIR}/tools/OrionBuild/target/${RUST_BUILD_TYPE}/OrionBuild.exe"
)

# -------------------------------------------------------------------------
# Editor
# -------------------------------------------------------------------------

if(WIN32)

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

    # ---------------------------------------------------------------------
    # Dependencies
    # ---------------------------------------------------------------------

    if(TARGET Shaders)
        add_dependencies(OrionEditor Shaders)
    endif()

    if(TARGET OrionResourceInjector)
        add_dependencies(OrionEditor OrionResourceInjector)
    endif()

    # 🔥 Rustツール依存
    add_dependencies(OrionEditor
        OrionAssetsBuilderTool
        OrionBuildTool
    )

    # ---------------------------------------------------------------------
    # Asset Builder 実行
    # ---------------------------------------------------------------------

    set(ORION_ICON_PATH
        "${CMAKE_SOURCE_DIR}/engine-assets/images/OrionEngineIcon.ico"
    )

    add_custom_target(RunAssetBuilder ALL
        COMMAND ${ASSET_BUILDER_EXE}
         --src "${CMAKE_SOURCE_DIR}"
         --build-dir "${CMAKE_BINARY_DIR}"
         --dst "$<TARGET_FILE_DIR:OrionEditor>"
         --icon "${ORION_ICON_PATH}"
         --injector "$<TARGET_FILE:OrionResourceInjector>"
        COMMENT "OrionAssetsBuilder: Syncing assets and injecting icon..."
        VERBATIM
    )

    # 実行順保証
    add_dependencies(RunAssetBuilder OrionAssetsBuilderTool)
    add_dependencies(OrionEditor RunAssetBuilder)

    # ---------------------------------------------------------------------
    # Debug Settings
    # ---------------------------------------------------------------------

    set_target_properties(OrionEditor PROPERTIES
        VS_DEBUGGER_COMMAND_ARGUMENTS "--project \"${CMAKE_SOURCE_DIR}/project\""
    )

    # ---------------------------------------------------------------------
    # Compiler Settings
    # ---------------------------------------------------------------------

    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(OrionEditor PRIVATE /W4 /EHsc /arch:SSE2)
    else()
        target_compile_options(OrionEditor PRIVATE /W4 /EHsc)
    endif()

    target_compile_definitions(OrionEditor PRIVATE UNICODE _UNICODE)

else()

    add_executable(OrionEditor
        ${CMAKE_SOURCE_DIR}/editor/EditorMain.cpp
        ${CMAKE_SOURCE_DIR}/editor/LinuxApp.hpp
        ${CMAKE_SOURCE_DIR}/editor/LinuxApp.cpp
    )

    target_link_libraries(OrionEditor
        PRIVATE OrionEngine
    )

endif()

# -------------------------------------------------------------------------
# Output Directory
# -------------------------------------------------------------------------

set_target_properties(OrionEditor PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY                "${CMAKE_BINARY_DIR}/editor"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG          "${CMAKE_BINARY_DIR}/editor"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE        "${CMAKE_BINARY_DIR}/editor"
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_BINARY_DIR}/editor"
    RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL     "${CMAKE_BINARY_DIR}/editor"
)