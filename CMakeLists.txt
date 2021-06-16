cmake_minimum_required(VERSION 3.19)
project(GradientLib)

set(CMAKE_CXX_STANDARD 11)

add_definitions(-DBUILD_INTERFACES)
include_directories(include)
file(GLOB SRC src/*.cpp test/*.cpp)

add_executable(${PROJECT_NAME} ${SRC} src/ICompact.cpp)