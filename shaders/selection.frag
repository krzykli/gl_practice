#version 410

out vec3 color;
in vec3 fragmentColor;

void main() {
  color = vec3(fragmentColor[0] * 2.0f, fragmentColor[1], fragmentColor[2]);
}
