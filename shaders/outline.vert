#version 410

layout(location = 0) in vec3 vertexPosition_modelspace;
layout(location = 1) in vec3 vertexColor;
layout(location = 2) in vec3 vertexNormal;

uniform mat4 MVP;
uniform vec3 camera_position;
out vec3 fragmentColor;

void main(){
    // Output position of the vertex, in clip space : MVP * position
    fragmentColor = vertexColor;
    vec3 newPos = vertexPosition_modelspace + normalize(vertexNormal) * max(length(vertexPosition_modelspace) * 0.05, 0.2);
    gl_Position =  MVP * vec4(newPos, 1);
}
