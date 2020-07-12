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
#include "mesh.h"

#include "assets/grid.h"
#include "assets/cube.h"

static Camera global_cam;
static double delta_time; 

static float cursor_delta_x = 0;
static float cursor_delta_y = 0;
static float last_press_x = 0;
static float last_press_y = 0;

static int window_width = 640;
static int window_height = 480;

static bool rotate_mode = false;
static bool pan_mode = false;

static glm::vec3 pan_vector_x;
static glm::vec3 pan_vector_y;

static Array mesh_data_array;
static xorshift32_state xor_state;

static bool render_flip = true;

static Mesh* selected_mesh = NULL;
static Mesh* mouse_over_mesh = NULL;

const float TWO_M_PI = M_PI*2.0f;
const float M_PI_OVER_TWO = M_PI/2.0f;

GLuint picker_shader_program_id;
GLuint selection_shader_program_id;


Mesh createRandomCubeOnASphere(xorshift32_state &xor_state)
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


Mesh createRandomCubeOnAPlane(xorshift32_state &xor_state)
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
    cube_mesh.model_matrix = glm::scale(cube_mesh.model_matrix, glm::vec3(offset / float(UINT_MAX)));

    return cube_mesh;
}


typedef struct v2i
{
    u32 x;
    u32 y;
} v2i;


v2i get_mouse_pixel_coords(GLFWwindow* window)
{
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);

    int frame_width, frame_height;
    glfwGetFramebufferSize(window, &frame_width, &frame_height);

    int window_width, window_height;
    glfwGetWindowSize(window, &window_width, &window_height);

    float hdpi_factor = float(frame_width/ window_width);
    xpos *= hdpi_factor;
    ypos *= hdpi_factor;

    int x_px = (int)(xpos);
    int y_px = (int)(frame_height - ypos);

    v2i result = {x_px, y_px};

    return result;
}



glm::mat4 get_view_matrix(int width, int height)
{
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
    return vp;
}

GLMesh create_gl_mesh_instance(Mesh &mesh, u32 vector_dimensions)
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


void decompose_u32(u32 number, byte* array)
{
    array[0] = number & 0xFF; 
    array[1] = (number >> 8) & 0xFF;
    array[2] = (number >> 16) & 0xFF;
    array[3] = (number >> 24) & 0xFF;
}


void render_selection_buffer(GLFWwindow* window)
{
    // Instance mesh
    Mesh mesh_instance;
    mesh_instance.vertex_positions = cube_vertices;
    mesh_instance.vertex_array_length = sizeof(cube_vertices) / sizeof(GLfloat);
    mesh_instance.vertex_colors = cube_colors;
    mesh_instance.model_matrix  = glm::mat4(1.0);
    GLMesh glmesh = create_gl_mesh_instance(mesh_instance, 3);

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glm::mat4 vp = get_view_matrix(window_width, window_height);

    u32 element_count = mesh_data_array.element_count;
    for (u32 i=0; i < element_count; i++)
    {
        Mesh* cube_mesh= (Mesh*)ArrayGetIndex(mesh_data_array, i);
        glmesh.mesh = cube_mesh;

        byte bytes[4];
        decompose_u32(i, bytes);
        // Create an ID from mesh index
        glm::vec4 picker_color = glm::vec4(bytes[0], bytes[1], bytes[2], bytes[3]);

        // Draw
        glm::mat4 mvp = vp * glmesh.mesh->model_matrix;
        GLuint matrix_id = glGetUniformLocation(
            picker_shader_program_id, "MVP");

        glUniformMatrix4fv(matrix_id, 1, GL_FALSE, &mvp[0][0]);

        GLuint picker_id = glGetUniformLocation(
            picker_shader_program_id, "picker_id");

        GLfloat uniform[4] = {
            (GLfloat)picker_color[0] / 255.0f,
            (GLfloat)picker_color[1] / 255.0f,
            (GLfloat)picker_color[2] / 255.0f,
            (GLfloat)picker_color[3] / 255.0f,
        };
        glUniform4fv(picker_id, 1, &uniform[0]);

        glUseProgram(picker_shader_program_id);
        glBindVertexArray(glmesh.vao);
        glDrawArrays(GL_TRIANGLES, 0, glmesh.mesh->vertex_array_length / 3.0f);
    }
}


static void cursorPositionCallback(GLFWwindow* window, double xpos, double ypos)
{
    render_selection_buffer(window);
    cursor_delta_x = xpos - last_press_x;
    cursor_delta_y = ypos - last_press_y;
    last_press_x = xpos;
    last_press_y = ypos;

    v2i px_coords = get_mouse_pixel_coords(window);

    glReadBuffer(GL_BACK);
    byte res[4];
    glReadPixels(px_coords.x, px_coords.y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, &res);
    u32 mesh_id = *res;
    if(mesh_id)
    {
        mouse_over_mesh = (Mesh*)ArrayGetIndex(mesh_data_array, mesh_id - 1);
    }
    else
    {
        mouse_over_mesh = NULL;
    }
}



void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    // TODO(kk): cleanup actions
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS && !mods)
    {
        render_selection_buffer(window);
        v2i pixel_coords = get_mouse_pixel_coords(window);
        glReadBuffer(GL_BACK);

        byte res[4];
        glReadPixels(pixel_coords.x, pixel_coords.y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, &res);
        u32 mesh_id = *res;

        if(mesh_id)
        {
            printf("Selected %i\n", mesh_id);
            selected_mesh = (Mesh*)ArrayGetIndex(mesh_data_array, mesh_id);
        }
        else
        {
            selected_mesh = NULL;
        }
    }

    if(button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS && mods & GLFW_MOD_ALT)
    {
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);
        last_press_x = xpos;
        last_press_y = ypos;
        rotate_mode = true;
    }

    if(button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS && mods & GLFW_MOD_CONTROL)
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

    if(button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
    {
        rotate_mode = false;
        pan_mode = false;
    }

}


void focus_on_mesh(Mesh* mesh)
{
    glm::vec3 target = mesh->model_matrix[3];
    // move towards the target
    global_cam.target = target;
    /*glm::vec3 dir = glm::normalize(global_cam.position - target);*/
    /*glm::vec3 new_pos = target + 10.0f * dir;*/
    /*global_cam.position = new_pos;*/
}



void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_UP)
    {
        for (int i=0; i < 10; ++i)
        {
            /*Mesh cube_mesh = createRandomCubeOnASphere(xor_state);*/
            Mesh cube_mesh = createRandomCubeOnAPlane(xor_state);
            ArrayAppend(mesh_data_array, (void*)&cube_mesh);
        }
    }
    else if (key == GLFW_KEY_DOWN)
    {
        for (int i=1; i < 10; ++i)
        {
            ArrayPop(mesh_data_array);
        }
    }
    else if (key == GLFW_KEY_F and action == GLFW_PRESS)
    {
        if (selected_mesh)
        {
            focus_on_mesh(selected_mesh);
        }
        else
            global_cam.target = glm::vec3(0.0f, 0.0f, 0.0f);
        updateCameraCoordinateFrame(global_cam);

    }
    else if (key == GLFW_KEY_E and action == GLFW_PRESS)
    {
        render_flip = !render_flip;
    }
}


void scrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    yoffset = fmin(yoffset, 1.0f);
    yoffset = fmax(yoffset, -1.3f);
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


size_t array_defaul_resizer(void* array_pointer)
{
    Array* arr = (Array*)array_pointer;
    printf("element size, %i\n", arr->element_size);
    printf("max element count, %i\n", arr->max_element_count);
    printf("element count, %i\n", arr->element_count);
    printf("need twice as much, %i\n", arr->_block_size * 2);
    return arr->_block_size * 2;
}


int main()
{
    GLFWwindow* window;

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


    // SHADER SETUP
    GLuint default_vert_id = glCreateShader(GL_VERTEX_SHADER);
    u8 rv = compileShader(default_vert_id, "shaders/default.vert");
    assert(rv);

    GLuint default_frag_id = glCreateShader(GL_FRAGMENT_SHADER);
    rv = compileShader(default_frag_id, "shaders/default.frag");
    assert(rv);

    GLuint outline_frag_id = glCreateShader(GL_FRAGMENT_SHADER);
    rv = compileShader(outline_frag_id, "shaders/default.frag");
    assert(rv);

    GLuint noop_vert_id = glCreateShader(GL_VERTEX_SHADER);
    rv = compileShader(noop_vert_id, "shaders/noop.vert");
    assert(rv);

    GLuint picker_frag_id = glCreateShader(GL_FRAGMENT_SHADER);
    rv = compileShader(picker_frag_id, "shaders/picker.frag");
    assert(rv);

    GLuint selection_frag_id = glCreateShader(GL_FRAGMENT_SHADER);
    rv = compileShader(selection_frag_id, "shaders/selection.frag");
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

    picker_shader_program_id = glCreateProgram();
    glAttachShader(picker_shader_program_id, default_vert_id);
    glAttachShader(picker_shader_program_id, picker_frag_id);
    glLinkProgram(picker_shader_program_id);

    selection_shader_program_id = glCreateProgram();
    glAttachShader(selection_shader_program_id, default_vert_id);
    glAttachShader(selection_shader_program_id, selection_frag_id);
    glLinkProgram(selection_shader_program_id);

    // WORLD
    glm::vec3 pos = glm::vec3(10, 10, -10);
    global_cam.position = pos;
    global_cam.target = glm::vec3(0, 0, 0);
    updateCameraCoordinateFrame(global_cam);

    u32 max_cubes = 100;
    mesh_data_array.element_size = sizeof(Mesh);
    mesh_data_array.max_element_count = max_cubes;
    mesh_data_array.resize_func = array_defaul_resizer;
    ArrayInit(mesh_data_array);

    xor_state.a = 10;

    double current_frame = glfwGetTime();
    double last_frame= current_frame;

    // Instance mesh
    Mesh cube_mesh = cube_create_mesh();
    GLMesh cube_glmesh = create_gl_mesh_instance(cube_mesh, 3);

    // World grid
    Mesh grid_mesh = grid_create_mesh();
    GLMesh grid_glmesh = create_gl_mesh_instance(grid_mesh, 3);

    Mesh origin_cube = cube_create_mesh();
    origin_cube.model_matrix = glm::translate(origin_cube.model_matrix,
                                              glm::vec3(0, 1, 0));
    ArrayAppend(mesh_data_array, (void*)&origin_cube);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glEnable(GL_LINE_SMOOTH);


    while (!glfwWindowShouldClose(window)) {
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        current_frame = glfwGetTime();
        delta_time = current_frame - last_frame;
        last_frame = current_frame;

        glfwGetWindowSize(window, &window_width, &window_height);
        glm::mat4 vp = get_view_matrix(window_width, window_height);

        u32 element_count = mesh_data_array.element_count;

        if (render_flip)
        {
            for (int i=0; i < element_count; i++)
            {
                Mesh* cube_mesh = (Mesh*)ArrayGetIndex(mesh_data_array, i);
                u32 offset = 0;
                float ratioX = offset / float(UINT_MAX);

                glm::vec3 position = cube_mesh->model_matrix[3];
                SphericalCoords spherical_coords = getSphericalCoords(position);
                spherical_coords.theta += (1 - ratioX) * 0.0001f ;
                glm::vec3 new_position = getCartesianCoords(spherical_coords);
                glm::vec3 diff = new_position - position;
                diff[1] += current_frame / 200.0f;

                /*cube_mesh->model_matrix = glm::translate(cube_mesh->model_matrix, diff);*/

                cube_glmesh.mesh = cube_mesh;

                GLuint shader = default_shader_program_id;
                /*printf("%p is %p\n", cube_mesh, mouse_over_mesh);*/
                if(cube_mesh == mouse_over_mesh || cube_mesh == selected_mesh)
                {
                    drawGLMesh(cube_glmesh, GL_TRIANGLES, selection_shader_program_id, vp);
                }
                else
                {
                    drawGLMesh(cube_glmesh, GL_TRIANGLES, default_shader_program_id, vp);
                }

                // kill logic
                if (new_position.y > 200)
                {
                    /*ArrayGetIndex(mesh_data_array, i);*/
                    /*ArrayRemove(mesh_data_array, i);*/
                }
            }

            // World grid
            glUseProgram(noop_shader_program_id);
            glBindVertexArray(grid_glmesh.vao);
            drawGLMesh(grid_glmesh, GL_LINES, noop_shader_program_id, vp);
        }
        else
            render_selection_buffer(window);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }


    ArrayFree(mesh_data_array);

    glfwTerminate();
    return 0;
}
