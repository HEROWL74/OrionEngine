if(NOT WIN32)
    return()
endif()

# ---------------------------------------------------------
# Runtime DLL 名
# ---------------------------------------------------------

if(NOT DEFINED RUNTIME_PROJECT_NAME)
    set(RUNTIME_PROJECT_NAME "OrionRuntime")
endif()

message(STATUS "Runtime DLL Name: ${RUNTIME_PROJECT_NAME}")

# ---------------------------------------------------------
# Launcher
# ---------------------------------------------------------

add_executable(OrionGame WIN32
    ${CMAKE_CURRENT_SOURCE_DIR}/GameMain.cpp
    ${CMAKE_SOURCE_DIR}/resources/app.rc
)

target_link_libraries(OrionGame
    PRIVATE OrionEngine
)

# ---------------------------------------------------------
# Runtime DLL
# ---------------------------------------------------------

file(GLOB_RECURSE RUNTIME_DLL_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp
)

list(FILTER RUNTIME_DLL_SOURCES EXCLUDE REGEX ".*GameMain\\.cpp$")

add_library(${RUNTIME_PROJECT_NAME} SHARED
    ${RUNTIME_DLL_SOURCES}
)

target_compile_definitions(${RUNTIME_PROJECT_NAME} PRIVATE
    ORION_RUNTIME_EXPORTS
    UNICODE
    _UNICODE
)

target_link_libraries(${RUNTIME_PROJECT_NAME}
    PRIVATE OrionEngine
)

# ---------------------------------------------------------
# 出力ディレクトリ
# ---------------------------------------------------------

set_target_properties(OrionGame PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/runtime"
)

set_target_properties(${RUNTIME_PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/runtime"
)
