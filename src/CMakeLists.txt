configure_file(../include/msdfgl.h.in msdfgl.h @ONLY)

# Finds all .glsl shaders in our source directory.
file(GLOB_RECURSE msdfgl_shaders ${CMAKE_CURRENT_SOURCE_DIR}/*.glsl)

add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/_msdfgl_shaders.h
                   DEPENDS ${msdfgl_shaders}
                   COMMAND ${CMAKE_COMMAND} -DSHADER_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
                                            -DTARGET_DIR="${CMAKE_CURRENT_BINARY_DIR}"
                                            -P "${CMAKE_CURRENT_SOURCE_DIR}/msdfgl_shaders.cmake")

find_package(Freetype 2.9.1 QUIET)
if(NOT FREETYPE_FOUND)
    include(FetchContent)
    fetchcontent_declare(freetype
                         URL https://download.savannah.gnu.org/releases/freetype/freetype-2.10.1.tar.gz
                         URL_MD5 c50a3c9e5e62bdc938a6e1598a782947)
    fetchcontent_getproperties(freetype)
    if(NOT freetype_POPULATED)
        foreach(_freetype_option "SKIP_INSTALL_HEADERS" "SKIP_INSTALL_LIBRARIES"
                "CMAKE_DISABLE_FIND_PACKAGE_PNG" "CMAKE_DISABLE_FIND_PACKAGE_HarfBuzz"
                "CMAKE_DISABLE_FIND_PACKAGE_BZip2" "CMAKE_DISABLE_FIND_PACKAGE_ZLIB"
                "CMAKE_DISABLE_FIND_PACKAGE_BROTLI")
            set(${_freetype_option} TRUE CACHE INTERNAL "")
        endforeach()
        fetchcontent_populate(freetype)
        add_subdirectory(${freetype_SOURCE_DIR} ${freetype_BINARY_DIR})
    endif()
	set(FREETYPE_LIBRARIES "freetype")
endif()

add_library(msdfgl ../include/msdfgl.h msdfgl.c msdfgl_serializer.c msdfgl_map.c
            ${CMAKE_CURRENT_BINARY_DIR}/_msdfgl_shaders.h)
if(BUILD_SHARED_LIBS)
    target_compile_definitions(msdfgl PRIVATE MSDFGL_EXPORTS)
else()
    target_compile_definitions(msdfgl PUBLIC MSDFGL_STATIC_DEFINE)
endif()
target_include_directories(msdfgl PUBLIC
                           $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
                           ${FREETYPE_INCLUDE_DIRS})
target_link_libraries(msdfgl PUBLIC glad ${FREETYPE_LIBRARIES} ${CMAKE_DL_LIBS})

target_compile_features(msdfgl PUBLIC c_std_11)
if(MSVC)
    #target_compile_options(msdfgl PRIVATE /W4 /WX)
elseif (APPLE)
  set_target_properties(msdfgl PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
  target_compile_options(msdfgl PRIVATE -Wall -Wextra -pedantic -Werror
                         -Wno-overlength-strings -Wno-unused-parameter -Wno-unused-variable
                         -DGL_SILENCE_DEPRECATION)
else()
  target_compile_options(msdfgl PRIVATE -Wall -Wextra -pedantic -Werror
                         -Wno-overlength-strings -Wno-unused-parameter -Wno-unused-variable)
endif()

if(MSDFGL_INSTALL)
    install(TARGETS msdfgl
            LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
            ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
            RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/msdfgl.h
            DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")
endif()
