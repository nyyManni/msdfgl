
cmake_minimum_required (VERSION 3.9)
project (msdfgl VERSION 0.2 DESCRIPTION "OpenGL MSDF Generator and renderer")

if (WIN32)
  # Modify these to match your environment
  set(ENV{FREETYPE_DIR} "${CMAKE_CURRENT_SOURCE_DIR}/lib/freetype/win32")
  set(GLFW3_LIB "C:/Program Files (x86)/glfw-3.3.bin.WIN32/lib-vc2019/glfw3.lib")
endif (WIN32)

find_package(Freetype)
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

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

message("-- Generating header generator")
if (WIN32)
  include_directories(${CMAKE_CURRENT_SOURCE_DIR}/example/glad/include)

  file(
    COPY $ENV{FREETYPE_DIR}/freetype.dll
    DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

  file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/gen-shaders.bat
    "
@echo off

set OUTPUT=\%1

SETLOCAL ENABLEDELAYEDEXPANSION

echo #ifndef _MSDFGL_SHADERS_H > \%OUTPUT\%
echo #define _MSDFGL_SHADERS_H >> \%OUTPUT\%


for \%\%f in (\%2 \%3 \%4 \%5 \%6) do (
	set fname=\%\%f
	set fname=!!fname:/=\\!!
	for \%\%b in (!!fname!!) do (set fname=\%\%~nb)
	echo const char * _!!fname!! = >> \%OUTPUT\%
	for /f \"tokens=*\" \%\%l in (\%\%f) do (
	    set line=\%\%l
	    echo \"!!line:\\=\\\\!!\\n\" >> \%OUTPUT\%
	)
	echo ; >> \%OUTPUT\%
)
echo #endif /* _MSDFGL_SHADERS_H */ >> \%OUTPUT\%")

else ()  # Linux and macOS

  file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/tmp/gen-shaders
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
echo '#endif /* _MSDF_SHADERS_H */' >>\$OUTPUT")
  file(
    COPY ${CMAKE_CURRENT_BINARY_DIR}/tmp/gen-shaders
    DESTINATION ${CMAKE_CURRENT_BINARY_DIR}
    FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)

endif (WIN32)

add_custom_command(
  PRE_BUILD
  OUTPUT ${SHADER_HEADER}
  COMMENT "Generating shader header"
  COMMAND ${CMAKE_CURRENT_BINARY_DIR}/gen-shaders ${SHADER_HEADER} ${SHADERS}
  DEPENDS ${SHADERS})

if (MSVC)
  add_library(msdfgl src/msdfgl.c src/msdfgl_serializer.c src/msdfgl_map.c example/glad/src/glad.c ${SHADER_HEADER})
  add_library(msdfglshared SHARED src/msdfgl.c src/msdfgl_serializer.c src/msdfgl_map.c example/glad/src/glad.c ${SHADER_HEADER})
  set_target_properties(msdfglshared PROPERTIES OUTPUT_NAME msdfgl)
  target_compile_definitions(msdfglshared PRIVATE MSDFGL_EXPORTS=1)

else()
  add_library(msdfgl SHARED src/msdfgl.c src/msdfgl_serializer.c src/msdfgl_map.c ${SHADER_HEADER})
  add_library(msdfglstatic STATIC src/msdfgl.c src/msdfgl_serializer.c src/msdfgl_map.c ${SHADER_HEADER})
  set_target_properties(msdfglstatic PROPERTIES OUTPUT_NAME msdfgl)
endif()

if (APPLE)

  # We make it the library user's responsibility to link against some OpenGL.
  set_target_properties(msdfgl PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
  add_definitions(-DGL_SILENCE_DEPRECATION)
endif (APPLE)

if (MSVC)
  #target_compile_options(msdfgl PRIVATE /W4 /WX)
else()
  target_compile_options(msdfgl PRIVATE -Wall -Wextra -pedantic -Werror -Wno-overlength-strings -Wno-unused-parameter -Wno-unused-variable)
  target_compile_options(msdfglstatic PRIVATE -Wall -Wextra -pedantic -Werror -Wno-overlength-strings -Wno-unused-parameter -Wno-unused-variable)
endif()

target_include_directories (msdfgl PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories (msdfglstatic PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(msdfgl ${FREETYPE_LIBRARIES})
target_link_libraries(msdfglstatic ${FREETYPE_LIBRARIES})

if (MSVC)
  include_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib/glfw3/include)
else ()
  find_package(glfw3)
endif ()

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/example/glad/include)
add_executable(msdfgldemo EXCLUDE_FROM_ALL example/demo.c example/glad/src/glad.c)
if (MSVC)
  target_link_libraries(msdfgldemo msdfgl ${OPENGL_LIBRARIES} ${FREETYPE_LIBRARIES} ${GLFW3_LIB})
else()
  target_link_libraries(msdfgldemo msdfgl ${OPENGL_LIBRARIES} ${FREETYPE_LIBRARIES} glfw dl)
endif()


install(TARGETS msdfgl msdfglstatic DESTINATION lib)
install(FILES include/msdfgl.h DESTINATION include)
