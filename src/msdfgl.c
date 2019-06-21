#include <locale.h>
#include <wchar.h>

#if defined(_MSC_VER)
#include <BaseTsd.h>
typedef SSIZE_T ssize_t;
#include <glad/glad.h>
#endif

#include <ft2build.h>
#include FT_FREETYPE_H

#ifdef __linux__

/* We don't want to link to any specific OpenGL implementation. */
#define GL_GLEXT_PROTOTYPES
#else
/* Figure out something. */
#endif

#include "msdfgl.h"
#include "msdfgl_map.h"
#include "msdfgl_serializer.h"

#include "_msdfgl_shaders.h" /* Auto-generated */

struct _msdfgl_font {
    char *font_name;

    float scale;
    float range;
    int texture_width;

    float vertical_advance;

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

typedef struct msdfgl_index_entry {
    GLfloat offset_x;
    GLfloat offset_y;
    GLfloat size_x;
    GLfloat size_y;
    GLfloat bearing_x;
    GLfloat bearing_y;
    GLfloat glyph_width;
    GLfloat glyph_height;
} msdfgl_index_entry;

struct _msdfgl_context {
    FT_Library ft_library;

    GLfloat dpi[2];

    GLuint gen_shader;

    GLint _atlas_projection_uniform;
    GLint _texture_offset_uniform;
    GLint _translate_uniform;
    GLint _scale_uniform;
    GLint _range_uniform;
    GLint _glyph_height_uniform;

    GLint _meta_offset_uniform;
    GLint _point_offset_uniform;

    GLint metadata_uniform;
    GLint point_data_uniform;

    GLuint render_shader;

    GLint window_projection_uniform;
    GLint _font_atlas_projection_uniform;
    GLint _index_uniform;
    GLint _atlas_uniform;
    GLint _padding_uniform;
    GLint _offset_uniform;
    GLint _dpi_uniform;
    GLint _units_per_em_uniform;

    GLint _max_texture_size;

    GLuint bbox_vao;
    GLuint bbox_vbo;
};

GLfloat _MAT4_ZERO_INIT[4][4] = {{0.0f, 0.0f, 0.0f, 0.0f},
                                 {0.0f, 0.0f, 0.0f, 0.0f},
                                 {0.0f, 0.0f, 0.0f, 0.0f},
                                 {0.0f, 0.0f, 0.0f, 0.0f}};

void _msdfgl_ortho(GLfloat left, GLfloat right, GLfloat bottom, GLfloat top,
                   GLfloat nearVal, GLfloat farVal, GLfloat dest[][4]) {
    GLfloat rl, tb, fn;

    memcpy(dest, _MAT4_ZERO_INIT, sizeof(_MAT4_ZERO_INIT));

    rl = 1.0f / (right - left);
    tb = 1.0f / (top - bottom);
    fn = -1.0f / (farVal - nearVal);

    dest[0][0] = 2.0f * rl;
    dest[1][1] = 2.0f * tb;
    dest[2][2] = 2.0f * fn;
    dest[3][0] = -(right + left) * rl;
    dest[3][1] = -(top + bottom) * tb;
    dest[3][2] = (farVal + nearVal) * fn;
    dest[3][3] = 1.0f;
}

int compile_shader(const char *source, GLenum type, GLuint *shader, const char *version) {

    /* Default to versio */
    if (!version)
        version = "330 core";

    *shader = glCreateShader(type);
    if (!*shader) {
        fprintf(stderr, "failed to create shader\n");
    }

    const char *src[] = {"#version ", version, "\n", source};

    glShaderSource(*shader, 4, src, NULL);
    glCompileShader(*shader);

    GLint status;
    glGetShaderiv(*shader, GL_COMPILE_STATUS, &status);
    if (!status) {
        char log[1000];
        GLsizei len;
        glGetShaderInfoLog(*shader, 1000, &len, log);
        fprintf(stderr, "Error: compiling: %*s\n", len, log);
        return 0;
    }

    return 1;
}

msdfgl_context_t msdfgl_create_context(const char *version) {
    msdfgl_context_t ctx = (msdfgl_context_t)calloc(1, sizeof(struct _msdfgl_context));

    if (!ctx)
        return NULL;

    FT_Error error = FT_Init_FreeType(&ctx->ft_library);
    if (error) {
        free(ctx);
        return NULL;
    }

    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &ctx->_max_texture_size);

    GLuint vertex_shader, geometry_shader, fragment_shader;
    if (!compile_shader(_msdf_vertex, GL_VERTEX_SHADER, &vertex_shader, version))
        return NULL;
    if (!compile_shader(_msdf_fragment, GL_FRAGMENT_SHADER, &fragment_shader, version))
        return NULL;

    ctx->gen_shader = glCreateProgram();
    if (!(ctx->gen_shader = glCreateProgram()))
        return NULL;

    glAttachShader(ctx->gen_shader, vertex_shader);
    glAttachShader(ctx->gen_shader, fragment_shader);

    glLinkProgram(ctx->gen_shader);
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);

    GLint status;
    glGetProgramiv(ctx->gen_shader, GL_LINK_STATUS, &status);
    if (!status)
        return NULL;

    ctx->_atlas_projection_uniform = glGetUniformLocation(ctx->gen_shader, "projection");
    ctx->_texture_offset_uniform = glGetUniformLocation(ctx->gen_shader, "offset");
    ctx->_translate_uniform = glGetUniformLocation(ctx->gen_shader, "translate");
    ctx->_scale_uniform = glGetUniformLocation(ctx->gen_shader, "scale");
    ctx->_range_uniform = glGetUniformLocation(ctx->gen_shader, "range");
    ctx->_glyph_height_uniform = glGetUniformLocation(ctx->gen_shader, "glyph_height");

    ctx->_meta_offset_uniform = glGetUniformLocation(ctx->gen_shader, "meta_offset");
    ctx->_point_offset_uniform = glGetUniformLocation(ctx->gen_shader, "point_offset");

    ctx->metadata_uniform = glGetUniformLocation(ctx->gen_shader, "metadata");
    ctx->point_data_uniform = glGetUniformLocation(ctx->gen_shader, "point_data");

    GLenum err = glGetError();
    if (err) {
        fprintf(stderr, "error: %x \n", err);
        glDeleteProgram(ctx->gen_shader);
        return NULL;
    }

    if (!compile_shader(_font_vertex, GL_VERTEX_SHADER, &vertex_shader, version))
        return NULL;
    if (!compile_shader(_font_geometry, GL_GEOMETRY_SHADER, &geometry_shader, version))
        return NULL;
    if (!compile_shader(_font_fragment, GL_FRAGMENT_SHADER, &fragment_shader, version))
        return NULL;

    if (!(ctx->render_shader = glCreateProgram()))
        return NULL;
    glAttachShader(ctx->render_shader, vertex_shader);
    glAttachShader(ctx->render_shader, geometry_shader);
    glAttachShader(ctx->render_shader, fragment_shader);

    glLinkProgram(ctx->render_shader);
    glDeleteShader(vertex_shader);
    glDeleteShader(geometry_shader);
    glDeleteShader(fragment_shader);

    glGetProgramiv(ctx->render_shader, GL_LINK_STATUS, &status);

    if (!status) {
        glDeleteProgram(ctx->gen_shader);
        return NULL;
    }

    ctx->window_projection_uniform =
        glGetUniformLocation(ctx->render_shader, "projection");
    ctx->_font_atlas_projection_uniform =
        glGetUniformLocation(ctx->render_shader, "font_projection");
    ctx->_index_uniform = glGetUniformLocation(ctx->render_shader, "font_index");
    ctx->_atlas_uniform = glGetUniformLocation(ctx->render_shader, "font_atlas");
    ctx->_padding_uniform = glGetUniformLocation(ctx->render_shader, "padding");
    ctx->_dpi_uniform = glGetUniformLocation(ctx->render_shader, "dpi");
    ctx->_units_per_em_uniform = glGetUniformLocation(ctx->render_shader, "units_per_em");

    ctx->dpi[0] = 72.0;
    ctx->dpi[1] = 72.0;

    if ((err = glGetError())) {
        fprintf(stderr, "error: %x \n", err);
        glDeleteProgram(ctx->gen_shader);
        glDeleteProgram(ctx->render_shader);
        return NULL;
    }

    glGenVertexArrays(1, &ctx->bbox_vao);
    glGenBuffers(1, &ctx->bbox_vbo);

    glBindBuffer(GL_ARRAY_BUFFER, ctx->bbox_vbo);
    glBufferData(GL_ARRAY_BUFFER, 12 * sizeof(GLfloat), 0, GL_STREAM_READ);

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    return ctx;
}

void msdfgl_destroy_context(msdfgl_context_t ctx) {

    if (!ctx)
        return;

    FT_Done_FreeType(ctx->ft_library);

    glDeleteProgram(ctx->gen_shader);
    glDeleteProgram(ctx->render_shader);

    glDeleteVertexArrays(1, &ctx->bbox_vao);
    glDeleteBuffers(1, &ctx->bbox_vbo);

    free(ctx);
}

msdfgl_font_t msdfgl_load_font(msdfgl_context_t ctx, const char *font_name, float range,
                               float scale, int texture_size) {

    msdfgl_font_t f = (msdfgl_font_t)calloc(1, sizeof(struct _msdfgl_font));
    if (!f)
        return NULL;

    if (FT_New_Face(ctx->ft_library, font_name, 0, &f->face)) {
        free(f);
        return NULL;
    }

    f->scale = scale;
    f->range = range;

    f->texture_width = texture_size ? texture_size : ctx->_max_texture_size;

    f->context = ctx;
    f->_nglyphs = 0;
    f->_nallocated = 0;
    f->_offset_x = 1;
    f->_offset_y = 1;
    f->_y_increment = 0;
    f->_texture_height = 0;
    f->_direct_lookup_upper_limit = 0;
    f->atlas_padding = 2;

    f->vertical_advance = (float)(f->face->ascender - f->face->descender);

    msdfgl_map_init(&f->character_index);

    glGenBuffers(1, &f->_meta_input_buffer);
    glGenBuffers(1, &f->_point_input_buffer);
    glGenTextures(1, &f->_meta_input_texture);
    glGenTextures(1, &f->_point_input_texture);

    glGenBuffers(1, &f->_index_buffer);
    glGenTextures(1, &f->index_texture);

    glGenTextures(1, &f->atlas_texture);
    glGenFramebuffers(1, &f->_atlas_framebuffer);

    return f;
}

void msdfgl_destroy_font(msdfgl_font_t font) {

    FT_Done_Face(font->face);

    glDeleteBuffers(1, &font->_meta_input_buffer);
    glDeleteBuffers(1, &font->_point_input_buffer);
    glDeleteTextures(1, &font->_meta_input_texture);
    glDeleteTextures(1, &font->_point_input_texture);

    glDeleteBuffers(1, &font->_index_buffer);
    glDeleteTextures(1, &font->index_texture);

    glDeleteTextures(1, &font->atlas_texture);
    glDeleteFramebuffers(1, &font->_atlas_framebuffer);

    msdfgl_map_destroy(&font->character_index);

    free(font);
}

int _msdfgl_generate_glyphs_internal(msdfgl_font_t font, int32_t start, int32_t end,
                                     unsigned int range, int32_t *keys, int nkeys) {
    msdfgl_context_t ctx = font->context;
    int retval = -2;

    int nrender = range ? (end - start) : (nkeys - 1);

    if (nrender < 0)
        return -1;

    if (!font->_nglyphs && range && !start) {
        /* We can generate an optimized lookup for the atlas index. */
        font->_direct_lookup_upper_limit = end;
    }
    size_t *meta_sizes = NULL, *point_sizes = NULL;
    msdfgl_index_entry *atlas_index = NULL;
    void *point_data = NULL, *metadata = NULL;

    /* We will start with a square texture. */
    int new_texture_height = font->_texture_height ? font->_texture_height : 1;
    int new_index_size = font->_nallocated ? font->_nallocated : 1;

    /* Calculate the amount of memory needed on the GPU.*/
    if (!(meta_sizes = (size_t *)calloc(nrender + 1, sizeof(size_t))))
        goto error;
    if (!(point_sizes = (size_t *)calloc(nrender + 1, sizeof(size_t))))
        goto error;

    /* Amount of new memory needed for the index. */
    size_t index_size = (nrender + 1) * sizeof(msdfgl_index_entry);
    atlas_index = (msdfgl_index_entry *)calloc(1, index_size);
    if (!atlas_index)
        goto error;

    size_t meta_size_sum = 0, point_size_sum = 0;
    for (size_t i = 0; (int)i <= (int)nrender; ++i) {
        int index = range ? start + (int)i : keys[i];
        msdfgl_glyph_buffer_size(font->face, index, &meta_sizes[i], &point_sizes[i]);

        meta_size_sum += meta_sizes[i];
        point_size_sum += point_sizes[i];
    }

    /* Allocate the calculated amount. */
    if (!(point_data = calloc(point_size_sum, 1)))
        goto error;
    if (!(metadata = calloc(meta_size_sum, 1)))
        goto error;

    /* Serialize the glyphs into RAM. */
    char *meta_ptr = metadata;
    char *point_ptr = point_data;
    for (size_t i = 0; (int)i <= (int)nrender; ++i) {
        float buffer_width, buffer_height;

        int index = range ? start + (int)i : keys[i];
        msdfgl_serialize_glyph(font->face, index, meta_ptr, (GLfloat *)point_ptr);

        msdfgl_map_item_t *m = msdfgl_map_insert(&font->character_index, index);
        m->index = font->_nglyphs + i;
        m->advance[0] = (float)font->face->glyph->metrics.horiAdvance;
        m->advance[1] = (float)font->face->glyph->metrics.vertAdvance;

        buffer_width = font->face->glyph->metrics.width / SERIALIZER_SCALE + font->range;
        buffer_height =
            font->face->glyph->metrics.height / SERIALIZER_SCALE + font->range;
        buffer_width *= font->scale;
        buffer_height *= font->scale;

        meta_ptr += meta_sizes[i];
        point_ptr += point_sizes[i];

        if (font->_offset_x + buffer_width > font->texture_width) {
            font->_offset_y += (font->_y_increment + font->atlas_padding);
            font->_offset_x = 1;
            font->_y_increment = 0;
        }
        font->_y_increment = (size_t)buffer_height > font->_y_increment
                                 ? (size_t)buffer_height
                                 : font->_y_increment;

        atlas_index[i].offset_x = (GLfloat)font->_offset_x;
        atlas_index[i].offset_y = (GLfloat)font->_offset_y;
        atlas_index[i].size_x = buffer_width;
        atlas_index[i].size_y = buffer_height;
        atlas_index[i].bearing_x = (GLfloat)font->face->glyph->metrics.horiBearingX;
        atlas_index[i].bearing_y = (GLfloat)font->face->glyph->metrics.horiBearingY;
        atlas_index[i].glyph_width = (GLfloat)font->face->glyph->metrics.width;
        atlas_index[i].glyph_height = (GLfloat)font->face->glyph->metrics.height;

        font->_offset_x += (size_t)buffer_width + font->atlas_padding;

        while ((font->_offset_y + buffer_height) > new_texture_height) {
            new_texture_height *= 2;
        }
        if (new_texture_height > font->context->_max_texture_size) {
            goto error;
        }
        while (((int)(font->_nglyphs + i) + 1) > new_index_size) {
            new_index_size *= 2;
        }
    }

    /* Allocate and fill the buffers on GPU. */
    glBindBuffer(GL_ARRAY_BUFFER, font->_meta_input_buffer);
    glBufferData(GL_ARRAY_BUFFER, meta_size_sum, metadata, GL_DYNAMIC_READ);

    glBindBuffer(GL_ARRAY_BUFFER, font->_point_input_buffer);
    glBufferData(GL_ARRAY_BUFFER, point_size_sum, point_data, GL_DYNAMIC_READ);

    if ((int)font->_nallocated == new_index_size) {
        glBindBuffer(GL_ARRAY_BUFFER, font->_index_buffer);
    } else {
        GLuint new_buffer;
        glGenBuffers(1, &new_buffer);
        glBindBuffer(GL_ARRAY_BUFFER, new_buffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(msdfgl_index_entry) * new_index_size, 0,
                     GL_DYNAMIC_READ);
        if (glGetError() == GL_OUT_OF_MEMORY) {
            glDeleteBuffers(1, &new_buffer);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
            goto error;
        }
        if (font->_nglyphs) {
            glBindBuffer(GL_COPY_READ_BUFFER, font->_index_buffer);
            glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_ARRAY_BUFFER, 0, 0,
                                font->_nglyphs * sizeof(msdfgl_index_entry));
            glBindBuffer(GL_COPY_READ_BUFFER, 0);
        }
        font->_nallocated = new_index_size;
        glDeleteBuffers(1, &font->_index_buffer);
        font->_index_buffer = new_buffer;
    }
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(msdfgl_index_entry) * font->_nglyphs,
                    index_size, atlas_index);

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    /* Link sampler textures to the buffers. */
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_BUFFER, font->_meta_input_texture);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_R8UI, font->_meta_input_buffer);
    glBindTexture(GL_TEXTURE_BUFFER, 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_BUFFER, font->_point_input_texture);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_R32F, font->_point_input_buffer);
    glBindTexture(GL_TEXTURE_BUFFER, 0);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_BUFFER, font->index_texture);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_R32F, font->_index_buffer);
    glBindTexture(GL_TEXTURE_BUFFER, 0);

    glActiveTexture(GL_TEXTURE0);

    /* Generate the atlas texture and bind it as the framebuffer. */
    if (font->_texture_height == new_texture_height) {
        /* No need to extend the texture. */
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, font->_atlas_framebuffer);
        glBindTexture(GL_TEXTURE_2D, font->atlas_texture);
        glViewport(0, 0, font->texture_width, font->_texture_height);
    } else {
        GLuint new_texture;
        GLuint new_framebuffer;
        glGenTextures(1, &new_texture);
        glGenFramebuffers(1, &new_framebuffer);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, new_framebuffer);

        glBindTexture(GL_TEXTURE_2D, new_texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, font->texture_width,
                     new_texture_height, 0, GL_RGBA, GL_FLOAT, NULL);

        if (glGetError() == GL_OUT_OF_MEMORY) {
            /* Buffer size too big, are you trying to type Klingon? */
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glDeleteFramebuffers(1, &new_framebuffer);
            glDeleteTextures(1, &new_texture);
            goto error;
        }

        glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                               new_texture, 0);
        glViewport(0, 0, font->texture_width, new_texture_height);
        glClearColor(0.0, 0.0, 0.0, 1.0);
        glClear(GL_COLOR_BUFFER_BIT);

        if (font->_texture_height) {
            /* Old texture had data -> copy. */
            glBindFramebuffer(GL_READ_FRAMEBUFFER, font->_atlas_framebuffer);
            glBlitFramebuffer(0, 0, font->texture_width, font->_texture_height, 0, 0,
                              font->texture_width, font->_texture_height,
                              GL_COLOR_BUFFER_BIT, GL_NEAREST);
            glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
        }

        glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
        font->_texture_height = new_texture_height;
        glDeleteTextures(1, &font->atlas_texture);
        font->atlas_texture = new_texture;
        glDeleteFramebuffers(1, &font->_atlas_framebuffer);
        font->_atlas_framebuffer = new_framebuffer;
    }
    glBindTexture(GL_TEXTURE_2D, 0);

    GLfloat framebuffer_projection[4][4];
    _msdfgl_ortho(0, (GLfloat)font->texture_width, 0, (GLfloat)font->_texture_height,
                  -1.0, 1.0, framebuffer_projection);
    _msdfgl_ortho(-(GLfloat)font->texture_width, (GLfloat)font->texture_width,
                  -(GLfloat)font->_texture_height, (GLfloat)font->_texture_height, -1.0,
                  1.0, font->atlas_projection);

    glUseProgram(ctx->gen_shader);
    glUniform1i(ctx->metadata_uniform, 0);
    glUniform1i(ctx->point_data_uniform, 1);

    glUniformMatrix4fv(ctx->_atlas_projection_uniform, 1, GL_FALSE,
                       (GLfloat *)framebuffer_projection);

    glUniform2f(ctx->_scale_uniform, font->scale, font->scale);
    glUniform1f(ctx->_range_uniform, font->range);
    glUniform1i(ctx->_meta_offset_uniform, 0);
    glUniform1i(ctx->_point_offset_uniform, 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        printf("framebuffer incomplete: %x\n", glCheckFramebufferStatus(GL_FRAMEBUFFER));

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_BUFFER, font->_meta_input_texture);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_BUFFER, font->_point_input_texture);

    glBindVertexArray(ctx->bbox_vao);
    glBindBuffer(GL_ARRAY_BUFFER, ctx->bbox_vbo);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), 0);
    glEnableVertexAttribArray(0);

    int meta_offset = 0;
    int point_offset = 0;
    for (int i = 0; i <= nrender; ++i) {
        msdfgl_index_entry g = atlas_index[i];
        float w = g.size_x;
        float h = g.size_y;
        GLfloat bounding_box[] = {0, 0, w, 0, 0, h, 0, h, w, 0, w, h};
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(bounding_box), bounding_box);

        glUniform2f(
            ctx->_translate_uniform, -g.bearing_x / SERIALIZER_SCALE + font->range / 2.0f,
            (g.glyph_height - g.bearing_y) / SERIALIZER_SCALE + font->range / 2.0f);

        glUniform2f(ctx->_texture_offset_uniform, g.offset_x, g.offset_y);
        glUniform1i(ctx->_meta_offset_uniform, meta_offset);
        glUniform1i(ctx->_point_offset_uniform, point_offset / (2 * sizeof(GLfloat)));
        glUniform1f(ctx->_glyph_height_uniform, g.size_y);

        /* Do not bother rendering control characters */
        /* if (i > 31 && !(i > 126 && i < 160)) */
        glDrawArrays(GL_TRIANGLES, 0, 6);

        meta_offset += meta_sizes[i];
        point_offset += point_sizes[i];
    }

    glDisableVertexAttribArray(0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_BUFFER, 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_BUFFER, 0);

    glUseProgram(0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    font->_nglyphs += (nrender + 1);
    retval = nrender;

error:

    if (meta_sizes)
        free(meta_sizes);
    if (point_sizes)
        free(point_sizes);
    if (atlas_index)
        free(atlas_index);
    if (point_data)
        free(point_data);
    if (metadata)
        free(metadata);

    return retval;

}

int msdfgl_generate_glyphs(msdfgl_font_t font, int32_t start, int32_t end) {
    return _msdfgl_generate_glyphs_internal(font, start, end, 1, NULL, 0);
}

int msdfgl_generate_glyph(msdfgl_font_t font, int32_t character) {
    return _msdfgl_generate_glyphs_internal(font, character, character, 1, NULL, 0);
}

int msdfgl_generate_glyph_list(msdfgl_font_t font, int32_t *list, size_t n) {
    return _msdfgl_generate_glyphs_internal(font, 0, 0, 0, list, n);
}

void msdfgl_render(msdfgl_font_t font, msdfgl_glyph_t *glyphs, int n,
                   GLfloat *projection) {

    for (int i = 0; i < n; ++i) {
        /* If glyphs 0 - N were generated first, we can optimize by having their
           indices be equal to their keys. */
        if (glyphs[i].key >= font->_direct_lookup_upper_limit) {
            msdfgl_map_item_t *e = msdfgl_map_get(&font->character_index, glyphs[i].key);
            glyphs[i].key = e ? e->index : 0;
        }
    }

    GLuint glyph_buffer;
    GLuint vao;
    glGenBuffers(1, &glyph_buffer);
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, glyph_buffer);
    glBufferData(GL_ARRAY_BUFFER, n * sizeof(struct _msdfgl_glyph), &glyphs[0],
                 GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(struct _msdfgl_glyph),
                          (void *)offsetof(struct _msdfgl_glyph, x));

    glEnableVertexAttribArray(1);
    glVertexAttribIPointer(1, 4, GL_UNSIGNED_BYTE, sizeof(struct _msdfgl_glyph),
                           (void *)offsetof(struct _msdfgl_glyph, color));

    glEnableVertexAttribArray(2);
    glVertexAttribIPointer(2, 1, GL_INT, sizeof(struct _msdfgl_glyph),
                           (void *)offsetof(struct _msdfgl_glyph, key));

    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(struct _msdfgl_glyph),
                          (void *)offsetof(struct _msdfgl_glyph, size));

    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, sizeof(struct _msdfgl_glyph),
                          (void *)offsetof(struct _msdfgl_glyph, offset));

    glEnableVertexAttribArray(5);
    glVertexAttribPointer(5, 1, GL_FLOAT, GL_FALSE, sizeof(struct _msdfgl_glyph),
                          (void *)offsetof(struct _msdfgl_glyph, skew));

    glEnableVertexAttribArray(6);
    glVertexAttribPointer(6, 1, GL_FLOAT, GL_FALSE, sizeof(struct _msdfgl_glyph),
                          (void *)offsetof(struct _msdfgl_glyph, strength));

    glUseProgram(font->context->render_shader);

    /* Bind atlas texture and index buffer. */
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, font->atlas_texture);
    glUniform1i(font->context->_atlas_uniform, 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_BUFFER, font->index_texture);
    glUniform1i(font->context->_index_uniform, 1);

    glUniformMatrix4fv(font->context->_font_atlas_projection_uniform, 1, GL_FALSE,
                       (GLfloat *)font->atlas_projection);

    glUniformMatrix4fv(font->context->window_projection_uniform, 1, GL_FALSE, projection);
    glUniform1f(font->context->_padding_uniform,
                (GLfloat)(font->range / 2.0 * SERIALIZER_SCALE));
    glUniform1f(font->context->_units_per_em_uniform, (GLfloat)font->face->units_per_EM);
    glUniform2f(font->context->_dpi_uniform, font->context->dpi[0],
                font->context->dpi[1]);

    /* Render the glyphs. */
    glDrawArrays(GL_POINTS, 0, n);

    /* Clean up. */
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_BUFFER, 0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);

    glUseProgram(0);

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(2);
    glDisableVertexAttribArray(3);
    glDisableVertexAttribArray(4);
    glDisableVertexAttribArray(5);
    glDisableVertexAttribArray(6);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glDeleteBuffers(1, &glyph_buffer);
    glDeleteVertexArrays(1, &vao);
}

float msdfgl_printf(float x, float y, msdfgl_font_t font, float size, int32_t color,
                    GLfloat *projection, enum msdfgl_printf_flags flags, const void *fmt,
                    ...) {
    va_list argp;
    va_start(argp, fmt);

    size_t bufsize;
    if (flags & MSDFGL_WCHAR) {
        static wchar_t arr[255];
        bufsize = vswprintf(arr, 255, (const wchar_t *)fmt, argp);
    } else {
        bufsize = vsnprintf(NULL, 0, (const char *)fmt, argp);
    }
    va_end(argp);

    void *s = calloc(bufsize + 1, flags & MSDFGL_WCHAR ? sizeof(wchar_t) : sizeof(char));
    if (!s)
        return x;
    va_start(argp, fmt);
    if (flags & MSDFGL_WCHAR)
        vswprintf((wchar_t *)s, bufsize + 1, (const wchar_t *)fmt, argp);
    else
        vsnprintf((char *)s, bufsize + 1, (const char *)fmt, argp);
    va_end(argp);

    msdfgl_glyph_t *glyphs = calloc(bufsize, sizeof(msdfgl_glyph_t));
    if (!glyphs) {
        free(s);
        return x;
    }

    for (size_t i = 0; i < bufsize; ++i) {
        glyphs[i].x = x;
        glyphs[i].y = y;
        glyphs[i].color = color;
        if (flags & MSDFGL_WCHAR)
            glyphs[i].key = (int32_t)((wchar_t *)s)[i];
        else
            glyphs[i].key = (int32_t)((char *)s)[i];
        glyphs[i].size = (GLfloat)size;
        glyphs[i].offset = 0;
        glyphs[i].skew = 0;
        glyphs[i].strength = 0.5;

        msdfgl_map_item_t *e = msdfgl_map_get(&font->character_index, glyphs[i].key);
        if (!e)
            continue;
        FT_Vector kerning = {0, 0};

        if (flags & MSDFGL_KERNING && i < bufsize - 1 && FT_HAS_KERNING(font->face))
            FT_Get_Kerning(font->face, FT_Get_Char_Index(font->face, glyphs[i].key),
                           FT_Get_Char_Index(font->face, glyphs[i + 1].key),
                           FT_KERNING_UNSCALED, &kerning);

        if (flags & MSDFGL_VERTICAL)
            y += (e->advance[1] + kerning.y) * (size * font->context->dpi[1] / 72.0f) /
                 font->face->units_per_EM;
        else
            x += (e->advance[0] + kerning.x) * (size * font->context->dpi[0] / 72.0f) /
                 font->face->units_per_EM;
    }
    msdfgl_render(font, glyphs, bufsize, projection);
    free(glyphs);
    free(s);

    return flags & MSDFGL_VERTICAL ? y : x;
}

float msdfgl_vertical_advance(msdfgl_font_t font, float size) {
    return font->vertical_advance * (size * font->context->dpi[1] / 72.0f) /
           font->face->units_per_EM;
}

GLuint _msdfgl_atlas_texture(msdfgl_font_t font) { return font->atlas_texture; }
GLuint _msdfgl_index_texture(msdfgl_font_t font) { return font->index_texture; }

void msdfgl_set_dpi(msdfgl_context_t context, float horizontal, float vertical) {
    context->dpi[0] = horizontal;
    context->dpi[1] = vertical;
}
