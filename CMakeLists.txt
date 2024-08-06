cmake_minimum_required(VERSION 3.12)
project(OpenH264Encode)

set(CMAKE_CXX_STANDARD 11)
include(ExternalProject)
    ExternalProject_Add(Ext_openh264
        GIT_REPOSITORY https://github.com/cisco/openh264.git
        GIT_TAG        v2.4.1
        GIT_SUBMODULES ""
        UPDATE_COMMAND ""
        INSTALL_COMMAND ""
        UPDATE_DISCONNECTED false
        SOURCE_DIR ${CMAKE_SOURCE_DIR}/External/openh264
        BINARY_DIR ${CMAKE_SOURCE_DIR}/External/openh264
        GIT_PROGRESS 1
        CONFIGURE_COMMAND ""
        BUILD_COMMAND OS=linux make ARCH=x86_64
        STEP_TARGETS build
        EXCLUDE_FROM_ALL TRUE
    )       
    add_library(openh264_lib STATIC IMPORTED)
    set_target_properties(openh264_lib PROPERTIES
    IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/External/openh264/libopenh264.a
)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GST REQUIRED gstreamer-1.0 gstreamer-app-1.0)
include_directories(${GST_INCLUDE_DIRS})
link_directories(${GST_LIBRARY_DIRS})

add_executable(OpenH264Encode main.cpp)

add_dependencies(OpenH264Encode Ext_openh264)
target_include_directories(OpenH264Encode PUBLIC "./" ${CMAKE_SOURCE_DIR}/External/openh264/codec/api/wels)
target_link_libraries(OpenH264Encode 
    openh264_lib
    ${GST_LIBRARIES}
)

