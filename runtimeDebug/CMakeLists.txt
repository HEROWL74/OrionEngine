# runtimeDebug/CMakeLists.txt

file(GLOB_RECURSE RUNTIME_SOURCES
    ${CMAKE_SOURCE_DIR}/src/runtime/*.cpp
    ${CMAKE_SOURCE_DIR}/src/runtime/*.hpp
)

add_executable(OrionGame WIN32
    ${RUNTIME_SOURCES}
    ${CMAKE_SOURCE_DIR}/resources/app.rc
)

target_link_libraries(OrionGame
    PRIVATE
        OrionEngine
)

target_include_directories(OrionGame
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src
)

if (MSVC)
    target_compile_options(OrionGame PRIVATE /W4 /EHsc)
    target_compile_definitions(OrionGame PRIVATE UNICODE _UNICODE)
endif()
