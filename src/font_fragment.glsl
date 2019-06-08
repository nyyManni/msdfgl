
precision highp float;
in vec2 text_pos;
in vec4 text_color;
in float strength;
out vec4 color;

uniform sampler2D font_atlas;
uniform mat4 font_projection;

float median(float r, float g, float b) {
    return max(min(r, g), min(max(r, g), b));
}
float pxRange = 4.0;

void main() {
    vec2 coords = (font_projection * vec4(text_pos, 0.0, 1.0)).xy;

    /* Invert the strength so that 1.0 becomes bold and 0.0 becomes thin */
    float threshold = 1.0 - strength;

    vec2 msdfUnit = pxRange/vec2(textureSize(font_atlas, 0));
    vec3 s = texture(font_atlas, coords).rgb;
    float sigDist = median(s.r, s.g, s.b) - threshold;
    sigDist *= dot(msdfUnit, 0.5/fwidth(coords));
    float opacity = clamp(sigDist + 0.5, 0.0, 1.0);
    color = mix(vec4(0.0, 0.0, 0.0, 0.0), text_color, opacity);
}
