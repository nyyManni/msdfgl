# MSDFGL - OpenGL implementation of the multi-channel distance-field algorithm.

The MSDF algorithm implemented in this library is created by Viktor Chlumský (https://github.com/Chlumsky/msdfgen). The full credit of the algorithm goes to him, although none of the original code can be found from this repository.

Details about the implementation and the benefits of MSDF can be read from his repository or his thesis, I'm not going to repeat them here.

The code had to go through quite a bit of modifications to make it runnable on the GPU:
1. It's no longer object-oriented, as the algorithm was rewritten in C (not supported in GLSL)
2. It now runs with constant memory (as there is no dynamic allocation support in GLSL)
3. It does not use pointers (not supported in GLSL either)
4. Dropped support for cubic segments (for simplicity's sake, this will probably be added later)

## Implementation
The highly parallelizable part of MSDF algorithm has been moved to run on the GPU (the part of msdfgen which is executed per each pixel of the bitmap).

A loaded msdfgl font has two textures:
- Atlas texture - 2D RGBA texture containing all the generated MSDF bitmaps
- Index texture - 1D FLOAT texture buffer containing the coordinates and dimensions of each glyph on the atlas texture (there is also information about the bearing of the glyph so that we do not have to store the bitmap all the way from the origin, only from where the glyph actually starts).

![Implementation](img/diagram.png)

## Usage:
```C
#include <msdfgl.h>

msdfgl_context_t context;
context = msdfgl_create_context();

msdfgl_font_t font;
font = msdfgl_load_font(context, "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf",
                        4.0, 2.0, 0); /* range, scale, texture_width (defaults to max available) */

/* Loads characters 0-128 onto the textures. This is where all the GPU cycles went. */
msdfgl_generate_ascii(font);

/*                  4x4 projection-matrix  color       size */
msdfgl_printf(font, (GLfloat *)projection, 0xffffffff, 18, "Hello, MSFDGL!");

/* Cleanup */
msdfgl_destroy_font(font);
msdfgl_destroy_context(context);
```

The library includes two shaders:
- Generator shader - heavy lifting, generates the MSDF bitmaps.
- Render shader - renders crisp text from the generated textures.


## TODO:
- Implement `msdfgl_printf`.
- There are strange artifacs all over the bitmaps, need to figure out where do they come from.
- Implement edge-coloring in C (so that we do not have to link against a custom version of libmsdfgen).
- Cross-platform compilation, cross-OpenGL shaders.
