cmake_minimum_required(VERSION 4.0)
set(CXX_STANDARD 20)
project(gbemu VERSION 1.0)

find_package(SDL2 REQUIRED CONFIG REQUIRED HINTS externals)

file(GLOB_RECURSE SOURCES src/*.cpp)
add_executable(${PROJECT_NAME} WIN32 ${SOURCES})
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 20)

target_include_directories(${PROJECT_NAME} PRIVATE include)
target_link_libraries(${PROJECT_NAME} PRIVATE SDL2::SDL2 shlwapi)
