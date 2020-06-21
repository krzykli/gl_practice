#include <cmath>

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/transform.hpp>

#include "types.h"
#include "camera.h"
#include "array.h"


static Camera global_cam;
static double delta_time; 

static float cursor_delta_x = 0;
static float cursor_delta_y = 0;
static float last_press_x = 0;
static float last_press_y = 0;

static bool rotate_mode = false;
static bool pan_mode = false;

static glm::vec3 pan_vector_x;
static glm::vec3 pan_vector_y;

static Array cube_data_array;
static xorshift32_state xor_state;

const float TWO_M_PI = M_PI*2.0f;
const float M_PI_OVER_TWO = M_PI/2.0f;


static GLfloat cube_vertices[] = {
    -1.0f,-1.0f,-1.0f, // triangle 1 : begin
    -1.0f,-1.0f, 1.0f,
    -1.0f, 1.0f, 1.0f, // triangle 1 : end
    1.0f, 1.0f,-1.0f, // triangle 2 : begin
    -1.0f,-1.0f,-1.0f,
    -1.0f, 1.0f,-1.0f, // triangle 2 : end
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

static GLfloat colorData[] = {
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

static GLfloat colorData1[] = {
    0.983f,  0.999f,  0.414f,
    0.909f,  0.999f,  0.436f,
    0.927f,  0.999f,  0.444f,
    0.922f,  0.999f,  0.401f,
    0.935f,  0.999f,  0.423f,
    0.910f,  0.999f,  0.485f,
    0.997f,  0.999f,  0.461f,
    0.959f,  0.999f,  0.430f,
    0.959f,  0.999f,  0.452f,
    0.983f,  0.999f,  0.489f,
    0.959f,  0.999f,  0.439f,
    0.995f,  0.999f,  0.459f,
    0.914f,  0.999f,  0.476f,
    0.971f,  0.999f,  0.470f,
    0.906f,  0.999f,  0.416f,
    0.976f,  0.999f,  0.433f,
    0.971f,  0.999f,  0.433f,
    0.940f,  0.999f,  0.489f,
    0.997f,  0.999f,  0.464f,
    0.945f,  0.999f,  0.492f,
    0.943f,  0.999f,  0.478f,
    0.979f,  0.999f,  0.405f,
    0.967f,  0.999f,  0.477f,
    0.947f,  0.999f,  0.437f,
    0.955f,  0.999f,  0.442f,
    0.914f,  0.999f,  0.445f,
    0.983f,  0.999f,  0.434f,
    0.922f,  0.999f,  0.474f,
    0.902f,  0.999f,  0.448f,
    0.925f,  0.999f,  0.440f,
    0.917f,  0.999f,  0.438f,
    0.953f,  0.999f,  0.420f,
    0.993f,  0.999f,  0.462f,
    0.973f,  0.999f,  0.457f,
    0.920f,  0.999f,  0.471f,
    0.982f,  0.999f,  0.479f
};



typedef struct Mesh
{
    u32 vertex_array_length;
    float* vertex_positions;
    float* vertex_colors;
    glm::mat4 model_matrix;
} Mesh;


typedef struct GLMesh
{
     GLuint vao;
     Mesh* mesh;
} GLMesh;


typedef struct CubeData
{
     u32 offset;
     Mesh mesh;
} CubeData;


CubeData createCube()
{
    Mesh cube_mesh;
    cube_mesh.vertex_array_length = 36 * 3;
    cube_mesh.vertex_positions = cube_vertices;
    cube_mesh.vertex_colors = colorData;
    cube_mesh.model_matrix  = glm::mat4(1.0);

    CubeData cube_data;
    cube_data.mesh = cube_mesh;
    cube_data.offset = 0;
    return cube_data;
}

CubeData createRandomCubeOnASphere(xorshift32_state &xor_state)
{
    Mesh cube_mesh;
    cube_mesh.vertex_array_length = 36 * 3;
    cube_mesh.vertex_positions = cube_vertices;
    cube_mesh.vertex_colors = colorData;
    cube_mesh.model_matrix  = glm::mat4(1.0);
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

    /*glm::vec3 cube_pos = glm::vec3(base_offset * ratioX - base_offset,*/
                                   /*base_offset * ratioY - base_offset,*/
                                   /*base_offset * ratioZ - base_offset);*/

    cube_mesh.model_matrix = glm::translate(cube_mesh.model_matrix, cube_pos);
    cube_mesh.model_matrix = glm::scale(cube_mesh.model_matrix, glm::vec3(offset / float(UINT_MAX)));
    CubeData cube_data;

    cube_data.mesh = cube_mesh;
    cube_data.offset = offset;

    return cube_data;
}


CubeData createRandomCubeOnAPlane(xorshift32_state &xor_state)
{
    Mesh cube_mesh;
    cube_mesh.vertex_array_length = 36 * 3;
    cube_mesh.vertex_positions = cube_vertices;
    cube_mesh.vertex_colors = colorData;
    cube_mesh.model_matrix  = glm::mat4(1.0);
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
    cube_mesh.model_matrix = glm::scale(cube_mesh.model_matrix, glm::vec3(offset / float(UINT_MAX)));
    CubeData cube_data;

    cube_data.mesh = cube_mesh;
    cube_data.offset = offset;

    return cube_data;
}


void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_W)
    {
        SphericalCoords spherical_coords = getSphericalCoords(global_cam.position);
        spherical_coords.theta -= 0.01f;
        global_cam.position = getCartesianCoords(spherical_coords);
    }
    else if (key == GLFW_KEY_UP)
    {
        for (int i=0; i < 10; ++i)
        {
            /*CubeData cube_data = createRandomCubeOnASphere(xor_state);*/
            CubeData cube_data = createRandomCubeOnAPlane(xor_state);
            ArrayAppend(cube_data_array, (void*)&cube_data);
        }
    }
    else if (key == GLFW_KEY_DOWN)
    {
        for (int i=1; i < 10; ++i)
        {
            ArrayPop(cube_data_array);
        }
    }
    else if(key == GLFW_KEY_F)
    {
        global_cam.target = glm::vec3(0.0f, 0.0f, 0.0f);
        updateCameraCoordinateFrame(global_cam);
    }
}


static void cursorPositionCallback(GLFWwindow* window, double xpos, double ypos)
{
    cursor_delta_x = xpos - last_press_x;
    cursor_delta_y = ypos - last_press_y;
    last_press_x = xpos;
    last_press_y = ypos;
}


void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{

    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS && mods & GLFW_MOD_ALT)
    {
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);
        last_press_x = xpos;
        last_press_y = ypos;
        rotate_mode = true;
    }
    else if(button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS && mods & GLFW_MOD_CONTROL)
    {
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);
        last_press_x = xpos;
        last_press_y = ypos;
        pan_mode = true;
        pan_vector_y = global_cam.up;
        if (pan_vector_y.y > 0)
            pan_vector_x = -getCameraRightVector(global_cam);
        else
            pan_vector_x = getCameraRightVector(global_cam);
    }
    else if(button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
    {
        rotate_mode = false;
        pan_mode = false;
    }
}


void scrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    yoffset = fmin(yoffset, 1.0f);
    yoffset = fmax(yoffset, -1.0f);
    glm::vec3 direction = getCameraDirection(global_cam);
    glm::vec3 offset = direction * (float)yoffset * 1.5f;
    glm::vec3 new_pos = global_cam.position + offset;
    float distance = glm::distance(new_pos, global_cam.target);
    if (distance < 1.0f)
        offset = glm::vec3(0, 0, 0);

    global_cam.position += offset;
}


void loadFileContents(const char* file_path, char* buffer)
{
    FILE* fh;
    fh = fopen(file_path, "r");
    fread(buffer, 1000, 1, fh);
    fclose(fh);
}


u8 compileShader(GLuint shader_id, const char* shader_path)
{
    u32 buffer_size = 1000;
    char* shader_buffer = (char*)calloc(1, sizeof(char) * buffer_size);
    /*memset((void*)shader_buffer, 0, buffer_size);*/

    loadFileContents(shader_path, shader_buffer);

    glShaderSource(shader_id, 1, &shader_buffer, NULL);
    glCompileShader(shader_id);
    free(shader_buffer);

    GLint is_compiled = 0;
    glGetShaderiv(shader_id, GL_COMPILE_STATUS, &is_compiled);
    if(is_compiled == GL_FALSE)
    {
        GLint max_length = 0;
        glGetShaderiv(shader_id, GL_INFO_LOG_LENGTH, &max_length);
        char errorLog[max_length];
        glGetShaderInfoLog(shader_id, max_length, &max_length, &errorLog[0]);
        glDeleteShader(shader_id);
        printf("%s", errorLog);
        return 0;
    }
    return 1;
}


void drawGLMesh(GLMesh &gl_mesh, GLenum mode, GLuint shader_program_id, glm::mat4 vp)
{
    glm::mat4 mvp = vp * gl_mesh.mesh->model_matrix;

    GLuint matrix_id = glGetUniformLocation(
        shader_program_id, "MVP");

    glUniformMatrix4fv(matrix_id, 1, GL_FALSE, &mvp[0][0]);

    glUseProgram(shader_program_id);
    glBindVertexArray(gl_mesh.vao);
    glDrawArrays(mode, 0, gl_mesh.mesh->vertex_array_length / 3.0f);
};


GLMesh prepareMeshForRendering(Mesh &mesh, u32 vector_dimensions)
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

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, vector_dimensions, GL_FLOAT, GL_FALSE, 0, NULL);

    GLuint colorbuffer;
    glGenBuffers(1, &colorbuffer);
    glBindBuffer(GL_ARRAY_BUFFER, colorbuffer);
    glBufferData(GL_ARRAY_BUFFER,
                 mesh.vertex_array_length * sizeof(float),
                 mesh.vertex_colors,
                 GL_STATIC_DRAW);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, vector_dimensions, GL_FLOAT, GL_FALSE, 0, NULL);

    GLMesh gl_mesh;
    gl_mesh.mesh = &mesh;
    gl_mesh.vao = vao;
    return gl_mesh;

}

void on_array_allocation_failed(size_t size)
{
     printf("CUSTOM_CALLBACK ohno");
}

int main()
{
    GLFWwindow* window;
    u32 window_width = 640;
    u32 window_height = 480;

    // GL INIT
    if (!glfwInit())
        return -1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    window = glfwCreateWindow(
        window_width, window_height, "Hello World", NULL, NULL);

    if (!window) {
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    glewExperimental = GL_TRUE;
    if (glewInit())
        return -1;

    glfwSetKeyCallback(window, keyCallback);
    glfwSetCursorPosCallback(window, cursorPositionCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetScrollCallback(window, scrollCallback);

    printf("OpenGL version supported by this platform: %s\n",
            glGetString(GL_VERSION));
    printf("GLSL version supported by this platform: %s\n",
            glGetString(GL_SHADING_LANGUAGE_VERSION));
    // END GL INIT


    GLuint default_vert_id = glCreateShader(GL_VERTEX_SHADER);
    u8 rv = compileShader(default_vert_id, "default.vert");
    assert(rv);

    GLuint default_frag_id = glCreateShader(GL_FRAGMENT_SHADER);
    rv = compileShader(default_frag_id, "default.frag");
    assert(rv);

    GLuint outline_frag_id = glCreateShader(GL_FRAGMENT_SHADER);
    rv = compileShader(outline_frag_id, "default.frag");
    assert(rv);

    GLuint noop_vert_id = glCreateShader(GL_VERTEX_SHADER);
    rv = compileShader(noop_vert_id, "noop.vert");
    assert(rv);

    GLuint default_shader_program_id = glCreateProgram();
    glAttachShader(default_shader_program_id, default_vert_id);
    glAttachShader(default_shader_program_id, default_frag_id);
    glLinkProgram(default_shader_program_id);

    GLuint outline_shader_program_id = glCreateProgram();
    glAttachShader(outline_shader_program_id, default_vert_id);
    glAttachShader(outline_shader_program_id, outline_frag_id);
    glLinkProgram(outline_shader_program_id);

    GLuint noop_shader_program_id = glCreateProgram();
    glAttachShader(noop_shader_program_id, noop_vert_id);
    glAttachShader(noop_shader_program_id, outline_frag_id);
    glLinkProgram(noop_shader_program_id);

    // WORLD
    glm::vec3 pos = glm::vec3(5, 5, -5);
    global_cam.position = pos;
    global_cam.target = glm::vec3(0, 0, 0);
    updateCameraCoordinateFrame(global_cam);

    u32 max_cubes = 3000;
    cube_data_array.element_size = sizeof(CubeData);
    cube_data_array.max_element_count = max_cubes;
    cube_data_array.onFailure = on_array_allocation_failed;
    ArrayInit(cube_data_array);


    xor_state.a = 10;

    double current_frame = glfwGetTime();
    double last_frame= current_frame;

    // Instance mesh
    Mesh cube_mesh;
    cube_mesh.vertex_positions = cube_vertices;
    cube_mesh.vertex_array_length = sizeof(cube_vertices) / sizeof(GLfloat);
    cube_mesh.vertex_colors = colorData;
    cube_mesh.model_matrix  = glm::mat4(1.0);
    GLMesh glmesh = prepareMeshForRendering(cube_mesh, 3);

    byte line_count = 22;
    u32 size = line_count * 2 * 3;

    GLfloat grid_verts[size];
    GLfloat grid_color[size];
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

    Mesh line_mesh;
    line_mesh.vertex_positions = grid_verts;
    line_mesh.vertex_colors= grid_color;
    line_mesh.vertex_array_length = sizeof(grid_verts) / sizeof(GLfloat);
    line_mesh.model_matrix  = glm::mat4(1.0);
    GLMesh lineGlmesh = prepareMeshForRendering(line_mesh, 3);

    while (!glfwWindowShouldClose(window)) {
        current_frame = glfwGetTime();
        delta_time = current_frame - last_frame;
        last_frame = current_frame;

        int width, height;
        glfwGetWindowSize(window, &width, &height);

        glm::mat4 Projection = glm::perspective(
            glm::radians(45.0f),
            (float)width / (float)height,
            0.1f, 10000.0f
        );

        glm::mat4 View = glm::mat4();

        // USER INTERACTION
        if (pan_mode)
        {
            float distance = glm::distance(global_cam.position, global_cam.target);
            float pan_speed_multiplier = distance * 0.001f;
            float delta_pan_x = cursor_delta_x * pan_speed_multiplier;
            float delta_pan_y = cursor_delta_y * pan_speed_multiplier;

            glm::vec3 offset = pan_vector_x * delta_pan_x + pan_vector_y * delta_pan_y;
            global_cam.position += offset;
            global_cam.target += offset;

            View = glm::lookAt(
                global_cam.position,
                global_cam.target,
                global_cam.up
            );
        }
        else if(rotate_mode)
        {
            // This thread saved me after a long time of having weird
            // flipping issues at phi 0 or 90
            // https://stackoverflow.com/questions/54027740/proper-way-to-handle-camera-rotations
            View = glm::lookAt(
                global_cam.position,
                global_cam.target,
                global_cam.up
            );

            float delta_theta = cursor_delta_x * 0.01f;
            float delta_phi = cursor_delta_y * 0.01f;
            if (delta_theta)
            {
                // We can perform world space rotation here, no need to calculate the pivot
                glm::vec3 pivot = global_cam.target;
                glm::vec3 axis  = glm::vec3(0, 1, 0);
                glm::mat4 rotation_matrix = glm::rotate(glm::mat4(1), delta_theta, axis);
                glm::mat4 rotation_with_pivot = glm::translate(glm::mat4(1), pivot) * rotation_matrix * glm::translate(glm::mat4(1), -pivot);
                // Since we're working in world space here we need to rotate first
                View = View * rotation_with_pivot;
            }
            if (delta_phi)
            {
                // To rotate the camera without flipping we need to rotate
                // it by moving it to origin and resetting it back.
                glm::vec3 pivot = glm::vec3(View * glm::vec4(global_cam.target, 1.0f));
                glm::vec3 axis  = glm::vec3(1, 0, 0);
                glm::mat4 rotation_matrix = glm::rotate(glm::mat4(1), delta_phi, axis);
                glm::mat4 rotation_with_pivot = glm::translate(glm::mat4(1), pivot) * rotation_matrix * glm::translate(glm::mat4(1), -pivot);
                // Apply the rotation after the current view
                View = rotation_with_pivot * View;
            }
        }
        else
        {
            View = glm::lookAt(
                global_cam.position,
                global_cam.target,
                global_cam.up
            );
        }
        cursor_delta_x = 0;
        cursor_delta_y = 0;
        // END USER INTERACTION

        // Update camera using the view matrix
        glm::mat4 camera_world = glm::inverse(View);
        float targetDist = glm::length(global_cam.target - global_cam.position);
        global_cam.position = glm::vec3(camera_world[3]);
        global_cam.target = global_cam.position - glm::vec3(camera_world[2]) * targetDist;
        global_cam.up = glm::vec3(camera_world[1]);

        glm::mat4 vp = Projection * View;

        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        u32 element_count = cube_data_array.element_count;

        for (int i=0; i < element_count; i++)
        {
            CubeData* cube_data= (CubeData*)ArrayGetIndex(cube_data_array, i);

            u32 offset = cube_data->offset;
            float ratioX = offset / float(UINT_MAX);

            glm::vec3 position = cube_data->mesh.model_matrix[3];
            SphericalCoords spherical_coords = getSphericalCoords(position);
            spherical_coords.theta += (1 - ratioX) * 0.01f ;
            glm::vec3 new_position = getCartesianCoords(spherical_coords);
            glm::vec3 diff = new_position - position;
            diff[1] += current_frame/200.0f;

            cube_data->mesh.model_matrix = glm::translate(cube_data->mesh.model_matrix, diff);

            glmesh.mesh = &cube_data->mesh;
            drawGLMesh(glmesh, GL_TRIANGLES, default_shader_program_id, vp);

            // kill logic
            if (new_position.y > 200)
            {
                /*ArrayGetIndex(cube_data_array, i);*/
                /*ArrayRemove(cube_data_array, i);*/
            }
        }

;       CubeData originCube = createCube();
        glmesh.mesh = &originCube.mesh;
        originCube.mesh.model_matrix = glm::translate(originCube.mesh.model_matrix,
                                                      glm::vec3(0, 1,0));
        drawGLMesh(glmesh, GL_TRIANGLES, default_shader_program_id, vp);

        glMatrixMode(GL_PROJECTION);
        glEnable(GL_LINE_SMOOTH);

        // World grid
        glUseProgram(noop_shader_program_id);
        glBindVertexArray(lineGlmesh.vao);
        drawGLMesh(lineGlmesh, GL_LINES, noop_shader_program_id, vp);

        glfwPollEvents();
        glfwSwapBuffers(window);
    }


    ArrayFree(cube_data_array);

    glfwTerminate();
    return 0;
}
