# engine/CMakeLists.txt
file(GLOB_RECURSE ENGINE_SOURCES
    ${CMAKE_SOURCE_DIR}/src/engine/*.cpp
    ${CMAKE_SOURCE_DIR}/src/engine/*.hpp
    ${CMAKE_SOURCE_DIR}/src/shared/*.cpp
)

add_library(OrionEngine STATIC ${ENGINE_SOURCES})

target_include_directories(OrionEngine PUBLIC
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/ThirdParty
)

find_package(lua CONFIG REQUIRED)
find_package(sol2 CONFIG REQUIRED)
find_package(DirectXTex CONFIG REQUIRED)

target_link_libraries(OrionEngine PUBLIC
    lua
    sol2
    Microsoft::DirectXTex
    d3d12 dxgi dxguid d3dcompiler
)

if(MSVC)
    target_compile_options(OrionEngine PRIVATE /W4 /EHsc)
    target_compile_definitions(OrionEngine PRIVATE UNICODE _UNICODE)
endif()
