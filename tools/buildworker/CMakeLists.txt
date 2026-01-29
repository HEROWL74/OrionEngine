# ‹¤’Êƒ‰ƒCƒuƒ‰ƒŠ
add_library(OrionBuildSystem
    ${CMAKE_SOURCE_DIR}/src/buildworker/BuildSystem.cpp
    ${CMAKE_SOURCE_DIR}/src/buildworker/BuildSystem.hpp
)

target_include_directories(OrionBuildSystem PUBLIC
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(OrionBuildSystem
    PUBLIC OrionEngine
)

# Worker EXE
add_executable(OrionBuildWorker
    ${CMAKE_SOURCE_DIR}/src/buildworker/BuildWorkerMain.cpp
)

target_link_libraries(OrionBuildWorker
    PRIVATE OrionBuildSystem
)

set_target_properties(OrionBuildWorker PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY
        ${CMAKE_SOURCE_DIR}/tools/$<CONFIG>
)
