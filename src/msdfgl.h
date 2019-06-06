#ifndef MSDFGL_H
#define MSDFGL_H

/**
 * OpenGL implementation for multi-channel signed distance field generator
 * -----------------------------------------------------------------------
 *
 * msdfgl              Henrik Nyman,    (c) 2019 -
 * msdfgen             Viktor Chlumsky, (c) 2014 - 2019
 *
 * The technique used to generate multi-channel distance fields in this code has
 * been developed by Viktor Chlumsky in 2014 for his master's thesis, "Shape
 * Decomposition for Multi-Channel Distance Fields". It provides improved
 * quality of sharp corners in glyphs and other 2D shapes in comparison to
 * monochrome distance fields. To reconstruct an image of the shape, apply the
 * median of three operation on the triplet of sampled distance field values.
 *
 * MSDFGL provides a version of that algorithm that runs partially on the GPU,
 * and a higher-level API handling font atlases and textures.
 */
#include <stdarg.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#include <GL/gl.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _msdfgl_context *msdfgl_context_t;
struct msdfgl_map_t;

typedef struct _map_elem {
    int key;
    int index;
    double horizontal_advance;
} map_elem_t;

typedef struct _msdfgl_elem_list {
    struct _msdfgl_elem_list *next;
    map_elem_t data[];
} msdfgl_elem_list_t;

typedef struct _msdfgl_map {
    void *root;
    size_t chunk_size;

    size_t i;
    msdfgl_elem_list_t *cur_list;
    msdfgl_elem_list_t *elems;
} msdfgl_map_t;

/**
 * Compile shaders and configure uniforms.
 *
 * Returns a new MSDF GL context, or NULL if creating the context failed.
 */
msdfgl_context_t msdfgl_create_context();

/**
 * Release resources allocated by `msdfgl_crate_context`.
 */
void msdfgl_destroy_context(msdfgl_context_t ctx);

struct _msdfgl_font {
    char *font_name;

    double scale;
    double range;
    int texture_width;

    double vertical_advance;
    float horizontal_advances[256];

    msdfgl_map_t character_index;

    GLfloat atlas_projection[4][4];

    /**
     * 2D RGBA atlas texture containing all MSDF-glyph bitmaps.
     */
    GLuint atlas_texture;
    GLuint _atlas_framebuffer;

    /**
     * 1D buffer containing glyph position information per character in the
     * atlas texture.
     */
    GLuint index_texture;
    GLuint _index_buffer;

    /**
     * Amount of glyphs currently rendered on the textures.
     */
    size_t _nglyphs;

    /**
     * The current size of the buffer index texture.
     */
    size_t _nallocated;

    /**
     * The amount of allocated texture height.
     */
    int _texture_height;

    /**
     * MSDFGL context handle.
     */
    msdfgl_context_t context;

    /**
     * FreeType Face handle.
     */
    FT_Face face;

    /**
     * The location in the atlas where the next bitmap would be rendered.
     */
    size_t _offset_y;
    size_t _offset_x;
    size_t _y_increment;

    /**
     * Amount of pixels to leave blank between MSDF bitmaps.
     */
    int atlas_padding;

    /**
     * Texture buffer objects for serialized FreeType data input.
     */
    GLuint _meta_input_buffer;
    GLuint _point_input_buffer;
    GLuint _meta_input_texture;
    GLuint _point_input_texture;

    int _direct_lookup_upper_limit;
};
typedef struct _msdfgl_font *msdfgl_font_t;

/**
 * Serialized format of a glyph.
 */
typedef struct _msdfgl_glyph {
    /**
     * X and Y coordinates in in the projection coordinates.
     */
    GLfloat x;
    GLfloat y;

    /**
     * The color of the character in 0xRRGGBBAA format.
     */
    GLuint color;

    /**
     * Unicode code point of the character.
     */
    GLint key;

    /**
     * Font size to use for rendering of this character.
     */
    GLfloat size;

    /**
     * Y offset (for e.g. subscripts and superscripts).
     */
    GLfloat offset;

    /**
     * The amount of "lean" on the character. Positive leans to the right,
     * negative leans to the left.
     */
    GLfloat skew;

    /**
     * The "boldness" of the character. 0.5 is normal strength, lower is thinner
     * and higher is thicker.
     */
    GLfloat strength;

} msdfgl_glyph_t;

/**
 * Load font from a font file and generate textures and buffers for it.
 */
msdfgl_font_t msdfgl_load_font(msdfgl_context_t ctx, const char *font_name, double range,
                               double scale, int texture_size);

/**
 * Release resources allocated by `msdfgl_load_font`.
 */
void msdfgl_destroy_font(msdfgl_font_t font);

/**
 * Render a single glyph onto the MSFD atlas. Intented use case is to generate
 * the bitmaps on-demand as the characters are appearing.
 */
int msdfgl_generate_glyph(msdfgl_font_t font, int32_t char_code);

/**
 * Render a range of glyphs onto the MSFD atlas. The range is inclusive. Intended
 * use case is to initialize the atlas in the beginning with e.g. ASCII characters.
 */
int msdfgl_generate_glyphs(msdfgl_font_t font, int32_t start, int32_t end);

/**
 * Render arbitrary character codes in bulk.
 */
int msdfgl_generate_glyph_list(msdfgl_font_t font, int32_t *list, size_t n);

/**
 * Shortcuts for common generators.
 */
#define msdfgl_generate_ascii(font) msdfgl_generate_glyphs(font, 0, 128)
#define msdfgl_generate_ascii_ext(font) msdfgl_generate_glyphs(font, 0, 256)

/**
 * Render a list of glyphs.
 */
void msdfgl_render(msdfgl_font_t font, msdfgl_glyph_t *glyphs, int n,
                   GLfloat *projection);

float msdfgl_printf(float x, float y, msdfgl_font_t font, float size, int32_t color,
                    GLfloat *projection, const char *fmt, ...);

#ifdef __cplusplus
}
#endif

#endif /* MSDFGL_H */
