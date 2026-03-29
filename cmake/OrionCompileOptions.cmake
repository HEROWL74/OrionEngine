# cmake/OrionCompileOptions.cmake

macro(orion_setup_compiler_environment)
    find_program(SCCACHE_EXE sccache)
    if(SCCACHE_EXE)
        set(CMAKE_CXX_COMPILER_LAUNCHER ${SCCACHE_EXE} CACHE STRING "" FORCE)
        set(CMAKE_C_COMPILER_LAUNCHER   ${SCCACHE_EXE} CACHE STRING "" FORCE)
        message(STATUS "OrionEngine: sccache enabled")

        if(MSVC)
            # /Zi を /Z7 に置換（sccache競合回避）
            set(flag_vars
                CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
                CMAKE_CXX_FLAGS_RELWITHDEBINFO CMAKE_CXX_FLAGS_MINSIZEREL
                CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE
                CMAKE_C_FLAGS_RELWITHDEBINFO CMAKE_C_FLAGS_MINSIZEREL)

            foreach(flag_var ${flag_vars})
                if(${flag_var})
                    string(REPLACE "/Zi" "" ${flag_var} "${${flag_var}}")
                    string(REPLACE "/ZI" "" ${flag_var} "${${flag_var}}")
                endif()
            endforeach()

            add_compile_options(/Z7 /bigobj /W4)
            add_link_options(/INCREMENTAL:NO)
        endif()
    endif()

    # 共通の言語設定
    set(CMAKE_CXX_STANDARD 23 CACHE STRING "")
    set(CMAKE_CXX_STANDARD_REQUIRED ON CACHE STRING "")
endmacro()