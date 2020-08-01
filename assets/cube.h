#ifndef CUBEH
#define CUBEH


static GLfloat cube_vertices[] = {
    -1.0f,-1.0f,-1.0f,
    -1.0f,-1.0f, 1.0f,
    -1.0f, 1.0f, 1.0f,
    1.0f, 1.0f,-1.0f,
    -1.0f,-1.0f,-1.0f,
    -1.0f, 1.0f,-1.0f,
    1.0f,-1.0f, 1.0f,
    -1.0f,-1.0f,-1.0f,
    1.0f,-1.0f,-1.0f,
    1.0f, 1.0f,-1.0f,
    1.0f,-1.0f,-1.0f,
    -1.0f,-1.0f,-1.0f,
    -1.0f,-1.0f,-1.0f,
    -1.0f, 1.0f, 1.0f,
    -1.0f, 1.0f,-1.0f,
    1.0f,-1.0f, 1.0f,
    -1.0f,-1.0f, 1.0f,
    -1.0f,-1.0f,-1.0f,
    -1.0f, 1.0f, 1.0f,
    -1.0f,-1.0f, 1.0f,
    1.0f,-1.0f, 1.0f,
    1.0f, 1.0f, 1.0f,
    1.0f,-1.0f,-1.0f,
    1.0f, 1.0f,-1.0f,
    1.0f,-1.0f,-1.0f,
    1.0f, 1.0f, 1.0f,
    1.0f,-1.0f, 1.0f,
    1.0f, 1.0f, 1.0f,
    1.0f, 1.0f,-1.0f,
    -1.0f, 1.0f,-1.0f,
    1.0f, 1.0f, 1.0f,
    -1.0f, 1.0f,-1.0f,
    -1.0f, 1.0f, 1.0f,
    1.0f, 1.0f, 1.0f,
    -1.0f, 1.0f, 1.0f,
    1.0f,-1.0f, 1.0f
};


static GLfloat cube_colors[] = {
    0.583f,  0.771f,  0.014f,
    0.609f,  0.115f,  0.436f,
    0.327f,  0.483f,  0.844f,
    0.822f,  0.569f,  0.201f,
    0.435f,  0.602f,  0.223f,
    0.310f,  0.747f,  0.185f,
    0.597f,  0.770f,  0.761f,
    0.559f,  0.436f,  0.730f,
    0.359f,  0.583f,  0.152f,
    0.483f,  0.596f,  0.789f,
    0.559f,  0.861f,  0.639f,
    0.195f,  0.548f,  0.859f,
    0.014f,  0.184f,  0.576f,
    0.771f,  0.328f,  0.970f,
    0.406f,  0.615f,  0.116f,
    0.676f,  0.977f,  0.133f,
    0.971f,  0.572f,  0.833f,
    0.140f,  0.616f,  0.489f,
    0.997f,  0.513f,  0.064f,
    0.945f,  0.719f,  0.592f,
    0.543f,  0.021f,  0.978f,
    0.279f,  0.317f,  0.505f,
    0.167f,  0.620f,  0.077f,
    0.347f,  0.857f,  0.137f,
    0.055f,  0.953f,  0.042f,
    0.714f,  0.505f,  0.345f,
    0.783f,  0.290f,  0.734f,
    0.722f,  0.645f,  0.174f,
    0.302f,  0.455f,  0.848f,
    0.225f,  0.587f,  0.040f,
    0.517f,  0.713f,  0.338f,
    0.053f,  0.959f,  0.120f,
    0.393f,  0.621f,  0.362f,
    0.673f,  0.211f,  0.457f,
    0.820f,  0.883f,  0.371f,
    0.982f,  0.099f,  0.879f
};


Mesh cube_create_mesh()
{
    Mesh mesh;
    mesh.vertex_array_length = sizeof(cube_vertices) / sizeof(*cube_vertices);
    mesh.vertex_positions = cube_vertices;
    mesh.vertex_colors = cube_colors;
    mesh.vertex_normals = NULL;
    mesh.model_matrix  = glm::mat4(1.0);
    return mesh;
}


Mesh cube_create_random_on_sphere(xorshift32_state &xor_state)
{
    Mesh cube_mesh = cube_create_mesh();
    u32 base_offset = 100;

    u32 offset = xorshift32(&xor_state);
    float ratioX = offset / float(UINT_MAX);

    offset = xorshift32(&xor_state);
    float ratioY = offset / float(UINT_MAX);

    offset = xorshift32(&xor_state);
    float ratioZ = offset / float(UINT_MAX);

    offset = xorshift32(&xor_state);

    SphericalCoords cube_sphr_coords;
    cube_sphr_coords.theta = 3.14f * 2 * ratioX;
    cube_sphr_coords.phi = 3.14f  * ratioY;
    cube_sphr_coords.radius = 100.0f;

    glm::vec3 cube_pos = getCartesianCoords(cube_sphr_coords);

    cube_mesh.model_matrix = glm::translate(cube_mesh.model_matrix, cube_pos);
    cube_mesh.model_matrix = glm::scale(cube_mesh.model_matrix, glm::vec3(offset / float(UINT_MAX)));

    return cube_mesh;
}


Mesh cube_create_random_on_plane(xorshift32_state &xor_state)
{
    Mesh cube_mesh = cube_create_mesh();
    u32 base_offset = 40;

    u32 offset = xorshift32(&xor_state);
    float ratioX = offset / float(UINT_MAX);
    ratioX -= .5f;

    offset = xorshift32(&xor_state);
    float ratioY = offset / 2 / float(UINT_MAX);

    offset = xorshift32(&xor_state);
    float ratioZ = offset / float(UINT_MAX);

    offset = xorshift32(&xor_state);

    glm::vec3 cube_pos = glm::vec3(base_offset * ratioX - base_offset,
                                   base_offset * ratioY - base_offset,
                                   0);

    cube_mesh.model_matrix = glm::translate(cube_mesh.model_matrix, cube_pos);
    /*cube_mesh.model_matrix = glm::scale(cube_mesh.model_matrix, glm::vec3(offset / float(UINT_MAX)));*/

    return cube_mesh;
}
#endif // CUBEH
