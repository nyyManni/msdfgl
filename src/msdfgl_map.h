#ifndef MSDFGL_MAP_H
#define MSDFGL_MAP_H

/**
 * A simple mapping container for glyph metadata.
 *
 * Items from 0 to 255 are stored statically, others are allocated dynamically.
 */

#include <stdlib.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#define MSDFGL_MAP_LINEAR_SIZE 256
#define MSDFGL_MAP_DYNAMIC_INITIAL_SIZE 256

typedef struct _msdfgl_map_item {
    FT_ULong code;
    int index;
    float advance[2];
} msdfgl_map_item_t;

typedef struct _msdfgl_map {
    msdfgl_map_item_t linear_map[MSDFGL_MAP_LINEAR_SIZE];
    msdfgl_map_item_t *dynamic_map;

    int dynamic_size;
    int dynamic_alloc;
} msdfgl_map_t;

void msdfgl_map_init(msdfgl_map_t *map);

msdfgl_map_item_t *msdfgl_map_get(msdfgl_map_t *map, FT_ULong code);

int msdfgl_map_in(msdfgl_map_t *map, FT_ULong code);

msdfgl_map_item_t *msdfgl_map_insert(msdfgl_map_t *map, FT_ULong code);

void msdfgl_map_destroy(msdfgl_map_t *map);

#endif /* MSDFGL_MAP_H */
