
cmake_minimum_required (VERSION 3.9)
project (msdfgl VERSION 0.1 DESCRIPTION "OpenGL MSFD Generator and renderer")

find_package(Freetype REQUIRED)
include_directories(${FREETYPE_INCLUDE_DIRS})

set(OpenGL_GL_PREFERENCE LEGACY)
find_package(OpenGL REQUIRED)
include_directories(${OPENGL_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_BINARY_DIR})


set(SHADERS ${CMAKE_CURRENT_SOURCE_DIR}/src/font_fragment.glsl
            ${CMAKE_CURRENT_SOURCE_DIR}/src/font_geometry.glsl
            ${CMAKE_CURRENT_SOURCE_DIR}/src/font_vertex.glsl
            ${CMAKE_CURRENT_SOURCE_DIR}/src/msdf_fragment.glsl
            ${CMAKE_CURRENT_SOURCE_DIR}/src/msdf_vertex.glsl)
set(SHADER_HEADER ${CMAKE_CURRENT_BINARY_DIR}/_msdfgl_shaders.h)

message("-- Generating header generator")
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/gen-shaders
"
OUTPUT=\$1
shift

echo '#ifndef _MSDF_SHADERS_H\\n#define _MSDF_SHADERS_H' >$OUTPUT
for header; do
        fname=\${header##*/};
        echo '\\nconst char * _'\${fname\%\%.glsl}' =' >>\$OUTPUT
        sed -e 's/\\\\/\\\\\\\\/' \\
            -e 's/^\\(.*\\)\$/\"\\1\\\\n\"/' \\
            \$header >>\$OUTPUT; \\
        echo ';' >>\$OUTPUT
done
echo '#endif /* _MSDF_SHADERS_H */' >>\$OUTPUT"
  )

add_custom_command(
    PRE_BUILD
    OUTPUT ${SHADER_HEADER}
    COMMENT "Generating shader header"
    COMMAND sh ${CMAKE_CURRENT_BINARY_DIR}/gen-shaders ${SHADER_HEADER} ${SHADERS}
    DEPENDS ${SHADERS}
)

add_library(msdfgl SHARED src/msdfgl.c src/msdfgl_serializer.c ${SHADER_HEADER})
if (APPLE)

  # We make it the library user's responsibility to link against some OpenGL.
  set_target_properties(msdfgl PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
  add_definitions(-DGL_SILENCE_DEPRECATION)
endif (APPLE)

target_include_directories (msdfgl PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(msdfgl ${FREETYPE_LIBRARIES})

install(TARGETS msdfgl DESTINATION lib)
install(FILES include/msdfgl.h DESTINATION include)
