# engine/CMakeLists.txt

if(WIN32)
    # -------------------------------------------------------------------------
    # Windows: engine
    # -------------------------------------------------------------------------
    file(GLOB_RECURSE ENGINE_SOURCES
        ${CMAKE_SOURCE_DIR}/engine/*.cpp
        ${CMAKE_SOURCE_DIR}/engine/*.hpp
    )

else()
    # -------------------------------------------------------------------------
    # Linux: Window_Linux / IWindow / WindowFactory
    # -------------------------------------------------------------------------
    set(ENGINE_SOURCES
        ${CMAKE_SOURCE_DIR}/engine/Core/Window/IWindow.hpp
        ${CMAKE_SOURCE_DIR}/engine/Core/Window/WindowFactory.hpp
        ${CMAKE_SOURCE_DIR}/engine/Core/Window/Window_Linux.hpp
        ${CMAKE_SOURCE_DIR}/engine/Core/Window/Window_Linux.cpp
    )

endif()

add_library(OrionEngine STATIC ${ENGINE_SOURCES})

find_path(MINIAUDIO_INCLUDE_DIRS "miniaudio.h")
find_package(Lua  REQUIRED)
find_package(sol2 CONFIG REQUIRED)

target_include_directories(OrionEngine PUBLIC
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/engine
    ${MINIAUDIO_INCLUDE_DIRS}
    ${LUA_INCLUDE_DIR}
)

if(WIN32)
    find_package(directxtex      CONFIG REQUIRED)
    find_package(directx-headers CONFIG REQUIRED)
    find_package(directx12-agility CONFIG REQUIRED)

    target_link_libraries(OrionEngine PUBLIC
        ${LUA_LIBRARIES}
        sol2::sol2
        Microsoft::DirectX-Headers
        Microsoft::DirectX12-Agility
        d3d12 dxgi dxguid d3dcompiler
    )
    target_link_libraries(OrionEngine PRIVATE
        Microsoft::DirectXTex
    )

    if(MSVC)
        target_compile_definitions(OrionEngine PRIVATE
            $<$<CONFIG:Debug>:_DEBUG>
            $<$<CONFIG:Debug>:USE_PIX>
            $<$<CONFIG:Release>:NDEBUG>
            $<$<CONFIG:RelWithDebInfo>:NDEBUG>
            $<$<CONFIG:RelWithDebInfo>:USE_PIX>
        )
    endif()

else()
    find_package(Vulkan REQUIRED)
    find_package(glfw3  CONFIG REQUIRED)

    target_link_libraries(OrionEngine PUBLIC
        ${LUA_LIBRARIES}
        sol2::sol2
        Vulkan::Vulkan
        glfw
    )
    target_compile_definitions(OrionEngine PUBLIC ORION_PLATFORM_LINUX)

endif()