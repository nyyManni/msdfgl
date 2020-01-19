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

#if defined(MSDFGL_STATIC_DEFINE)
#  define MSDFGL_EXPORT
#  define MSDFGL_NO_EXPORT
#else
#  ifdef _MSC_VER
#    ifdef MSDFGL_EXPORTS
#      define MSDFGL_EXPORT __declspec(dllexport)
#    else
#      define MSDFGL_EXPORT __declspec(dllimport)
#    endif
#  else
#    define MSDFGL_EXPORT __attribute__((visibility("default")))
#  endif
#endif

#ifdef _WIN32
#include <stdint.h>
#include <windows.h>
#endif

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_OUTLINE_H

#ifdef __APPLE__
#include <OpenGL/gl3.h>
#else
#include <GL/gl.h>
#endif

#define MSDFGL_VERSION "@msdfgl_VERSION@"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _msdfgl_context *msdfgl_context_t;
typedef struct _msdfgl_font *msdfgl_font_t;
typedef struct _msdfgl_atlas *msdfgl_atlas_t;

/**
 * Compile shaders and configure uniforms.
 *
 * version-parameter defines with which GLSL version the shaders will be
 * compiled. Supported versions are "330", "330 core" and "320 es".
 * giving NULL uses the default ("330 core").
 *
 * Returns a new MSDF GL context, or NULL if creating the context failed.
 */
MSDFGL_EXPORT msdfgl_context_t msdfgl_create_context(const char *version);

/**
 * Release resources allocated by `msdfgl_crate_context`.
 */
MSDFGL_EXPORT void msdfgl_destroy_context(msdfgl_context_t ctx);

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
     * negative leans to the left. Skew can create /italics/ effect without
     * loading a separate font atlas.
     */
    GLfloat skew;

    /**
     * The "boldness" of the character. 0.5 is normal strength, lower is thinner
     * and higher is thicker. Strength can create *bold* effect without loading
     * a separate font atlas.
     */
    GLfloat strength;

} msdfgl_glyph_t;

/**
 * Allocate textures for a font atlas.
 */
MSDFGL_EXPORT msdfgl_atlas_t msdfgl_create_atlas(msdfgl_context_t ctx, int texture_width,
                                                 int padding);

/**
 * Release resources of a font atlas.
 */
MSDFGL_EXPORT void msdfgl_destroy_atlas(msdfgl_atlas_t atlas);

/**
 * Load font from a font file and generate textures and buffers for it.
 */
MSDFGL_EXPORT msdfgl_font_t msdfgl_load_font(msdfgl_context_t ctx, const char *font_name,
                                             float range, float scale,
                                             msdfgl_atlas_t atlas);

/**
 * Get vertical advance of the font with the given size.
 */
MSDFGL_EXPORT float msdfgl_vertical_advance(msdfgl_font_t font, float size);

/**
 * Release resources allocated by `msdfgl_load_font`.
 */
MSDFGL_EXPORT void msdfgl_destroy_font(msdfgl_font_t font);

/**
 * Render a single glyph onto the MSDF atlas. Intented use case is to generate
 * the bitmaps on-demand as the characters are appearing.
 * Parameter _user is defined just to make this function compatible as a missing
 * glyph callback. It will not be used by the function.
 */
MSDFGL_EXPORT int msdfgl_generate_glyph(msdfgl_font_t font, int32_t char_code, void *_user);

/**
 * Render a range of glyphs onto the MSDF atlas. The range is inclusive. Intended
 * use case is to initialize the atlas in the beginning with e.g. ASCII characters.
 */
MSDFGL_EXPORT int msdfgl_generate_glyphs(msdfgl_font_t font, int32_t start, int32_t end);

/**
 * Render arbitrary character codes in bulk.
 */
MSDFGL_EXPORT int msdfgl_generate_glyph_list(msdfgl_font_t font, int32_t *list, size_t n);

/**
 * Shortcuts for common generators.
 */
#define msdfgl_generate_ascii(font) msdfgl_generate_glyphs(font, 0, 127)
#define msdfgl_generate_ascii_ext(font) msdfgl_generate_glyphs(font, 0, 255)

/**
 * Render a list of glyphs.
 */
MSDFGL_EXPORT void msdfgl_render(msdfgl_font_t font, msdfgl_glyph_t *glyphs, int n,
                                 GLfloat *projection);

/**
 * Printf options.
 */
enum msdfgl_printf_flags {

    /**
     * Use FreeType kerning if it is available for the font.
     */
    MSDFGL_KERNING = 0x01,

    /**
     * Render wide character arrays. Give the fmt-argment as wchar_t * in UTF-32.
     *
     * NOTE: due to limitations in wchar string operations there is an upper
     *       limit of 255 characters that can be printed in one call. If more
     *       characters are to be printed at once use UTF-8 instead.
     */
    MSDFGL_WCHAR = 0x02,

    /**
     * Draw text vertically instead of horizontally.
     */
    MSDFGL_VERTICAL = 0x04,

    /**
     * Parse the text as an UTF-8 string.
     *
     * NOTE: msdfgl does not validate the string in any way. The caller is
     *       responsible for validating the untrusted string before passing it
     *       to msdfgl.
     */
    MSDFGL_UTF8 = 0x08,
};

/**
 * Print a formatted string on currently active framebuffer.
 *
 * x - x coordinate of the starting glyph.
 * y - y coordinate of the starting glyph (note that this points to the origin
 *     of the glyph, which is usually at bottom left).
 * font - font to use for rendering.
 * size - font size in points (fractional sizes are allowed).
 * color - color of the text (0xrrggbbaa).
 * projection - 4x4 projection matrix.
 * flags - drawing options, see `enum msdfgl_printf_flags`
 * fmt - char * (or wchar_t *) format string.
 * ... - substitution variables for the format string.
 *
 * Returns the x (y if vertical drawing is enabled) position of the glyph that
 * would follow the rendered ones.
 */
MSDFGL_EXPORT float msdfgl_printf(float x, float y, msdfgl_font_t font, float size,
                                  int32_t color, GLfloat *projection,
                                  enum msdfgl_printf_flags flags, const void *fmt, ...);


/**
 * Calculate the width or height of the text.
 *
 * x - pointer to x coordinate of the starting glyph will be incremented by
 *     the call.
 * y - pointer to y coordinate of the starting glyph will be incremented by
 *     the call (note that this points to the origin of the glyph, which is
 *     usually at bottom left).
 * font - font to use for rendering.
 * size - font size in points (fractional sizes are allowed).
 * flags - drawing options, see `enum msdfgl_printf_flags`
 * fmt - char * (or wchar_t *) format string.
 * ... - substitution variables for the format string.
 *
 * Does not render the text - just calculates how much the cursor would move if
 * the text were to be rendered. This does not mean the call would not hit GPU,
 * as `msdfgl_geometry` will trigger missing_glyph_callback if one is set.
 *
 * Also note, that only the coordinate to the direction of text flow is updated.
 * E.g. with MSDGL_VERTICAL set only the y-coordinate is moved - and vice versa.
 */
MSDFGL_EXPORT void msdfgl_geometry(float *x, float *y, msdfgl_font_t font, float size,
                                   enum msdfgl_printf_flags flags, const void *fmt, ...);


/**
 * Handle undefined glyphs during `msdfgl_printf`. The callback gets called with
 * the active font as the first argument and UTF-32 codepoint of the undefined
 * character as the second argument. After the function returns `msdfgl_printf`
 * will attempt the lookup of the character again, if the callback returned
 * a non-zero value.
 *
 * If there is no context-switch or other maintenance required before/after
 * generation, auto-generating glyphs can be acquired simply by calling:
 *
 * `msdfgl_set_missing_glyph_callback(<context>, msdfgl_generate_glyph, <user data>)`
 *
 * <user data> is the same pointer which was given when setting up the callback.
 */
MSDFGL_EXPORT void msdfgl_set_missing_glyph_callback(msdfgl_context_t,
                                                     int (*)(msdfgl_font_t, int32_t, void *),
                                                     void *);

/**
 * Set the DPI for the current session. Following draw calls will use the new DPI.
 * The DPI value is a vector, which allows for rendering text with a monitor which
 * has non-square pixels.
 * The default DPI is (72, 72).
 */
MSDFGL_EXPORT void msdfgl_set_dpi(msdfgl_context_t context, float horizontal,
                                  float vertical);

/* Plumbing commands. In case you want to build your own renderer. */
/**
 * Generates an orthographic projection (similar to glm's ortho).
 */
MSDFGL_EXPORT void _msdfgl_ortho(GLfloat left, GLfloat right, GLfloat bottom, GLfloat top,
                                 GLfloat nearVal, GLfloat farVal, GLfloat dest[][4]);

/**
 * Get the atlas texture of the given font.
 */
MSDFGL_EXPORT GLuint _msdfgl_atlas_texture(msdfgl_font_t font);
/**
 * Get the index texture of the given font.
 */
MSDFGL_EXPORT GLuint _msdfgl_index_texture(msdfgl_font_t font);
#ifdef __cplusplus
}
#endif

#endif /* MSDFGL_H */
