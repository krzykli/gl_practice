#version 410

// Input vertex data, different for all executions of this shader.
layout(location = 0) in vec3 vertexPosition_modelspace;
layout(location = 1) in vec3 vertexColor;
layout(location = 2) in vec3 vertexNormal;

// Values that stay constant for the whole mesh.
uniform mat4 MVP;
uniform vec3 camera_position;

out vec3 fragmentColor;
out vec3 normal;

void main(){
    // Output position of the vertex, in clip space : MVP * position
    normal = vertexNormal;
    fragmentColor = vertexColor;

    gl_Position =  MVP * vec4(vertexPosition_modelspace, 1);
}
