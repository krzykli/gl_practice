#version 410

out vec3 color;
in vec3 fragmentColor;

void main() {
  color = fragmentColor;
}
