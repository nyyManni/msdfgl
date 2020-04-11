add_executable(msdfgldemo demo.c)

find_package(glfw3 3.3 QUIET)
if(NOT TARGET glfw)
    include(FetchContent)
    fetchcontent_declare(glfw3
            URL https://github.com/glfw/glfw/archive/3.3.1.tar.gz
            URL_MD5 ffa7c0d8547f56b0897276c59efdd641
            )
    fetchcontent_getproperties(glfw3)
    if(NOT glfw3_POPULATED)
        foreach(_glfw3_option "GLFW_BUILD_TESTS" "GLFW_BUILD_EXAMPLES" "GLFW_BUILD_DOCS" "GLFW_INSTALL")
            set(${_glfw3_option} OFF CACHE INTERNAL "")
        endforeach()
        fetchcontent_populate(glfw3)
        add_subdirectory(${glfw3_SOURCE_DIR} ${glfw3_BINARY_DIR})
    endif()
endif()
if (APPLE)
  set_target_properties(msdfgldemo PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
  target_compile_options(msdfgldemo PRIVATE -DGL_SILENCE_DEPRECATION -Wno-macro-redefined)
endif()


target_include_directories(msdfgldemo PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
target_link_libraries(msdfgldemo PRIVATE msdfgl glfw)
