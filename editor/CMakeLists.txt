# -------------------------------------------------------------------------
# editor/CMakeLists.txt
# -------------------------------------------------------------------------

find_package(imgui CONFIG REQUIRED)

if(WIN32)
    find_package(WinPixEvent CONFIG REQUIRED)
    find_package(directx-headers CONFIG REQUIRED)
    find_package(directx12-agility CONFIG REQUIRED)
endif()

if(WIN32)
    # -------------------------------------------------------------------------
    # Windows Implementation
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

    # 依存関係を明示 (これが無いと POST_BUILD で Injector が見つからない)
    if(TARGET Shaders)
        add_dependencies(OrionEditor Shaders)
    endif()
    
    # ターゲットが存在する場合、強制的に先にビルドさせる
    if(TARGET OrionResourceInjector)
        add_dependencies(OrionEditor OrionResourceInjector)
    endif()

    # -------------------------------------------------------------------------
    # Build Tool Integration (OrionAssetsBuilder)
    # -------------------------------------------------------------------------
    
    set(ASSET_BUILDER_EXE "${CMAKE_SOURCE_DIR}/target/debug/OrionAssetsBuilder.exe")
    set(ORION_ICON_PATH "${CMAKE_SOURCE_DIR}/engine-assets/images/OrionEngineIcon.ico")
    add_custom_target(RunAssetBuilder ALL
        COMMAND ${ASSET_BUILDER_EXE}
                --src "${CMAKE_SOURCE_DIR}"
                --dst "$<TARGET_FILE_DIR:OrionEditor>"
                --icon "${ORION_ICON_PATH}"
                --injector "$<TARGET_FILE:OrionResourceInjector>"
        COMMENT "OrionAssetsBuilder: Syncing assets and injecting icon..."
        VERBATIM
    )

    # -------------------------------------------------------------------------
    # Compiler Settings
    # -------------------------------------------------------------------------
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
    # Linux Implementation
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

# -------------------------------------------------------------------------
# Common Output Directory Settings
# -------------------------------------------------------------------------
set_target_properties(OrionEditor PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY                "${CMAKE_BINARY_DIR}/editor"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG          "${CMAKE_BINARY_DIR}/editor"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE        "${CMAKE_BINARY_DIR}/editor"
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_BINARY_DIR}/editor"
    RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL     "${CMAKE_BINARY_DIR}/editor"
)