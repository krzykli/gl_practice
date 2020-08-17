#version 410
layout (location = 0) in vec2 vertex; // <vec2 pos>
uniform mat4 ortho_projection;

void main()
{
    gl_Position = ortho_projection * vec4(vertex.xy, 0.0, 1.0);
}
