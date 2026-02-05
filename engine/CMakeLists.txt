# engine/CMakeLists.txt
file(GLOB_RECURSE ENGINE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/engine/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/engine/*.hpp
)

add_library(OrionEngine STATIC ${ENGINE_SOURCES})

find_path(MINIAUDIO_INCLUDE_DIRS "miniaudio.h")

target_include_directories(OrionEngine PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
    ${MINIAUDIO_INCLUDE_DIRS}
)

find_package(lua CONFIG REQUIRED)
find_package(sol2 CONFIG)
find_package(DirectXTex CONFIG REQUIRED)
find_package(directx-headers CONFIG REQUIRED)

target_link_libraries(OrionEngine PUBLIC
    lua
    sol2
    Microsoft::DirectXTex
    Microsoft::DirectX-Headers
    d3d12 dxgi dxguid d3dcompiler
)

if (MSVC)
    target_compile_definitions(OrionEngine PRIVATE
        $<$<CONFIG:Debug>:_DEBUG>
        $<$<CONFIG:Release>:NDEBUG>
        $<$<CONFIG:RelWithDebInfo>:NDEBUG>
    )
endif()

