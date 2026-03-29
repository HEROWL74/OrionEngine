# rootDirectory CMakeLists
cmake_minimum_required(VERSION 3.21)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

if(WIN32)
    project(Orion LANGUAGES CXX RC)
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
if(WIN32)
    add_subdirectory(runtime)
endif()
add_subdirectory(editor)