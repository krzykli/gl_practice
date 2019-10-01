#version 410

// Input vertex data, different for all executions of this shader.
layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec3 vertexColor;

// Values that stay constant for the whole mesh.
out vec3 fragmentColor;

void main(){
  // Output position of the vertex, in clip space : MVP * position
  fragmentColor = vertexColor;
  gl_Position = vec4(vertexPosition, 1);
}
