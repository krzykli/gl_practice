#version 410

out vec3 color;
in vec3 fragmentColor;
uniform float time;

void main() {
    color = fragmentColor * ((sin(time * 8) + 2) / 2);
}
