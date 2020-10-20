#version 330 core
out vec4 FragColor;

in vec3 ourColor;
in vec2 TexCoord;

uniform sampler2D texture1;

void main()
{
    // renders are linear, apply srgb
    float gamma = 2.2;
    FragColor = texture(texture1, TexCoord);
    FragColor.rgb = pow(FragColor.rgb, vec3(gamma));
}

