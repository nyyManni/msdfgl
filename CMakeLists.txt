cmake_minimum_required(VERSION 3.11.0 FATAL_ERROR)
project(msdfgl VERSION 0.2.2 DESCRIPTION "OpenGL MSDF Generator and renderer" LANGUAGES C)

include(GNUInstallDirs)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_BINDIR})

option(BUILD_SHARED_LIBS "Build Shared Libraries" ON)
option(BUILD_MSDFGL_EXAMPLE "Build MSDF example project" ON)
option(MSDFGL_INSTALL "Generate installation target" ON)

if(NOT TARGET glad)
    add_subdirectory(third_party/glad)
endif()
add_subdirectory(src)
if(BUILD_MSDFGL_EXAMPLE)
    add_subdirectory(example)
endif()
