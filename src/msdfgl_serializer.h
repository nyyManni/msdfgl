
#ifndef MSDF_SERIALIZER_H
#define MSDF_SERIALIZER_H

#include "msdfgl.h"

#define SERIALIZER_SCALE 64.0

int msdfgl_glyph_buffer_size(FT_Face face, int code, size_t *meta_size,
                             size_t *point_size);

int msdfgl_serialize_glyph(FT_Face face, int code, char *meta_buffer,
                           GLfloat *point_buffer, GLfloat *width, GLfloat *height,
                           GLfloat *bearing_x, GLfloat *bearing_y, GLfloat *advance);

#endif
