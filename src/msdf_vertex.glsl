
layout (location = 0) in vec2 vertex;

precision mediump float;
uniform mat4 projection;
uniform vec2 offset;

void main() {
    gl_Position = projection * vec4(vertex.xy + offset, 1.0, 1.0);
}
