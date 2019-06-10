
layout (points) in;
layout (triangle_strip, max_vertices = 4) out;

in VS_OUT {
    int glyph;
    vec4 color;
    float size;
    float y_offset;
    float skewness;
    float strength;
} gs_in[];

out vec2 text_pos;
out vec4 text_color;
out float strength;

uniform mat4 projection;
uniform float padding;
uniform float units_per_em;
uniform vec2 dpi;

precision mediump samplerBuffer;
uniform samplerBuffer font_index;


void main() {
    text_color = gs_in[0].color;
    strength = gs_in[0].strength;

    vec4 font_size = vec4(gs_in[0].size * dpi / 72.0 / units_per_em, 1.0, 1.0);

    int _offset = 8 * gs_in[0].glyph;
    vec2 text_offset = vec2(texelFetch(font_index, _offset + 0).r,
                            texelFetch(font_index, _offset + 1).r);
    vec2 glyph_texture_width = vec2(texelFetch(font_index, _offset + 2).r, 0.0 );
    vec2 glyph_texture_height = vec2(0.0, texelFetch(font_index, _offset + 3).r);

    vec4 bearing = vec4(texelFetch(font_index, _offset + 4).r,
                        -texelFetch(font_index, _offset + 5).r, 0.0, 0.0) * font_size;

    vec4 glyph_width = vec4(texelFetch(font_index, _offset + 6).r, 0.0, 0.0, 0.0) * font_size;
    vec4 glyph_height = vec4(0.0, texelFetch(font_index, _offset + 7).r, 0.0, 0.0) * font_size;

    vec4 padding_x = vec4(padding, 0.0, 0.0, 0.0) * font_size;
    vec4 padding_y = vec4(0.0, padding, 0.0, 0.0) * font_size;
    float skewness = gs_in[0].skewness;

    vec4 p = gl_in[0].gl_Position + vec4(0.0, gs_in[0].y_offset, 0.0, 0.0);
    vec4 _p = p;


    // BL
    _p = p + bearing + glyph_height - padding_x + padding_y;
    _p.x += skewness * (p.y - _p.y);
    gl_Position = projection * _p;
    text_pos = text_offset + glyph_texture_height;
    EmitVertex();

    // BR
    _p = p + bearing + glyph_height + glyph_width + padding_x + padding_y;
    _p.x += skewness * (p.y - _p.y);
    gl_Position = projection * _p;
    text_pos = text_offset + glyph_texture_width + glyph_texture_height;
    EmitVertex();

    // TL
    _p = p + bearing - padding_x - padding_y;
    _p.x += skewness * (p.y - _p.y);
    gl_Position = projection * _p;
    text_pos = text_offset;
    EmitVertex();

    // TR
    _p = p + bearing + glyph_width + padding_x - padding_y;
    _p.x += skewness * (p.y - _p.y);
    gl_Position = projection * _p;
    text_pos = text_offset + glyph_texture_width;
    EmitVertex();

    EndPrimitive();
}
