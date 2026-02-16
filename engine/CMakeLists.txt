# engine/CMakeLists.txt
file(GLOB_RECURSE ENGINE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/engine/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/engine/*.hpp
)

add_library(OrionEngine STATIC ${ENGINE_SOURCES})

find_path(MINIAUDIO_INCLUDE_DIRS "miniaudio.h")

find_package(Lua REQUIRED)
find_package(sol2 CONFIG REQUIRED)
find_package(directxtex CONFIG REQUIRED)
find_package(directx-headers CONFIG REQUIRED)

target_include_directories(OrionEngine PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
    ${MINIAUDIO_INCLUDE_DIRS}
    ${LUA_INCLUDE_DIR}
)

target_link_libraries(OrionEngine PUBLIC
    ${LUA_LIBRARIES}
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

if(EMSCRIPTEN)
     target_compile_definitions(OrionEngine PRIVATE ORION_PLATFORM_WEB)
endif()

