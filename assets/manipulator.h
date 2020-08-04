
#ifndef MANIPULATORH
#define MANIPULATORH

Mesh manipulator_create_mesh()
{
    u32 size = 6 * 3 * 3;

    //GLfloat vertices[] = {-1, -1, 0, // bottom left corner
                      //-1,  1, 0, // top left corner
                       //1,  1, 0, // top right corner
                       //1, -1, 0}; // bottom right corner
                       // 0 1 2 0 2 3
    GLfloat verts[] = {
        0.0f, 0.0f, -0.01f,
        1.0f, 0.0f, -0.01f,
        1.0f, 0.0f, 0.01f,

        0.0f, 0.0f, -0.01f,
        1.0f, 0.0f, 0.01f,
        0.0f, 0.0f, 0.01f,

        -0.01f, 0.0f, 0.0f,
        -0.01f, 0.0f, 1.0f,
        0.01f, 0.0f, 1.0f,

        -0.01f, 0.0f, 0.0f,
        0.01f, 0.0f, 1.0f,
        0.01f, 0.0f, 0.00f,

        -0.01f, 0.0f, 0.0f,
        -0.01f, 1.0f, 0.0f,
        0.01f, 1.0f, 0.0f,

        -0.01f, 0.0f, 0.0f,
        0.01f, 1.0f, 0.0f,
        0.01f, 0.0f, 0.00f,
    };
    GLfloat* manip_verts = (GLfloat*)malloc(size * sizeof(verts[0]));
    for(u32 i=0; i<size; ++i)
    {
         manip_verts[i] = verts[i];
    }

    GLfloat colors[] = {
        1.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,

        0.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f,

        0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f,
    };

    GLfloat* manip_colors = (GLfloat*)malloc(size * sizeof(colors[0]));
    for(u32 i=0; i<size; ++i)
    {
         manip_colors[i] = colors[i];
    }


    Mesh manipulator_mesh;
    manipulator_mesh.vertex_positions = manip_verts;
    manipulator_mesh.vertex_colors = manip_colors;
    manipulator_mesh.vertex_normals = NULL;
    manipulator_mesh.vertex_array_length = sizeof(verts) / sizeof(GLfloat);
    manipulator_mesh.model_matrix = glm::mat4(1.0f);
    return manipulator_mesh;
}

#endif // MANIPULATORH
