cmake_minimum_required(VERSION 3.21)
if(WIN32)
    project(Orion LANGUAGES CXX RC)
else()
    project(Orion LANGUAGES CXX)
endif()

find_program(SCCACHE_EXE sccache)
if(SCCACHE_EXE)
    set(CMAKE_CXX_COMPILER_LAUNCHER ${SCCACHE_EXE})
    set(CMAKE_C_COMPILER_LAUNCHER   ${SCCACHE_EXE})
    message(STATUS "OrionEngine: sccache enabled")

    if(MSVC)
        # CMakeがデフォルトで挿入する /Zi を全構成から除去して /Z7 に統一する
        # （/Zi はsccacheと競合するため）
        foreach(flag_var
            CMAKE_CXX_FLAGS
            CMAKE_CXX_FLAGS_DEBUG
            CMAKE_CXX_FLAGS_RELEASE
            CMAKE_CXX_FLAGS_RELWITHDEBINFO
            CMAKE_CXX_FLAGS_MINSIZEREL
            CMAKE_C_FLAGS
            CMAKE_C_FLAGS_DEBUG
            CMAKE_C_FLAGS_RELEASE
            CMAKE_C_FLAGS_RELWITHDEBINFO
            CMAKE_C_FLAGS_MINSIZEREL)
            string(REPLACE "/Zi" "" ${flag_var} "${${flag_var}}")
            string(REPLACE "/ZI" "" ${flag_var} "${${flag_var}}")
        endforeach()

        # オブジェクトファイルにデバッグ情報を埋め込む（sccache対応）
        add_compile_options(/Z7)
        # インクリメンタルリンク無効（CI環境向け）
        add_link_options(/INCREMENTAL:NO)

        # C1128エラー対策：セクション数の上限を拡張
        add_compile_options(/bigobj)
    
        add_compile_options(/W4)
    endif()
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---------------------------------------------------------
# 出力ディレクトリ
# ---------------------------------------------------------
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
foreach(CONFIG ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${CONFIG} CONFIG_UPPER)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${CONFIG_UPPER} ${CMAKE_BINARY_DIR}/lib)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${CONFIG_UPPER} ${CMAKE_BINARY_DIR}/lib)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${CONFIG_UPPER} ${CMAKE_BINARY_DIR}/bin)
endforeach()

# ---------------------------------------------------------
# サブディレクトリ
# ---------------------------------------------------------
add_subdirectory(engine)
add_subdirectory(renderer)
add_subdirectory(engine-assets)
add_subdirectory(tools/buildworker)
if(WIN32)
    add_subdirectory(runtime)
endif()
add_subdirectory(editor)