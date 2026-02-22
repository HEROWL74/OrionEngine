cmake_minimum_required(VERSION 3.21)

project(Orion LANGUAGES CXX RC)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# =========================================================
# Output Layout
# =========================================================

# lib 出力統一
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Visual Studio (multi-config) 対応
foreach(CONFIG ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${CONFIG} CONFIG_UPPER)

    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${CONFIG_UPPER}
        ${CMAKE_BINARY_DIR}/lib)

    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${CONFIG_UPPER}
        ${CMAKE_BINARY_DIR}/lib)
endforeach()

# =========================================================
# Subdirectories
# =========================================================

add_subdirectory(engine)
add_subdirectory(tools/buildworker)
add_subdirectory(editor)
add_subdirectory(runtime)
