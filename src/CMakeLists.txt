configure_file(../include/msdfgl.h.in ${CMAKE_CURRENT_SOURCE_DIR}/../include/msdfgl.h @ONLY)

file(GLOB_RECURSE MSDFGL_SHADERS ${CMAKE_CURRENT_SOURCE_DIR}/*.glsl)
foreach(MSDFGL_SHADER IN LISTS MSDFGL_SHADERS)
    get_filename_component(${MSDFGL_SHADER}_NAME ${MSDFGL_SHADER} NAME_WE)
    file(READ ${MSDFGL_SHADER} ${MSDFGL_SHADER}_CONTENTS)
    set(COMBINED_SHADERS "${COMBINED_SHADERS}\n\nconst char * _${${MSDFGL_SHADER}_NAME} = R\"(${${MSDFGL_SHADER}_CONTENTS})\";")
endforeach()
file(WRITE ${CMAKE_CURRENT_SOURCE_DIR}/_msdfgl_shaders.h "#ifndef _MSDFGL_SHADERS_H
#define _MSDFGL_SHADERS_H${COMBINED_SHADERS}\n
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

add_library(msdfgl msdfgl.c msdfgl_serializer.c msdfgl_map.c)
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
    target_compile_options(msdfgl PRIVATE -Wall -Wextra -pedantic -Werror -Wno-overlength-strings -Wno-unused-parameter -Wno-unused-variable)
endif()
