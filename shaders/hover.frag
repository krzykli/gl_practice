#version 410

out vec3 color;
in vec3 fragmentColor;

void main() {
    color = vec3(fragmentColor) * 1.5;
}
