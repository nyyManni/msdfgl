cmake_minimum_required(VERSION 3.9 FATAL_ERROR)
project(msdfgl VERSION 0.2.1 DESCRIPTION "OpenGL MSDF Generator and renderer" LANGUAGES C)

option(BUILD_SHARED_LIBS "Build Shared Libraries" ON)
option(BUILD_MSDFGL_EXAMPLE "Build MSDF example project" ON)

add_subdirectory(third_party/glad)
add_subdirectory(src)
if(BUILD_MSDFGL_EXAMPLE)
    add_subdirectory(example)
endif()
