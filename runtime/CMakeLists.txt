# runtime/CMakeLists.txt

file(GLOB_RECURSE RUNTIME_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/runtime/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/runtime/*.hpp
)

add_executable(OrionGame WIN32
    ${RUNTIME_SOURCES}
    ${CMAKE_CURRENT_SOURCE_DIR}/../resources/app.rc
)

# ★ Engine をリンクする（最重要）
target_link_libraries(OrionGame
    PRIVATE OrionEngine
)