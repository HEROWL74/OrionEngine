cmake_minimum_required(VERSION 3.21)

# vcpkg toolchain
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "C:/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
endif()

project(OrionGameRuntime LANGUAGES CXX RC)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# プロジェクトルートのvcpkg_installed を使用
set(CMAKE_PREFIX_PATH "${CMAKE_SOURCE_DIR}/../vcpkg_installed/x64-windows-static" CACHE PATH "")

# Engine sources
file(GLOB_RECURSE ENGINE_SOURCES
    ${CMAKE_SOURCE_DIR}/../src/engine/*.cpp
    ${CMAKE_SOURCE_DIR}/../src/engine/*.hpp
)

add_library(OrionEngine STATIC ${ENGINE_SOURCES})

target_include_directories(OrionEngine PUBLIC
    ${CMAKE_SOURCE_DIR}/../src
    ${CMAKE_SOURCE_DIR}/../ThirdParty
)

find_package(lua CONFIG REQUIRED)
find_package(sol2 CONFIG REQUIRED)
find_package(DirectXTex CONFIG REQUIRED)

target_link_libraries(OrionEngine PUBLIC
    lua
    sol2::sol2
    Microsoft::DirectXTex
    d3d12 dxgi dxguid d3dcompiler
)

if(MSVC)
    target_compile_options(OrionEngine PRIVATE /W4 /EHsc)
    target_compile_definitions(OrionEngine PRIVATE UNICODE _UNICODE)
endif()

# Runtime executable
file(GLOB_RECURSE RUNTIME_SOURCES
    ${CMAKE_SOURCE_DIR}/../src/runtime/*.cpp
    ${CMAKE_SOURCE_DIR}/../src/runtime/*.hpp
)

add_executable(OrionGame WIN32
    ${RUNTIME_SOURCES}
    ${CMAKE_SOURCE_DIR}/../resources/app.rc
)

target_link_libraries(OrionGame PRIVATE OrionEngine)

if(MSVC)
    target_compile_options(OrionGame PRIVATE /W4 /EHsc)
endif()