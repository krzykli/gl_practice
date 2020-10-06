#ifndef MESHH
#define MESHH

typedef struct Mesh
{
    GLuint vao;
    GLuint shader_id;
    u32 vertex_array_length;
    glm::mat4 model_matrix;
    glm::mat4 inverse_model_matrix;
    glm::mat4 inverse_transpose_model_matrix;
    float bbox[6];
    float* vertex_positions;
    float* vertex_colors;
    float* vertex_normals;
    const char* mesh_name;
} Mesh;


void mesh_get_bbox(float* vertex_buffer, u32 length, float* bbox)
{
    float minx = vertex_buffer[0];
    float miny = vertex_buffer[1];
    float minz = vertex_buffer[2];

    float maxx = vertex_buffer[0];
    float maxy = vertex_buffer[1];
    float maxz = vertex_buffer[2];

    for(float* p=vertex_buffer;
        p < vertex_buffer + length;
        p += 3)
    {
         float x = *p;
         float y = *(p+1);
         float z = *(p+2);

         if(x < minx)
         {
              minx = x;
         }
         else if(x > maxx)
         {
              maxx = x;
         }

         if(y < miny)
         {
              miny = y;
         }
         else if(y > maxy)
         {
              maxy = y;
         }

         if(z < minz)
         {
              minz = z;
         }
         else if(z > maxz)
         {
              maxz = z;
         }
    }

    bbox[0] = minx;
    bbox[1] = miny;
    bbox[2] = minz;
    bbox[3] = maxx;
    bbox[4] = maxy;
    bbox[5] = maxz;
}


void mesh_init(Mesh &mesh, u32 vector_dimensions)
{
    mesh.model_matrix = glm::mat4(1);
    mesh.mesh_name = "mesh";
    mesh_get_bbox(mesh.vertex_positions, mesh.vertex_array_length, mesh.bbox);

    print("%f %f %f - %f %f %f", mesh.bbox[0], mesh.bbox[1], mesh.bbox[2], mesh.bbox[3], mesh.bbox[4], mesh.bbox[5]);

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
        glVertexAttribPointer(2, vector_dimensions, GL_FLOAT, GL_FALSE, 0, NULL);
    }
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    mesh.vao = vao;
}


void mesh_draw_bbox(Mesh& mesh, u32 shader_id, glm::mat4 vp) {
    // Cube 1x1x1, centered on origin
    GLfloat vertices[] = {
        -0.5, -0.5, -0.5, 1.0,
        0.5, -0.5, -0.5, 1.0,
        0.5,  0.5, -0.5, 1.0,
        -0.5,  0.5, -0.5, 1.0,
        -0.5, -0.5,  0.5, 1.0,
        0.5, -0.5,  0.5, 1.0,
        0.5,  0.5,  0.5, 1.0,
        -0.5,  0.5,  0.5, 1.0,
    };

    GLfloat colors[] = {
        1.0, 0.0, 0.0, 1.0,
        1.0, 0.0, 0.0, 1.0,
        1.0, 0.0, 0.0, 1.0,
        1.0, 0.0, 0.0, 1.0,
        1.0, 0.0, 0.0, 1.0,
        1.0, 0.0, 0.0, 1.0,
        1.0, 0.0, 0.0, 1.0,
        1.0, 0.0, 0.0, 1.0,
    };
    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    GLuint vbo_vertices;
    glGenBuffers(1, &vbo_vertices);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_vertices);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    GLushort elements[] = {
        0, 1, 2, 3,
        4, 5, 6, 7,
        0, 4, 1, 5,
        2, 6, 3, 7
    };

    GLuint ibo_elements;
    glGenBuffers(1, &ibo_elements);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_elements);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(elements), elements, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    float minx = mesh.bbox[0];
    float miny = mesh.bbox[1];
    float minz = mesh.bbox[2];
    float maxx = mesh.bbox[3];
    float maxy = mesh.bbox[4];
    float maxz = mesh.bbox[5];

    glm::vec3 size = glm::vec3(maxx-minx, maxy-miny, maxz-minz);
    glm::mat4 transform = vp * glm::scale(mesh.model_matrix, size);

    /* Apply object's transformation matrix */
    glUseProgram(shader_id);
    GLuint matrix_id = glGetUniformLocation(shader_id, "MVP");
    glUniformMatrix4fv(matrix_id, 1, GL_FALSE, &transform[0][0]);

    glBindBuffer(GL_ARRAY_BUFFER, vbo_vertices);
    u32 attribute_v_coord = 0;
    glEnableVertexAttribArray(attribute_v_coord);
    glVertexAttribPointer(
        attribute_v_coord,  // attribute
        4,                  // number of elements per vertex, here (x,y,z,w)
        GL_FLOAT,           // the type of each element
        GL_FALSE,           // take our values as-is
        0,                  // no extra data between each position
        0                   // offset of first element
    );

    GLuint colorbuffer;
    glGenBuffers(1, &colorbuffer);
    glBindBuffer(GL_ARRAY_BUFFER, colorbuffer);
    glBufferData(GL_ARRAY_BUFFER,
                 32 * sizeof(float),
                 colors,
                 GL_STATIC_DRAW);

    // shader layout 1
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, NULL);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_elements);
    glDrawElements(GL_LINE_LOOP, 4, GL_UNSIGNED_SHORT, 0);
    glDrawElements(GL_LINE_LOOP, 4, GL_UNSIGNED_SHORT, (GLvoid*)(4*sizeof(GLushort)));
    glDrawElements(GL_LINES, 8, GL_UNSIGNED_SHORT, (GLvoid*)(8*sizeof(GLushort)));
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    glDisableVertexAttribArray(attribute_v_coord);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glDeleteBuffers(1, &vbo_vertices);
    glDeleteBuffers(1, &ibo_elements);
    glBindVertexArray(0);
    glUseProgram(0);
}

#endif // MESHH
