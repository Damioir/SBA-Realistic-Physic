cmake_minimum_required(VERSION 3.22.1)

project(SBARealisticPhysics LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(third_party/dobby-mini)

add_library(
    sba_physics
    SHARED
    src/physics.cpp
    src/costume_map.cpp
    src/editor_session.cpp
    src/map_storage.cpp
    src/unity_map_runtime.cpp
)

find_library(log-lib log)

target_link_libraries(
    sba_physics
    ${log-lib}
    dobby_mini
)
