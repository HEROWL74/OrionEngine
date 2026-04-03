# rootDirectory CMakeLists
cmake_minimum_required(VERSION 3.21)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

if(WIN32)
    project(Orion LANGUAGES CXX RC)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        # RCコンパイラとして直接 rc.exe を指定し、Clang経由のプリプロセスをバイパスする
        set(CMAKE_RC_COMPILER "rc.exe")
        # 余計なフラグが渡らないように空にする
        set(CMAKE_RC_FLAGS "/rv /DWIN32")
    endif()
else()
    project(Orion LANGUAGES CXX)
endif()

include(OrionCompileOptions)
ORION_SETUP_COMPILER_ENVIRONMENT()

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

# アイコン注入ツール（OrionGame / OrionEditor の POST_BUILD より先にビルドされる必要がある）
if(WIN32)
    add_subdirectory(tools/OrionResourceInjector)
endif()

if(WIN32)
    add_subdirectory(runtime)
endif()
add_subdirectory(editor)