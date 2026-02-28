# editor/CMakeLists.txt

# =========================================================
# OrionEditor
# =========================================================
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

if (TARGET Shaders)
    add_dependencies(OrionEditor Shaders)
endif()

add_custom_command(
    TARGET OrionEditor POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/editor/player"
    
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/engine-assets/fonts"
        "${CMAKE_BINARY_DIR}/editor/engine-assets/fonts"
          COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/engine-assets/skybox"
        "${CMAKE_BINARY_DIR}/editor/engine-assets/skybox"
          COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/engine-assets/images"
        "${CMAKE_BINARY_DIR}/editor/engine-assets/images"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/project"
        "${CMAKE_BINARY_DIR}/editor/project"

    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:OrionGame>"
        "${CMAKE_BINARY_DIR}/editor/player/OrionGame.exe"

    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:OrionRuntime>"
        "${CMAKE_BINARY_DIR}/editor/OrionRuntime.dll"

    COMMENT "Bundling player runtime and assets into editor directory"
)

if (MSVC)
    target_compile_options(OrionEditor PRIVATE /W4 /EHsc)
    target_compile_definitions(OrionEditor PRIVATE UNICODE _UNICODE)
endif()
