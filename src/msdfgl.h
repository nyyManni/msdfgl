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
 * Get vertical advance of the font with the given size.
 */
float msdfgl_vertical_advance(msdfgl_font_t font, float size);

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

/* Plumbing commands. */
/**
 * Get the atlas texture of the given font.
 */
GLuint _msdfgl_atlas_texture(msdfgl_font_t font);
#ifdef __cplusplus
}
#endif

#endif /* MSDFGL_H */
