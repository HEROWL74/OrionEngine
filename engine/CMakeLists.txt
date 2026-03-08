# engine/CMakeLists.txt

if(WIN32)
    # -------------------------------------------------------------------------
    # Windows: 既存の全ソースをそのまま使う（変更なし）
    # -------------------------------------------------------------------------
    file(GLOB_RECURSE ENGINE_SOURCES
        ${CMAKE_SOURCE_DIR}/src/engine/*.cpp
        ${CMAKE_SOURCE_DIR}/src/engine/*.hpp
    )

else()
    # -------------------------------------------------------------------------
    # Linux: Window_Linux / IWindow / WindowFactory のみ
    #   Phase 2 以降: VulkanDevice 等をここに追加していく
    # -------------------------------------------------------------------------
    set(ENGINE_SOURCES
        ${CMAKE_SOURCE_DIR}/src/engine/Core/Window/IWindow.hpp
        ${CMAKE_SOURCE_DIR}/src/engine/Core/Window/WindowFactory.hpp
        ${CMAKE_SOURCE_DIR}/src/engine/Core/Window/Window_Linux.hpp
        ${CMAKE_SOURCE_DIR}/src/engine/Core/Window/Window_Linux.cpp
    )

endif()

add_library(OrionEngine STATIC ${ENGINE_SOURCES})

find_path(MINIAUDIO_INCLUDE_DIRS "miniaudio.h")
find_package(Lua  REQUIRED)
find_package(sol2 CONFIG REQUIRED)

target_include_directories(OrionEngine PUBLIC
    ${CMAKE_SOURCE_DIR}/src
    ${MINIAUDIO_INCLUDE_DIRS}
    ${LUA_INCLUDE_DIR}
)

if(WIN32)
    find_package(directxtex      CONFIG REQUIRED)
    find_package(directx-headers CONFIG REQUIRED)

    target_link_libraries(OrionEngine PUBLIC
        ${LUA_LIBRARIES}
        sol2
        Microsoft::DirectX-Headers
        d3d12 dxgi dxguid d3dcompiler
    )
    target_link_libraries(OrionEngine PRIVATE
        directxtex::DirectXTex
    )

    if(MSVC)
        target_compile_definitions(OrionEngine PRIVATE
            $<$<CONFIG:Debug>:_DEBUG>
            $<$<CONFIG:Release>:NDEBUG>
            $<$<CONFIG:RelWithDebInfo>:NDEBUG>
        )
    endif()

else()
    find_package(Vulkan REQUIRED)
    find_package(glfw3  CONFIG REQUIRED)

    target_link_libraries(OrionEngine PUBLIC
        ${LUA_LIBRARIES}
        sol2
        Vulkan::Vulkan
        glfw
    )
    target_compile_definitions(OrionEngine PUBLIC ORION_PLATFORM_LINUX)

endif()

if(EMSCRIPTEN)
    target_compile_definitions(OrionEngine PRIVATE ORION_PLATFORM_WEB)
endif()