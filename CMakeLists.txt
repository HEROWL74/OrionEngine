cmake_minimum_required(VERSION 3.21)
if(EMSCRIPTEN)
    project(Orion LANGUAGES CXX)
else()
    project(Orion LANGUAGES CXX RC)
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

foreach(CONFIG ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${CONFIG} CONFIG_UPPER)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${CONFIG_UPPER} ${CMAKE_BINARY_DIR}/lib)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${CONFIG_UPPER} ${CMAKE_BINARY_DIR}/lib)
endforeach()

add_subdirectory(engine)

if(NOT EMSCRIPTEN)
    add_subdirectory(engine-assets)
    add_subdirectory(tools/buildworker)
    add_subdirectory(editor)
endif()

add_subdirectory(runtime)