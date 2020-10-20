#version 410

uniform vec3 camera_position;
uniform float hover_multiplier;

out vec3 color;
in vec3 fragmentColor;
in vec3 normal;

void main() {
    vec3 light_pos = vec3(1,1,1);
    vec3 light_color = vec3(0.5,0.8,1);
    vec3 ambient_light  = vec3(0.1,0.1,0.2);
    float cosTheta = clamp( dot( normal, light_pos ), 0,1 );
    color = fragmentColor + vec3(0.7, 0.0, 0.7) * hover_multiplier + cosTheta * light_color + ambient_light;
    color = normal;
}
