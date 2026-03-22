add_library(OrionBuildSystem

    ${CMAKE_CURRENT_SOURCE_DIR}/BuildSystem.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/BuildSystem.hpp
)

target_include_directories(OrionBuildSystem PUBLIC

    ${CMAKE_SOURCE_DIR}/engine
)

target_link_libraries(OrionBuildSystem PUBLIC

    OrionEngine
)
