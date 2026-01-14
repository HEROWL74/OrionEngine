# RootDirectory CMakeLists.txt
cmake_minimum_required(VERSION 3.21)

project(Orion LANGUAGES CXX RC)

# --------------------------------------------------
# vcpkg toolchain
# --------------------------------------------------
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE AND EXISTS
   "${CMAKE_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake")
    set(CMAKE_TOOLCHAIN_FILE
        "${CMAKE_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake"
        CACHE STRING "")
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(engine)
add_subdirectory(editor)
add_subdirectory(runtimeDebug)
