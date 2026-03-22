# ---------------------------------------------------------
# Renderer Sources
# ---------------------------------------------------------

file(GLOB_RECURSE RENDERER_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp
)

add_library(OrionRenderer STATIC
    ${RENDERER_SOURCES}
)

# ---------------------------------------------------------
# Include directories
# ---------------------------------------------------------

target_include_directories(OrionRenderer PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/engine
)

# ---------------------------------------------------------
# Engine dependency
# ---------------------------------------------------------

target_link_libraries(OrionRenderer PUBLIC
    OrionEngine
)

# ---------------------------------------------------------
# Windows (DX12)
# ---------------------------------------------------------

if(WIN32)

    find_package(directxtex CONFIG REQUIRED)
    find_package(directx-headers CONFIG REQUIRED)

    target_link_libraries(OrionRenderer PUBLIC
        Microsoft::DirectX-Headers
        d3d12
        dxgi
        dxguid
        d3dcompiler
    )

    target_link_libraries(OrionRenderer PRIVATE
        Microsoft::DirectXTex
    )

endif()

# ---------------------------------------------------------
# Linux (Vulkan)
# ---------------------------------------------------------

if(UNIX AND NOT APPLE)

    find_package(Vulkan REQUIRED)
    find_package(glfw3 CONFIG REQUIRED)

    target_link_libraries(OrionRenderer PUBLIC
        Vulkan::Vulkan
        glfw
    )

endif()
