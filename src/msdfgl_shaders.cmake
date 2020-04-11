
# Finds all .glsl shaders in our source directory.
file(GLOB_RECURSE  _temp_msdfgl_shaders ${SHADER_DIR}/*.glsl)
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
file(WRITE ${TARGET_DIR}/_msdfgl_shaders.h "#ifndef _MSDFGL_SHADERS_H
#define _MSDFGL_SHADERS_H${_temp_msdfgl_shaders_sources}\n
#endif /* _MSDFGL_SHADERS_H */
")
