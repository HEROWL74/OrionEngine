# tools/OrionResourceInjector/CMakeLists.txt
#
# POST_BUILD でアイコンを .exe に注入するスタンドアロンツール。
# Windows 専用。

if(NOT WIN32)
    return()
endif()

add_executable(OrionResourceInjector
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
)

target_compile_definitions(OrionResourceInjector PRIVATE
    UNICODE
    _UNICODE
)

# ツールは bin ではなく tools/ 以下に出力する
set_target_properties(OrionResourceInjector PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY                "${CMAKE_BINARY_DIR}/tools"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG          "${CMAKE_BINARY_DIR}/tools"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE        "${CMAKE_BINARY_DIR}/tools"
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_BINARY_DIR}/tools"
    RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL     "${CMAKE_BINARY_DIR}/tools"
)