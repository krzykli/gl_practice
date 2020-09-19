#version 410

layout(location = 0) in vec3 vertexPosition_modelspace;
layout(location = 1) in vec3 vertexColor;
layout(location = 2) in vec3 vertexNormal;

uniform mat4 MVP;
uniform vec3 camera_position;
out vec3 fragmentColor;
out vec3 normal;

void main(){
    // Output position of the vertex, in clip space : MVP * position
    fragmentColor = vertexColor;
    normal = vertexNormal;
    vec3 cam_distance = abs(camera_position - vertexPosition_modelspace);
    vec3 newPos = vertexPosition_modelspace + normalize(vertexNormal) * cam_distance / 100.0f;
    gl_Position =  MVP * vec4(newPos, 1);
}
