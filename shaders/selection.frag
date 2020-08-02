#version 410

in vec3 fragmentColor;
out vec3 color;

uniform float time;

void main() {
    color = fragmentColor * ((sin(time * 8) + 2) / 2);
}
