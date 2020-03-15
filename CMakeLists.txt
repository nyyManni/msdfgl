
SET(MAJOR_VERSION 0)
SET(MINOR_VERSION 2)
SET(PATCH_VERSION 1)
SET(MSDFGL_DESCRIPTION "OpenGL MSDF Generator and renderer")

cmake_minimum_required (VERSION 3.9)
project (msdfgl VERSION ${MAJOR_VERSION}.${MINOR_VERSION}
         DESCRIPTION "${MSDFGL_DESCRIPTION}")

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


set(MSDFGL_VERSION "${MAJOR_VERSION}.${MINOR_VERSION}")
configure_file(include/msdfgl.h.in msdfgl.h @ONLY)

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

echo '#ifndef _MSDF_SHADERS_H' >\$OUTPUT
echo '#define _MSDF_SHADERS_H' >>\$OUTPUT
for header; do
        fname=\${header##*/};
        echo '' >>\$OUTPUT
        echo 'const char * _'\${fname\%\%.glsl}' =' >>\$OUTPUT
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
  add_library(msdfglstatic src/msdfgl.c src/msdfgl_serializer.c src/msdfgl_map.c example/glad/src/glad.c ${SHADER_HEADER})
  add_library(msdfglshared SHARED src/msdfgl.c src/msdfgl_serializer.c src/msdfgl_map.c example/glad/src/glad.c ${SHADER_HEADER})
  set_target_properties(msdfglshared PROPERTIES OUTPUT_NAME msdfgl)
  set_target_properties(msdfglstatic PROPERTIES OUTPUT_NAME msdfgl)
  target_compile_definitions(msdfglshared PRIVATE MSDFGL_EXPORTS=1)

else()
  add_library(msdfglstatic STATIC src/msdfgl.c src/msdfgl_serializer.c src/msdfgl_map.c ${SHADER_HEADER})
  add_library(msdfglshared SHARED src/msdfgl.c src/msdfgl_serializer.c src/msdfgl_map.c ${SHADER_HEADER})
  set_target_properties(msdfglstatic PROPERTIES OUTPUT_NAME msdfgl)
  set_target_properties(msdfglshared PROPERTIES OUTPUT_NAME msdfgl)
endif()

if (APPLE)

  # We make it the library user's responsibility to link against some OpenGL.
  set_target_properties(msdfglshared PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
  add_definitions(-DGL_SILENCE_DEPRECATION)
endif (APPLE)

if (MSVC)
  #target_compile_options(msdfgl PRIVATE /W4 /WX)
else()
  target_compile_options(msdfglshared PRIVATE -Wall -Wextra -pedantic -Werror -Wno-overlength-strings -Wno-unused-parameter -Wno-unused-variable)
  target_compile_options(msdfglstatic PRIVATE -Wall -Wextra -pedantic -Werror -Wno-overlength-strings -Wno-unused-parameter -Wno-unused-variable)
endif()

target_include_directories (msdfglshared PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories (msdfglstatic PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(msdfglshared ${FREETYPE_LIBRARIES})
target_link_libraries(msdfglstatic ${FREETYPE_LIBRARIES})

if (MSVC)
  include_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib/glfw3/include)
else ()
  find_package(glfw3)
endif ()

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/example/glad/include)
add_executable(msdfgldemo EXCLUDE_FROM_ALL example/demo.c example/glad/src/glad.c)
if (MSVC)
  target_link_libraries(msdfgldemo msdfglstatic ${OPENGL_LIBRARIES} ${FREETYPE_LIBRARIES} ${GLFW3_LIB})
else()
  target_link_libraries(msdfgldemo msdfglshared ${OPENGL_LIBRARIES} ${FREETYPE_LIBRARIES} glfw dl)
endif()

if (WIN32)
  install(TARGETS msdfglstatic DESTINATION lib COMPONENT libraries)
  install (FILES ${CMAKE_CURRENT_BINARY_DIR}/msdfgl.h DESTINATION include COMPONENT headers)
else ()
  install(TARGETS msdfglshared msdfglstatic DESTINATION lib)
  install (FILES ${CMAKE_CURRENT_BINARY_DIR}/msdfgl.h DESTINATION include)
endif (WIN32)

if (WIN32)
  set (CPACK_PACKAGE_NAME "msdfgl")
  set (CPACK_PACKAGE_VENDOR "Henrik Nyman")

  SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${MSDFGL_DESCRIPTION}")
  SET(CPACK_PACKAGE_VENDOR "Henrik Nyman")
  SET(CPACK_PACKAGE_CONTACT "h@nyymanni.com")
  SET(CPACK_PACKAGE_VERSION_MAJOR "${MAJOR_VERSION}")
  SET(CPACK_PACKAGE_VERSION_MINOR "${MINOR_VERSION}")

  set (CPACK_PACKAGE_INSTALL_DIRECTORY "msdfgl")

  set (CPACK_COMPONENTS_ALL libraries headers)
  set (CPACK_COMPONENT_LIBRARIES_DISPLAY_NAME "Libraries")
  set (CPACK_COMPONENT_HEADERS_DISPLAY_NAME "C Headers")

  set (CPACK_COMPONENT_LIBRARIES_DESCRIPTION
    "MSDFGL static library")
  set (CPACK_COMPONENT_HEADERS_DESCRIPTION
    "C/C++ header files for use with MSDFGL")

  set (CPACK_COMPONENT_HEADERS_DEPENDS libraries)

  set(CPACK_COMPONENT_LIBRARIES_GROUP "")
  set(CPACK_COMPONENT_HEADERS_GROUP "Development")

  set(CPACK_COMPONENT_GROUP_DEVELOPMENT_DESCRIPTION
    "MSDFGL library")

  set(CPACK_ALL_INSTALL_TYPES Full)
  set(CPACK_COMPONENT_LIBRARIES_INSTALL_TYPES Full)
  set(CPACK_COMPONENT_HEADERS_INSTALL_TYPES Full)

  include(CPack)
else ()
  if(EXISTS "${CMAKE_ROOT}/Modules/CPack.cmake")
    INCLUDE(InstallRequiredSystemLibraries)

    SET(CPACK_SET_DESTDIR "on")
    SET(CPACK_PACKAGING_INSTALL_PREFIX "/tmp")
    SET(CPACK_GENERATOR "DEB")

    SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${MSDFGL_DESCRIPTION}")
    SET(CPACK_PACKAGE_VENDOR "Henrik Nyman")
    SET(CPACK_PACKAGE_CONTACT "h@nyymanni.com")
    SET(CPACK_PACKAGE_VERSION_MAJOR "${MAJOR_VERSION}")
    SET(CPACK_PACKAGE_VERSION_MINOR "${MINOR_VERSION}")
    SET(CPACK_PACKAGE_VERSION_PATCH "${PATCH_VERSION}")
    SET(CPACK_PACKAGE_NAME "lib${CMAKE_PROJECT_NAME}-dev")
    SET(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}_${MAJOR_VERSION}.${MINOR_VERSION}-${CPACK_PACKAGE_VERSION_PATCH}")
    SET(CPACK_SOURCE_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}_${MAJOR_VERSION}.${MINOR_VERSION}-${CPACK_PACKAGE_VERSION_PATCH}")

    SET(CPACK_DEBIAN_PACKAGE_DEPENDS "")
    SET(CPACK_DEBIAN_PACKAGE_SUGGESTS "")

    SET(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
    SET(CPACK_DEBIAN_PACKAGE_VERSION "${MAJOR_VERSION}.${MINOR_VERSION}-${CPACK_PACKAGE_VERSION_PATCH}")
    SET(CPACK_DEBIAN_PACKAGE_SECTION "devel")
    SET(CPACK_DEBIAN_ARCHITECTURE ${CMAKE_SYSTEM_PROCESSOR})

    SET(CPACK_COMPONENTS_ALL Libraries ApplicationData)
    INCLUDE(CPack)

  endif(EXISTS "${CMAKE_ROOT}/Modules/CPack.cmake")

endif(WIN32)
