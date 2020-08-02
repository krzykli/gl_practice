#version 410

out vec3 color;
uniform vec3 camera_position;

void main() {
    color = normalize(camera_position);
}
