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
