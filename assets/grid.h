#ifndef GRIDH
#define GRIDH

#define GRID_VERT_COUNT 132

static GLfloat grid_verts[GRID_VERT_COUNT];
static GLfloat grid_color[GRID_VERT_COUNT];

Mesh grid_create_mesh()
{
    u32 size = GRID_VERT_COUNT;

    int x_min = -5;
    int x_max = 5;

    int z_min = -5;
    int z_max = 5;
    int step = 1;

    int counter = 0;
    for (int i=0; i < size / 2; i += 6)
    {
        counter = i / 6;

        grid_verts[i] = x_min;
        grid_verts[i+1] = 0;
        grid_verts[i+2] = z_min + counter * step;

        grid_verts[i+3] = x_max;
        grid_verts[i+4] = 0;
        grid_verts[i+5] = z_min + counter * step;

        grid_color[i] = 0.3f;
        grid_color[i + 1] = 0.5f;
        grid_color[i + 2] = 0.7f;
        grid_color[i + 3] = 0.3f;
        grid_color[i + 4] = 0.5f;
        grid_color[i + 5] = 0.7f;

    }

    for (byte i=size/2; i < size; i += 6)
    {
        int counter2 = i / 6 - counter - 1 ;
        grid_verts[i] = x_min + counter2 * step;
        grid_verts[i+1] = 0;
        grid_verts[i+2] = z_min;

        grid_verts[i+3] = x_min  + counter2 * step;
        grid_verts[i+4] = 0;
        grid_verts[i+5] = z_max;

        grid_color[i] = 0.3f;
        grid_color[i + 1] = 0.5f;
        grid_color[i + 2] = 0.7f;
        grid_color[i + 3] = 0.3f;
        grid_color[i + 4] = 0.5f;
        grid_color[i + 5] = 0.7f;
    }

    Mesh grid_mesh;
    grid_mesh.vertex_positions = grid_verts;
    grid_mesh.vertex_colors = grid_color;
    grid_mesh.vertex_array_length = sizeof(grid_verts) / sizeof(GLfloat);
    grid_mesh.model_matrix  = glm::mat4(1.0);
    return grid_mesh;
}

#endif // GRIDH

