configure_file(../include/msdfgl.h.in msdfgl.h @ONLY)

# Finds all .glsl shaders in our source directory.
file(GLOB_RECURSE  _temp_msdfgl_shaders ${CMAKE_CURRENT_SOURCE_DIR}/*.glsl)
# Creates a C-style string for each shader source.
foreach(_temp_msdfgl_shader IN LISTS _temp_msdfgl_shaders)
    # Gets the shader file name, without the extension.
    get_filename_component(_temp_shader_name ${_temp_msdfgl_shader} NAME_WE)
    # Reads the contents of the GLSL shader.
    file(READ ${_temp_msdfgl_shader} _temp_shader_contents)
    string(REGEX REPLACE "\n$" "" _temp_shader_contents "${_temp_shader_contents}")
    # Escapes backslash characters from source glsl.
    string(REPLACE "\\" "\\\\" _temp_shader_contents "${_temp_shader_contents}")
    # Add quotes around each line in the shader.
    string(REPLACE "\n" "\\n\"\n\"" _temp_shader_contents "${_temp_shader_contents}")
    set(_temp_msdfgl_shaders_sources "${_temp_msdfgl_shaders_sources}\n\nconst char * _${_temp_shader_name} = \"${_temp_shader_contents}\";")
endforeach()
# Write a header guard around our GLSL shader sources.
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/_msdfgl_shaders.h "#ifndef _MSDFGL_SHADERS_H
#define _MSDFGL_SHADERS_H${_temp_msdfgl_shaders_sources}\n
#endif /* _MSDFGL_SHADERS_H */")

find_package(Freetype 2.9.1 QUIET)
if(NOT FREETYPE_FOUND)
    include(FetchContent)
    FetchContent_Declare(freetype
            URL https://download.savannah.gnu.org/releases/freetype/freetype-2.10.1.tar.gz
            URL_MD5 c50a3c9e5e62bdc938a6e1598a782947
            )
    FetchContent_GetProperties(freetype)
    if(NOT freetype_POPULATED)
        foreach(_freetype_option "SKIP_INSTALL_HEADERS" "SKIP_INSTALL_LIBRARIES" 
                "CMAKE_DISABLE_FIND_PACKAGE_PNG" "CMAKE_DISABLE_FIND_PACKAGE_HarfBuzz" "CMAKE_DISABLE_FIND_PACKAGE_BZip2"
                "CMAKE_DISABLE_FIND_PACKAGE_ZLIB" "CMAKE_DISABLE_FIND_PACKAGE_BROTLI"
                )
            set(${_freetype_option} TRUE CACHE INTERNAL "")
        endforeach()
        FetchContent_Populate(freetype)
        add_subdirectory(${freetype_SOURCE_DIR} ${freetype_BINARY_DIR})
    endif()
	set(FREETYPE_LIBRARIES "freetype")
endif()

add_library(msdfgl ../include/msdfgl.h msdfgl.c msdfgl_serializer.c msdfgl_map.c)
if(BUILD_SHARED_LIBS)
    target_compile_definitions(msdfgl PRIVATE MSDFGL_EXPORTS)
else()
    target_compile_definitions(msdfgl PUBLIC MSDFGL_STATIC_DEFINE)
endif()
target_include_directories(msdfgl PUBLIC 
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        ${FREETYPE_INCLUDE_DIRS}
        )
target_link_libraries(msdfgl PUBLIC glad ${FREETYPE_LIBRARIES} ${CMAKE_DL_LIBS})

target_compile_features(msdfgl PUBLIC c_std_11)
if(MSVC)
    #target_compile_options(msdfgl PRIVATE /W4 /WX)
else()
    target_compile_options(msdfgl PRIVATE -Wall -Wextra -pedantic -Werror -Wno-overlength-strings -Wno-unused-parameter -Wno-unused-variable)
endif()

if(MSDFGL_INSTALL)
    install(TARGETS msdfgl
            LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
            ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
            RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
            )
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/msdfgl.h
            DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
            )
endif()

# TODO: Modernize CPack.
if(WIN32)
    set(CPACK_PACKAGE_NAME "msdfgl")
    set(CPACK_PACKAGE_VENDOR "Henrik Nyman")
    
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${MSDFGL_DESCRIPTION}")
    set(CPACK_PACKAGE_VENDOR "Henrik Nyman")
    set(CPACK_PACKAGE_CONTACT "h@nyymanni.com")
    set(CPACK_PACKAGE_VERSION_MAJOR "${MAJOR_VERSION}")
    set(CPACK_PACKAGE_VERSION_MINOR "${MINOR_VERSION}")
    
    set(CPACK_PACKAGE_INSTALL_DIRECTORY "msdfgl")
    
    set(CPACK_COMPONENTS_ALL libraries headers)
    set(CPACK_COMPONENT_LIBRARIES_DISPLAY_NAME "Libraries")
    set(CPACK_COMPONENT_HEADERS_DISPLAY_NAME "C Headers")
    
    set(CPACK_COMPONENT_LIBRARIES_DESCRIPTION "MSDFGL static library")
    set(CPACK_COMPONENT_HEADERS_DESCRIPTION "C/C++ header files for use with MSDFGL")
    
    set(CPACK_COMPONENT_HEADERS_DEPENDS libraries)
    
    set(CPACK_COMPONENT_LIBRARIES_GROUP "")
    set(CPACK_COMPONENT_HEADERS_GROUP "Development")
    
    set(CPACK_COMPONENT_GROUP_DEVELOPMENT_DESCRIPTION "MSDFGL library")
    
    set(CPACK_ALL_INSTALL_TYPES Full)
    set(CPACK_COMPONENT_LIBRARIES_INSTALL_TYPES Full)
    set(CPACK_COMPONENT_HEADERS_INSTALL_TYPES Full)
   
    include(CPack)
else()
    if(EXISTS "${CMAKE_ROOT}/Modules/CPack.cmake")
        include(InstallRequiredSystemLibraries)
        
        set(CPACK_SET_DESTDIR "on")
        set(CPACK_PACKAGING_INSTALL_PREFIX "/tmp")
        set(CPACK_GENERATOR "DEB")
        
        set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${MSDFGL_DESCRIPTION}")
        set(CPACK_PACKAGE_VENDOR "Henrik Nyman")
        set(CPACK_PACKAGE_CONTACT "h@nyymanni.com")
        set(CPACK_PACKAGE_VERSION_MAJOR "${msdfgl_MAJOR_VERSION}")
        set(CPACK_PACKAGE_VERSION_MINOR "${msdfgl_MINOR_VERSION}")
        set(CPACK_PACKAGE_VERSION_PATCH "${msdfgl_PATCH_VERSION}")
        set(CPACK_PACKAGE_NAME "lib${CMAKE_PROJECT_NAME}-dev")
        set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}_${msdfgl_MAJOR_VERSION}.${msdfgl_MINOR_VERSION}-${CPACK_PACKAGE_VERSION_PATCH}")
        set(CPACK_SOURCE_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}_${msdfgl_MAJOR_VERSION}.${msdfgl_MINOR_VERSION}-${CPACK_PACKAGE_VERSION_PATCH}")
        
        set(CPACK_DEBIAN_PACKAGE_DEPENDS "")
        set(CPACK_DEBIAN_PACKAGE_SUGGESTS "")
        
        set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
        set(CPACK_DEBIAN_PACKAGE_VERSION "${msdfgl_MAJOR_VERSION}.${msdfgl_MINOR_VERSION}-${CPACK_PACKAGE_VERSION_PATCH}")
        set(CPACK_DEBIAN_PACKAGE_SECTION "devel")
        set(CPACK_DEBIAN_ARCHITECTURE ${CMAKE_SYSTEM_PROCESSOR})
        
        set(CPACK_COMPONENTS_ALL Libraries ApplicationData)

        include(CPack)
    endif()
endif()
