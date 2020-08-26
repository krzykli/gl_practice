#version 410
layout (location = 0) in vec2 vertex; // <vec2 pos>
layout (location = 1) in vec3 vertexColor;

out vec3 fragmentColor;

void main()
{
    fragmentColor = vertexColor;
    gl_Position = vec4(vertex.xy, 0.0, 1.0);
}
