#ifndef MESHH
#define MESHH

typedef struct Mesh
{
    GLuint vao;
    GLuint shader_id;
    u32 vertex_array_length;
    glm::mat4 model_matrix;
    float* vertex_positions;
    float* vertex_colors;
    float* vertex_normals;
    char* mesh_name;
} Mesh;


void mesh_initialize_VAO(Mesh &mesh, u32 vector_dimensions)
{
    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    GLuint vbo;
    glGenBuffers(1, &vbo);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 mesh.vertex_array_length * sizeof(float),
                 mesh.vertex_positions,
                 GL_STATIC_DRAW);

    // shader layout 0
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, vector_dimensions, GL_FLOAT, GL_FALSE, 0, NULL);

    if(mesh.vertex_colors)
    {
        GLuint colorbuffer;
        glGenBuffers(1, &colorbuffer);
        glBindBuffer(GL_ARRAY_BUFFER, colorbuffer);
        glBufferData(GL_ARRAY_BUFFER,
                     mesh.vertex_array_length * sizeof(float),
                     mesh.vertex_colors,
                     GL_STATIC_DRAW);

        // shader layout 1
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, vector_dimensions, GL_FLOAT, GL_FALSE, 0, NULL);
    }

    if(mesh.vertex_normals)
    {
        GLuint normal_buffer;
        glGenBuffers(1, &normal_buffer);
        glBindBuffer(GL_ARRAY_BUFFER, normal_buffer);
        glBufferData(GL_ARRAY_BUFFER,
                     mesh.vertex_array_length * sizeof(float),
                     mesh.vertex_normals,
                     GL_STATIC_DRAW);

        // shader layout 2
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(1, vector_dimensions, GL_FLOAT, GL_FALSE, 0, NULL);
    }
    mesh.vao = vao;
}


#endif // MESHH
