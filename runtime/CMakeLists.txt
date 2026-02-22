# ==============================================
# Runtime (Game Executable)
# ==============================================

# ------------------------------------------------
# 変数（Editor から渡される）
# ------------------------------------------------
if(NOT DEFINED RUNTIME_PROJECT_NAME)
    set(RUNTIME_PROJECT_NAME "OrionGame")
endif()

message(STATUS "Runtime Target Name: ${RUNTIME_PROJECT_NAME}")

# ------------------------------------------------
# ソース収集
# ルートの src/runtime を参照する
# ------------------------------------------------
file(GLOB_RECURSE RUNTIME_SOURCES
    ${CMAKE_SOURCE_DIR}/src/runtime/*.cpp
    ${CMAKE_SOURCE_DIR}/src/runtime/*.hpp
)

# ------------------------------------------------
# 実行ファイル
# ------------------------------------------------
add_executable(${RUNTIME_PROJECT_NAME} WIN32
    ${RUNTIME_SOURCES}
    ${CMAKE_SOURCE_DIR}/resources/app.rc
)

# ------------------------------------------------
# Engine に依存
# ------------------------------------------------
target_link_libraries(${RUNTIME_PROJECT_NAME}
    PRIVATE OrionEngine
)

# ------------------------------------------------
# 出力ディレクトリ
# buildDir/runtime/{Config}/ に出力する
# ------------------------------------------------
set_target_properties(${RUNTIME_PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/runtime"
)


# ------------------------------------------------
# MSVC 設定
# ------------------------------------------------
if (MSVC)
    target_compile_options(${RUNTIME_PROJECT_NAME} PRIVATE /W4 /EHsc)
    target_compile_definitions(${RUNTIME_PROJECT_NAME} PRIVATE UNICODE _UNICODE)
endif()