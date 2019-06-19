
#ifndef MSDFGL_SERIALIZER_H
#define MSDFGL_SERIALIZER_H

#include "msdfgl.h"

#define SERIALIZER_SCALE 64.0f

int msdfgl_glyph_buffer_size(FT_Face face, int code, size_t *meta_size,
                             size_t *point_size);

int msdfgl_serialize_glyph(FT_Face face, int code, char *meta_buffer,
                           GLfloat *point_buffer);

#endif  /* MSDFGL_SERIALIZER_H */
