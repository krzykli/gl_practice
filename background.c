#ifndef BACKGROUNDH
#define BACKGROUNDH

GLuint background_init_vao()
{
    float background_vertices[4][2] = {
        {-1,  1},
        {-1, -1},
        { 1, -1},
        { 1,  1}
    };

    float background_vertex_colors[4][3] = {
        {0.2, 0.2, 0.2},
        {0.05, 0.05, 0.05},
        {0.05, 0.05, 0.05},
        {0.2, 0.2, 0.2},
    };

    unsigned int background_VAO, background_VBO, background_color;
    glGenVertexArrays(1, &background_VAO);
    glGenBuffers(1, &background_VBO);
    glGenBuffers(1, &background_color);

    glBindVertexArray(background_VAO);
    glBindBuffer(GL_ARRAY_BUFFER, background_VBO);
    glBufferData(GL_ARRAY_BUFFER,
                 8 * sizeof(float),
                 background_vertices,
                 GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, NULL);

    glBindBuffer(GL_ARRAY_BUFFER, background_color);
    glBufferData(GL_ARRAY_BUFFER,
                 12 * sizeof(float),
                 background_vertex_colors,
                 GL_STATIC_DRAW);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, NULL);
    glBindVertexArray(0);

    return background_VAO;
}
#endif // BACKGROUNDH
