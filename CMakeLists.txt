# CMakeLists.txt (Root)

cmake_minimum_required(VERSION 3.21)

if(WIN32)
    project(Orion LANGUAGES CXX RC)
else()
    project(Orion LANGUAGES CXX)
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

foreach(CONFIG ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${CONFIG} CONFIG_UPPER)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${CONFIG_UPPER} ${CMAKE_BINARY_DIR}/lib)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${CONFIG_UPPER} ${CMAKE_BINARY_DIR}/lib)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${CONFIG_UPPER} ${CMAKE_BINARY_DIR}/bin)
endforeach()

# -------------------------------------------------------------------
# Linux: engine と editor だけビルド
# Windows: 全サブディレクトリをビルド（従来通り）
# -------------------------------------------------------------------
if(WIN32)
    add_subdirectory(engine)
    add_subdirectory(engine-assets)
    add_subdirectory(tools/buildworker)
    add_subdirectory(runtime)
    add_subdirectory(editor)
else()
    add_subdirectory(engine)
    add_subdirectory(editor)
endif()