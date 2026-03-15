cmake_minimum_required(VERSION 3.21)

if(WIN32)
    project(Orion LANGUAGES CXX RC)
else()
    project(Orion LANGUAGES CXX)
endif()

find_program(SCCACHE_EXE sccache)
if(SCCACHE_EXE)
    set(CMAKE_CXX_COMPILER_LAUNCHER ${SCCACHE_EXE})
    set(CMAKE_C_COMPILER_LAUNCHER   ${SCCACHE_EXE})
    message(STATUS "OrionEngine: sccache enabled")
    
    if(MSVC)
        # どの構成でも/Z7 を使用してオブジェクト内に情報を埋め込む
        add_compile_options(/Z7)

        # CI環境ではリンク時間を短縮するためにインクリメンタルリンクを無効化
        add_link_options(/INCREMENTAL:NO)
    endif()
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---------------------------------------------------------
# 出力ディレクトリ
# ---------------------------------------------------------
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

foreach(CONFIG ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${CONFIG} CONFIG_UPPER)

    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${CONFIG_UPPER} ${CMAKE_BINARY_DIR}/lib)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${CONFIG_UPPER} ${CMAKE_BINARY_DIR}/lib)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${CONFIG_UPPER} ${CMAKE_BINARY_DIR}/bin)
endforeach()

# ---------------------------------------------------------
# サブディレクトリ
# ---------------------------------------------------------

add_subdirectory(engine)
add_subdirectory(renderer)
add_subdirectory(engine-assets)
add_subdirectory(tools/buildworker)

if(WIN32)
    add_subdirectory(runtime)
endif()

add_subdirectory(editor)