#version 410

uniform ivec3 picker_id;

out vec4 color;

void main()
{
    color = vec4(picker_id[0], picker_id[1], picker_id[2], 0);
}
