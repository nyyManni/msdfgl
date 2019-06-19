#include "msdfgl_map.h"


void msdfgl_map_init(msdfgl_map_t *map) {
    map->dynamic_map = NULL;
    map->dynamic_size = 0;
    map->dynamic_alloc = 0;

    for (int i = 0; i < MSDFGL_MAP_LINEAR_SIZE; ++i)
        map->linear_map[i].index = -1;
}

int __cmp(const void *a, const void *b) {
    return ((msdfgl_map_item_t *)a)->code - ((msdfgl_map_item_t *)b)->code;
}

msdfgl_map_item_t *msdfgl_map_get(msdfgl_map_t *map, FT_ULong code) {
    if (code < MSDFGL_MAP_LINEAR_SIZE)
        return map->linear_map[code].index != -1 ? &map->linear_map[code] : NULL;

    if (!map->dynamic_map)
        return NULL;

    msdfgl_map_item_t i;
    i.code = code;
    
    return bsearch(&i, map->dynamic_map, map->dynamic_size, sizeof(msdfgl_map_item_t),
                   __cmp);
}

int msdfgl_map_in(msdfgl_map_t *map, FT_ULong code) {
    return msdfgl_map_get(map, code) != NULL;
}

msdfgl_map_item_t *msdfgl_map_insert(msdfgl_map_t *map, FT_ULong code) {
    if (code < MSDFGL_MAP_LINEAR_SIZE) {
        map->linear_map[code].code = code;
        return &map->linear_map[code];
    }

    /* Reallocate a new array if there is no space left */
    if (map->dynamic_size + 1 > map->dynamic_alloc) {
        int new_size = map->dynamic_alloc ? (map->dynamic_alloc * 2)
                                          : MSDFGL_MAP_DYNAMIC_INITIAL_SIZE;

        msdfgl_map_item_t *new = calloc(new_size, sizeof(msdfgl_map_item_t));
        if (!new)
            return NULL;

        if (map->dynamic_map) {
            memcpy(new, map->dynamic_map, map->dynamic_size);
            free(map->dynamic_map);
        }
        map->dynamic_map = new;
        map->dynamic_alloc = new_size;
    }
    int i = 0;
    while (i < map->dynamic_size && map->dynamic_map[i].code < code) ++i;
    
    /* Shift the end of the array */
    if (i != map->dynamic_size)
        memmove(&map->dynamic_map[i + 1], &map->dynamic_map[i],
                (map->dynamic_size - i) * sizeof(msdfgl_map_item_t));
    
    /* Place in the new item. */
    map->dynamic_map[i].code = code;
    ++map->dynamic_size;
    return &map->dynamic_map[i];
}

void msdfgl_map_destroy(msdfgl_map_t *map) {
    if (map->dynamic_map)
        free(map->dynamic_map);
}


