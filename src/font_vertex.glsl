
layout (location = 0) in vec2 vertex;
layout (location = 1) in uvec4 glyph_color;
layout (location = 2) in int glyph_index;
layout (location = 3) in float size;
layout (location = 4) in float y_offset;
layout (location = 5) in float skewness;
layout (location = 6) in float strength;

uniform mat4 projection;

out VS_OUT {
    int glyph;
    vec4 color;
    float size;
    float y_offset;
    float skewness;
    float strength;

} vs_out;

void main() {
    gl_Position = vec4(vertex.xy, 0.0, 1.0);
    vs_out.glyph = glyph_index;
    uvec4 c = glyph_color;
    vs_out.color = vec4(float(c.a) / 255.0, float(c.b) / 255.0,
                        float(c.g) / 255.0, float(c.r) / 255.0);
    vs_out.size = size;
    vs_out.y_offset = y_offset;
    vs_out.skewness = skewness;
    vs_out.strength = strength;
}
