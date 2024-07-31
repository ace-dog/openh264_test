cmake_minimum_required(VERSION 3.10)
project(OpenH264Encode)

set(CMAKE_CXX_STANDARD 11)

find_package(OpenH264 REQUIRED)

include_directories(${OpenH264_INCLUDE_DIRS})
link_directories(${OpenH264_LIBRARY_DIRS})

add_executable(OpenH264Encode main.cpp sample_frame.cpp)
target_link_libraries(OpenH264Encode ${OpenH264_LIBRARIES})
