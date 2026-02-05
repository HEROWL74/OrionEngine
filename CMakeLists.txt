cmake_minimum_required(VERSION 3.21)

project(Orion LANGUAGES CXX RC)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(engine)
add_subdirectory(tools/buildworker)
add_subdirectory(editor)
add_subdirectory(runtime)
