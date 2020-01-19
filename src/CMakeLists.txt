configure_file(../include/msdfgl.h.in ${CMAKE_CURRENT_SOURCE_DIR}/../include/msdfgl.h @ONLY)

# Finds all .glsl shaders in our source directory.
file(GLOB_RECURSE  _temp_msdfgl_shaders ${CMAKE_CURRENT_SOURCE_DIR}/*.glsl)
# Creates a C-style string for each shader source.
foreach(_temp_msdfgl_shader IN LISTS _temp_msdfgl_shaders)
    # Gets the shader file name, without the extension.
    get_filename_component(_temp_shader_name ${_temp_msdfgl_shader} NAME_WE)
    # Reads the contents of the GLSL shader.
    file(READ ${_temp_msdfgl_shader} _temp_shader_contents)
    # Escapes characters so they can be contained in C style strings.
    string(REPLACE "\"" "\\\"" _temp_shader_contents ${_temp_shader_contents})
    string(REPLACE "\\" "\\\\" _temp_shader_contents ${_temp_shader_contents})
    # Add quotes around each line in the shader.
    string(REPLACE "\n" "\"\n\"" _temp_shader_contents ${_temp_shader_contents})
    set(_temp_msdfgl_shaders_sources "${_temp_msdfgl_shaders_sources}\n\nconst char * _${_temp_shader_name} = \"${_temp_shader_contents}\";")
endforeach()
# Write a header guard around our GLSL shader sources.
file(WRITE ${CMAKE_CURRENT_SOURCE_DIR}/_msdfgl_shaders.h "#ifndef _MSDFGL_SHADERS_H
#define _MSDFGL_SHADERS_H${_temp_msdfgl_shaders_sources}\n
#endif /* _MSDFGL_SHADERS_H */")

find_package(Freetype 2.9.1 QUIET)
if(NOT FREETYPE_FOUND)
    FetchContent_Declare(freetype
            URL https://download.savannah.gnu.org/releases/freetype/freetype-2.10.1.tar.gz
            URL_MD5 c50a3c9e5e62bdc938a6e1598a782947
            )
    FetchContent_GetProperties(freetype)
    if(NOT freetype_POPULATED)
        foreach(_freetype_option "CMAKE_DISABLE_FIND_PACKAGE_PNG" "CMAKE_DISABLE_FIND_PACKAGE_HarfBuzz"
                "CMAKE_DISABLE_FIND_PACKAGE_BZip2" "CMAKE_DISABLE_FIND_PACKAGE_ZLIB" "CMAKE_DISABLE_FIND_PACKAGE_BROTLI"
                )
            set(${_freetype_option} TRUE CACHE INTERNAL "")
        endforeach()
        FetchContent_Populate(freetype)
        add_subdirectory(${freetype_SOURCE_DIR} ${freetype_BINARY_DIR})
    endif()
endif()

add_library(msdfgl ../include/msdfgl.h msdfgl.c msdfgl_serializer.c msdfgl_map.c)
if(BUILD_SHARED_LIBS)
    target_compile_definitions(msdfgl PRIVATE MSDFGL_EXPORTS)
else()
    target_compile_definitions(msdfgl PUBLIC MSDFGL_STATIC_DEFINE)
endif()
target_include_directories(msdfgl PUBLIC 
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
        $<$<BOOL:${FREETYPE_INCLUDE_DIRS}>:${FREETYPE_INCLUDE_DIRS}>
        )
target_link_libraries(msdfgl PUBLIC glad ${FREETYPE_LIBRARIES})

target_compile_features(msdfgl PUBLIC c_std_11)
if(MSVC)
    #target_compile_options(msdfgl PRIVATE /W4 /WX)
else()
    target_compile_options(msdfgl PRIVATE -Wall -Wextra -pedantic -Werror -Wno-overlength-strings -Wno-unused-parameter -Wno-unused-variable)
endif()
