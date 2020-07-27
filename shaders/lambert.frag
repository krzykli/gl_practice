#version 410

out vec3 color;
in vec3 fragmentColor;
in vec3 Normal;

void main() {
  color = Normal;
}
