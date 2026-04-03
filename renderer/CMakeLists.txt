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
    # CMakeが「OpenMPが見つからない！」と騒ぐのを、変数を直接書き換えて黙らせる
    set(OpenMP_C_FOUND TRUE)
    set(OpenMP_CXX_FOUND TRUE)
    
    # clang-cl が MSVC の vcomp.lib を使えるようにダミーフラグを設定
    set(OpenMP_C_FLAGS "/openmp")
    set(OpenMP_CXX_FLAGS "/openmp")
    set(OpenMP_CXX_LIB_NAMES "vcomp")
    set(OpenMP_vcomp_LIBRARY "vcomp")

    # directxtex が内部で期待している「OpenMP::OpenMP_CXX」というターゲットを手動で作成する
    if(NOT TARGET OpenMP::OpenMP_CXX)
        add_library(OpenMP::OpenMP_CXX INTERFACE IMPORTED)
        # clang-cl に MSVC の OpenMP ライブラリをリンクするよう指示
        set_target_properties(OpenMP::OpenMP_CXX PROPERTIES
            INTERFACE_COMPILE_OPTIONS "/openmp"
            INTERFACE_LINK_LIBRARIES "vcomp"
        )
    endif()
endif()

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
