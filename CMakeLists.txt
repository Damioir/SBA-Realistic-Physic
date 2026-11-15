Cmake
cmake_minimum_required(VERSION 3.22.1)

project(SBARealisticPhysics)

add_library(
    sba_physics
    SHARED
    src/physics.cpp
)

find_library(log-lib log)

target_link_libraries(
    sba_physics
    ${log-lib}
)
