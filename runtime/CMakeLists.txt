# runtime/CMakeLists.txt

if(NOT WIN32)
    return()
endif()

if(NOT DEFINED RUNTIME_PROJECT_NAME)
    set(RUNTIME_PROJECT_NAME "OrionRuntime")
endif()

message(STATUS "Runtime DLL Name: ${RUNTIME_PROJECT_NAME}")

# アイコンパス（editor と共通）
set(ORION_ICON_PATH "${CMAKE_SOURCE_DIR}/engine-assets/images/OrionEngineIcon.ico")

# ---------------------------------------------------------
# Launcher (OrionGame.exe)
# ---------------------------------------------------------

add_executable(OrionGame WIN32
    ${CMAKE_CURRENT_SOURCE_DIR}/GameMain.cpp
    # app.rc を除去。アイコンは POST_BUILD で OrionResourceInjector が注入する。
)

target_link_libraries(OrionGame
    PRIVATE OrionEngine
    PRIVATE OrionRenderer
)

# OrionGame へのアイコン注入
# （editor の POST_BUILD で player/ にコピーした後にも再注入されるが、
#   単体実行時のために runtime 出力先にも適用しておく）
add_custom_command(
    TARGET OrionGame POST_BUILD
    COMMAND "$<TARGET_FILE:OrionResourceInjector>"
            "$<TARGET_FILE:OrionGame>"
            "${ORION_ICON_PATH}"
    COMMENT "Injecting icon into OrionGame.exe"
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
    PRIVATE OrionRenderer
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