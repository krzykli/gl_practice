#ifndef MESHH
#define MESHH

typedef struct Mesh
{
    u32 vertex_array_length;
    glm::mat4 model_matrix;
    float* vertex_positions;
    float* vertex_colors;
} Mesh;


typedef struct GLMesh
{
     GLuint vao;
     Mesh* mesh;
} GLMesh;

#endif // MESHH
