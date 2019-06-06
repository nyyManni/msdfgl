
#ifndef MSDF_SERIALIZER_H
#define MSDF_SERIALIZER_H

#include <ft2build.h>
#include FT_FREETYPE_H
#include <GL/gl.h>

int msdfgl_glyph_buffer_size(FT_Face face, int code, size_t *meta_size,
                             size_t *point_size);

int msdfgl_serialize_glyph(FT_Face face, int code, char *meta_buffer,
                           GLfloat *point_buffer, GLfloat *width, GLfloat *height,
                           GLfloat *bearing_x, GLfloat *bearing_y, GLfloat *advance);

#endif
