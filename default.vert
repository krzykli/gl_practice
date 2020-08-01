#version 410

// Input vertex data, different for all executions of this shader.
layout(location = 0) in vec3 vertexPosition_modelspace;
layout(location = 1) in vec3 vertexColor;
layout(location = 2) in vec3 n;

// Values that stay constant for the whole mesh.
uniform mat4 MVP;
out vec3 fragmentColor;
out vec3 Normal;

void main(){
  // Output position of the vertex, in clip space : MVP * position
  fragmentColor = vertexColor;
  gl_Position =  MVP * vec4(vertexPosition_modelspace, 1);
  Normal = MVP * vec4(n, 1);
}
