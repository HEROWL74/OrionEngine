# runtime/CMakeLists.txt

# -------------------------------------------------------
# RUNTIME_PROJECT_NAME : ゲームロジック DLL の名前
# デフォルトは "OrionRuntime"
# -------------------------------------------------------
if(NOT DEFINED RUNTIME_PROJECT_NAME)
    set(RUNTIME_PROJECT_NAME "OrionRuntime")
endif()

message(STATUS "Runtime DLL Name: ${RUNTIME_PROJECT_NAME}")

# -------------------------------------------------------
# ソース定義
# -------------------------------------------------------

# ランチャー EXE 用: GameMain.cpp のみ
set(LAUNCHER_SOURCES
    ${CMAKE_SOURCE_DIR}/src/runtime/GameMain.cpp
)

# DLL 用: GameMain.cpp 以外の全ファイル
file(GLOB_RECURSE RUNTIME_DLL_SOURCES
    ${CMAKE_SOURCE_DIR}/src/runtime/*.cpp
    ${CMAKE_SOURCE_DIR}/src/runtime/*.hpp
)
list(FILTER RUNTIME_DLL_SOURCES EXCLUDE REGEX ".*GameMain\\.cpp$")

# -------------------------------------------------------
# OrionGame.exe（固定ランチャー）
# -------------------------------------------------------
add_executable(OrionGame WIN32
    ${LAUNCHER_SOURCES}
    ${CMAKE_SOURCE_DIR}/resources/app.rc
)

set_target_properties(OrionGame PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY                "${CMAKE_BINARY_DIR}/runtime"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG          "${CMAKE_BINARY_DIR}/runtime"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE        "${CMAKE_BINARY_DIR}/runtime"
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_BINARY_DIR}/runtime"
    RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL     "${CMAKE_BINARY_DIR}/runtime"
)

# 開発中: VS デバッガから F5 で起動したとき --project を自動付与
set_target_properties(OrionGame PROPERTIES
    VS_DEBUGGER_COMMAND_ARGUMENTS
    "--project \"${CMAKE_SOURCE_DIR}/project\""
)

if(MSVC)
    target_compile_options(OrionGame PRIVATE /W4 /EHsc)
    target_compile_definitions(OrionGame PRIVATE UNICODE _UNICODE)
endif()

# -------------------------------------------------------
# ${RUNTIME_PROJECT_NAME}.dll（ゲームロジック DLL）
# -------------------------------------------------------
add_library(${RUNTIME_PROJECT_NAME} SHARED
    ${RUNTIME_DLL_SOURCES}
    ${CMAKE_SOURCE_DIR}/resources/app.rc
)

target_compile_definitions(${RUNTIME_PROJECT_NAME} PRIVATE
    ORION_RUNTIME_EXPORTS
    UNICODE _UNICODE
)

target_link_libraries(${RUNTIME_PROJECT_NAME}
    PRIVATE OrionEngine
)

set_target_properties(${RUNTIME_PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY                "${CMAKE_BINARY_DIR}/runtime"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG          "${CMAKE_BINARY_DIR}/runtime"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE        "${CMAKE_BINARY_DIR}/runtime"
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_BINARY_DIR}/runtime"
    RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL     "${CMAKE_BINARY_DIR}/runtime"
    ARCHIVE_OUTPUT_DIRECTORY                "${CMAKE_BINARY_DIR}/lib"
    ARCHIVE_OUTPUT_DIRECTORY_DEBUG          "${CMAKE_BINARY_DIR}/lib"
    ARCHIVE_OUTPUT_DIRECTORY_RELEASE        "${CMAKE_BINARY_DIR}/lib"
)

if(MSVC)
    target_compile_options(${RUNTIME_PROJECT_NAME} PRIVATE /W4 /EHsc)
endif()