add_executable(msdfgldemo demo.c)

find_package(glfw3 3.3 QUIET)
if(NOT TARGET glfw)
    include(FetchContent)
    FetchContent_Declare(glfw3
            URL https://github.com/glfw/glfw/archive/3.3.1.tar.gz
            URL_MD5 ffa7c0d8547f56b0897276c59efdd641
            )
    FetchContent_GetProperties(glfw3)
    if(NOT glfw3_POPULATED)
        foreach(_glfw3_option "GLFW_BUILD_TESTS" "GLFW_BUILD_EXAMPLES" "GLFW_BUILD_DOCS" "GLFW_INSTALL")
            set(${_glfw3_option} OFF CACHE INTERNAL "")
        endforeach()
        FetchContent_Populate(glfw3)
        add_subdirectory(${glfw3_SOURCE_DIR} ${glfw3_BINARY_DIR})
    endif()
endif()

target_include_directories(msdfgldemo PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
target_link_libraries(msdfgldemo PRIVATE msdfgl glfw)