file(GLOB_RECURSE EDITOR_SOURCES
    ${CMAKE_SOURCE_DIR}/src/editor/*.cpp
)

add_executable(OrionEditor WIN32
    ${EDITOR_SOURCES}
    ${CMAKE_SOURCE_DIR}/resources/app.rc
)

target_link_libraries(OrionEditor PRIVATE OrionEngine)
add_dependencies(OrionEditor Shaders)

# --------------------------------------------------
# アセットコピー（Editor 用）
# --------------------------------------------------
set(OUT_DIR "$<TARGET_FILE_DIR:OrionEditor>")

add_custom_command(TARGET OrionEditor POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/assets" "${OUT_DIR}/assets"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/engine-assets" "${OUT_DIR}/engine-assets"
)
