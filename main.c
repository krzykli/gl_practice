// TODOs
// - Manipulators
// - multiple selection
// - cleanup text rendering
// - Indexed draws
// - picker shader fixes
// - dict struct
// - lights
// - transform stack?
// - UI widgets
// - gradient background
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
#include "debug.h"
#include "camera.h"
#include "array.h"
#include "dict.h"
#include "mesh.h"
#include "text.h"

#include "assets/grid.h"
#include "assets/cube.h"
#include "assets/manipulator.h"
#include "io/objloader.h"

static Camera global_cam;
static double delta_time; 

static float cursor_delta_x = 0;
static float cursor_delta_y = 0;
static float last_press_x = 0;
static float last_press_y = 0;

static float press_start_x = 0;
static float press_start_y = 0;

static int window_width = 640;
static int window_height = 480;

static bool rotate_mode = false;
static bool pan_mode = false;

static glm::vec3 pan_vector_x;
static glm::vec3 pan_vector_y;

static Array mesh_data_array;
static xorshift32_state xor_state;

static bool render_selction_buffer = false;
static bool draw_viewport_marquee = false;

static Array selected_mesh_indices;
static Mesh* mouse_over_mesh = NULL;

const float TWO_M_PI = M_PI*2.0f;
const float M_PI_OVER_TWO = M_PI/2.0f;

GLuint default_shader_program_id;
GLuint hover_shader_program_id;
GLuint picker_shader_program_id;
GLuint selection_shader_program_id;


typedef struct v2i
{
    u32 x;
    u32 y;
} v2i;


typedef struct v2f
{
    float x;
    float y;
} v2f;


typedef struct Marquee
{
    v2f bottom;
    v2f top;
} Marquee;

Marquee marquee;


void loadFileContents(const char* file_path, char* buffer)
{
    FILE* fh;
    fh = fopen(file_path, "r");
    fread(buffer, 1000, 1, fh);  // FIXME
    fclose(fh);
}


u8 compile_shader(GLuint shader_id, const char* shader_path)
{
    print("Compiling shader %s", shader_path);
    u32 buffer_size = 1000;
    char* shader_buffer = (char*)calloc(1, sizeof(char) * buffer_size);

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
        print("%s", errorLog);
        return 0;
    }
    return 1;
}


GLuint create_shader(const char* vertex_shader, const char* fragment_shader)
{
    GLuint shader_program_id = glCreateProgram();
    GLuint vert_id = glCreateShader(GL_VERTEX_SHADER);
    u8 rv = compile_shader(vert_id, vertex_shader);
    assert(rv);

    glAttachShader(shader_program_id, vert_id);
    GLuint frag_id = glCreateShader(GL_FRAGMENT_SHADER);
    rv = compile_shader(frag_id, fragment_shader);
    assert(rv);

    glAttachShader(shader_program_id, frag_id);
    glLinkProgram(shader_program_id);
    return shader_program_id;
}


u32 get_selected_mesh_index(byte* color)
{
    u32 mesh_id = 0;
    mesh_id += (255-color[0]) << 0;
    mesh_id += (255-color[1]) << 8;
    mesh_id += (255-color[2]) << 16;
    mesh_id += (255-color[3]) << 24;
    return mesh_id;
}


void draw_marquee(glm::mat4 ortho_projection, GLuint outline_shader_program_id, GLuint inside_shader_program_id)
{
    v2f start = marquee.bottom;
    v2f end = marquee.top;
    float vertices[4][2] = {
        {start.x, end.y},
        {start.x, start.y},
        {end.x, start.y},
        {end.x, end.y}
    };

    GLuint VAO;  // memleak move
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    GLuint VBO;  // memleak move
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER,
                 8 * sizeof(float),
                 vertices,
                 GL_DYNAMIC_DRAW);

    glEnable(GL_STENCIL_TEST);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, NULL);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glUseProgram(outline_shader_program_id);
    glUniformMatrix4fv(
        glGetUniformLocation(outline_shader_program_id, "ortho_projection"), 1, GL_FALSE, &ortho_projection[0][0]);

    glDrawArrays(GL_LINE_LOOP, 0, 4);

    glUseProgram(inside_shader_program_id);
    glUniformMatrix4fv(
        glGetUniformLocation(inside_shader_program_id, "ortho_projection"), 1, GL_FALSE, &ortho_projection[0][0]);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    glBindVertexArray(0);
    glDisable(GL_BLEND);
}


v2i hdpi_pixel_convert(GLFWwindow* window, float xpos, float ypos)
{
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


v2i get_mouse_pixel_coords(GLFWwindow* window)
{
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);
    return hdpi_pixel_convert(window, xpos, ypos);
}


glm::mat4 get_view_matrix()
{
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

    return View;
}


void drawMesh(Mesh &mesh, GLenum mode, GLuint shader_program_id, glm::mat4 vp)
{
    glUseProgram(shader_program_id);

    glm::mat4 mvp = vp * mesh.model_matrix;

    GLuint matrix_id = glGetUniformLocation(shader_program_id, "MVP");
    glUniformMatrix4fv(matrix_id, 1, GL_FALSE, &mvp[0][0]);

    GLuint uniform_camera_pos = glGetUniformLocation(shader_program_id, "camera_position");
    if(uniform_camera_pos)
        glUniform3fv(uniform_camera_pos, 1, &global_cam.position[0]);

    GLfloat time = glfwGetTime();
    GLuint time_id = glGetUniformLocation(shader_program_id, "time");
    glUniform1f(time_id, time);

    glBindVertexArray(mesh.vao);
    glDrawArrays(mode, 0, mesh.vertex_array_length / 3.0f);
};


void decompose_u32(u32 number, byte* array)
{
    array[0] = number & 0xFF; 
    array[1] = (number >> 8) & 0xFF;
    array[2] = (number >> 16) & 0xFF;
    array[3] = (number >> 24) & 0xFF;
}


void render_selection_buffer(GLFWwindow* window, glm::mat4 vp)
{
    // Instance mesh
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    u32 element_count = mesh_data_array.element_count;
    for (u32 i=0; i < element_count; ++i)
    {
        Mesh* mesh = (Mesh*)array_get_index(mesh_data_array, i);

        byte bytes[4];
        decompose_u32(i, bytes);
        // Create an ID from mesh index
        glm::vec4 picker_color = glm::vec4(
            255 - bytes[0], 255 - bytes[1], 255 - bytes[2], 255 - bytes[3]);

        // Draw
        glUseProgram(picker_shader_program_id);
        glm::mat4 mvp = vp * mesh->model_matrix;
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
        glBindVertexArray(mesh->vao);
        glDrawArrays(GL_TRIANGLES, 0, mesh->vertex_array_length / 3.0f);
    }
}


static void cursorPositionCallback(GLFWwindow* window, double xpos, double ypos)
{
    cursor_delta_x = xpos - last_press_x;
    cursor_delta_y = ypos - last_press_y;
    last_press_x = xpos;
    last_press_y = ypos;

    v2i px_coords = get_mouse_pixel_coords(window);

    glReadBuffer(GL_BACK);
    byte pixel_color[4];
    glReadPixels(px_coords.x, px_coords.y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, &pixel_color);
    u32 mesh_id = get_selected_mesh_index(pixel_color);

    if(mesh_id != UINT_MAX)
    {
        mouse_over_mesh = (Mesh*)array_get_index(mesh_data_array, mesh_id);
        /*print("over %i: %p", mesh_idx, mouse_over_mesh);*/
    }
    else
    {
        mouse_over_mesh = NULL;
    }
}


void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    if(button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);
        last_press_x = xpos;
        last_press_y = ypos;

        press_start_x = xpos;
        press_start_y = ypos;

        if (!mods || mods & GLFW_MOD_SHIFT)
        {
            print("Press");
            v2i pixel_coords = get_mouse_pixel_coords(window);
            glReadBuffer(GL_BACK);

            byte pixel_color[4];
            glReadPixels(pixel_coords.x, pixel_coords.y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, &pixel_color);

            u32 mesh_id = get_selected_mesh_index(pixel_color);
            print("Pixel color %hhu %hhu %hhu %hhu, mesh id %u", pixel_color[0], pixel_color[1], pixel_color[2], pixel_color[3], mesh_id);

            if(mesh_id != UINT_MAX)
            {
                print("Selected %i", mesh_id);
                bool already_selected = false;
                for (int i=0; i < selected_mesh_indices.element_count; ++i)
                {
                    u32* present_idx = (u32*)array_get_index(selected_mesh_indices, i);
                    if(*present_idx == mesh_id)
                        already_selected = true;
                }
                if (!already_selected)
                {
                    if(mods & GLFW_MOD_SHIFT)
                    {
                        array_append(selected_mesh_indices, &mesh_id);
                    }
                    else
                    {
                        array_clear(selected_mesh_indices);
                        array_append(selected_mesh_indices, &mesh_id);
                    }
                }
            }
            else
            {
                array_clear(selected_mesh_indices);
            }
            draw_viewport_marquee = true;
        }
        else if (mods & GLFW_MOD_ALT)
        {
            rotate_mode = true;
        }
        else if (mods & GLFW_MOD_CONTROL)
        {
            pan_mode = true;
            pan_vector_y = global_cam.up;
            if (pan_vector_y.y > 0)
                pan_vector_x = -getCameraRightVector(global_cam);
            else
                pan_vector_x = getCameraRightVector(global_cam);
        }
    }

    if(button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
    {
        rotate_mode = false;
        pan_mode = false;
        print("Release");
        draw_viewport_marquee = false;

        byte pixel_color[4];

        v2i top_hdpi = hdpi_pixel_convert(window, marquee.top.x, marquee.top.y);
        v2i bottom_hdpi = hdpi_pixel_convert(window, marquee.bottom.x, marquee.bottom.y);

        int width = top_hdpi.x - bottom_hdpi.x;
        int height = bottom_hdpi.y - top_hdpi.y;

        print("points %u/%u", bottom_hdpi.x, bottom_hdpi.y);
        print("width/height %i/%i", width, height);
        u32 pixel_count = width * height;
        u32 buffer_size = pixel_count * sizeof(u32);
        u32* buffer = (u32*)malloc(buffer_size);

        int window_width, window_height;
        glfwGetFramebufferSize(window, &window_width, &window_height);

        glReadBuffer(GL_BACK);
        // NOTE(kk): Need to flip bottom because the values are already stored "correctly"
        glReadPixels(bottom_hdpi.x, window_height - bottom_hdpi.y, width, height,
                     GL_RGBA, GL_UNSIGNED_BYTE, (void*)buffer);


        Array mesh_indices;
        array_init(mesh_indices, sizeof(u32), 128);

        for(u32* ptr = buffer; ptr < buffer + pixel_count; ++ptr) 
        {
            u32 boit = *ptr;
            byte bytes[4];
            decompose_u32((u32)*ptr, bytes);
            u8 r = bytes[0];
            u8 g = bytes[1];
            u8 b = bytes[2];
            u8 a = bytes[3];
            if(r*b*g*a != 0)
            {
                u32 mesh_id = get_selected_mesh_index(bytes);
                // TODO(kk): It should be really a hashmap or a set
                bool found = false;
                for (int i=0; i < mesh_indices.element_count; ++i)
                {
                    u32* mesh_index = (u32*)array_get_index(mesh_indices, i);
                    if(*mesh_index == mesh_id)
                    {
                        found = true;
                        break;
                    }
                }
                if(!found)
                {
                    array_append(mesh_indices, &mesh_id);
                }
            }
        }
        if(mesh_indices.element_count > 0)
        {
            array_clear(selected_mesh_indices);
            for (int i=0; i < mesh_indices.element_count; ++i)
            {
                u32* mesh_index = (u32*)array_get_index(mesh_indices, i);
                array_append(selected_mesh_indices, mesh_index);
            }
        }

        array_free(mesh_indices);

        free(buffer);
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
        /*Mesh cube_mesh = cube_create_random_on_sphere(xor_state);*/
        Mesh cube_mesh = cube_create_random_on_plane(xor_state);
        mesh_initialize_VAO(cube_mesh, 3);
        cube_mesh.shader_id = default_shader_program_id;
        array_append(mesh_data_array, &cube_mesh);
        /*print("count %i", mesh_data_array.element_count);*/
    }
    else if (key == GLFW_KEY_DOWN)
    {
        for (int i=1; i < 10; ++i)
        {
            array_pop(mesh_data_array);
        }
    }
    else if (key == GLFW_KEY_F and action == GLFW_PRESS)
    {
        if (selected_mesh_indices.element_count > 0)
        {
            u32* mesh_idx = (u32*)array_get_index(selected_mesh_indices, 0);
            Mesh* mesh = (Mesh*)array_get_index(mesh_data_array, *mesh_idx);
            focus_on_mesh(mesh);
        }
        else
        {
            global_cam.target = glm::vec3(0.0f, 0.0f, 0.0f);
        }
        updateCameraCoordinateFrame(global_cam);
    }
    else if (key == GLFW_KEY_E and action == GLFW_PRESS)
    {
        render_selction_buffer = true;
    }
    else if (key == GLFW_KEY_E and action == GLFW_RELEASE)
    {
        render_selction_buffer = false;
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


size_t array_defaul_resizer(void* array_pointer)
{
    Array* arr = (Array*)array_pointer;
    print("element size, %i", arr->element_size);
    print("max element count, %i", arr->max_element_count);
    print("element count, %i", arr->element_count);
    print("need twice as much, %i", arr->_block_size * 2);
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
        window_width, window_height, "OpenGL Practice", NULL, NULL);

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

    print("OpenGL version supported by this platform: %s",
          glGetString(GL_VERSION));
    print("GLSL version supported by this platform: %s",
          glGetString(GL_SHADING_LANGUAGE_VERSION));
    // END GL INIT


    Character* helvetica_characters = (Character*)malloc(sizeof(Character) * 128);
    text_initialize_font("/System/Library/Fonts/Helvetica.ttc", helvetica_characters);

    default_shader_program_id = create_shader(
        "shaders/default.vert", "shaders/default.frag");

    GLuint outline_shader_program_id = create_shader(
        "shaders/outline.vert", "shaders/outline.frag");

    hover_shader_program_id = create_shader(
        "shaders/outline.vert", "shaders/hover.frag");

    GLuint noop_shader_program_id = create_shader(
        "shaders/noop.vert", "shaders/outline.frag");

    picker_shader_program_id = create_shader(
        "shaders/default.vert", "shaders/picker.frag");

    selection_shader_program_id = create_shader(
        "shaders/default.vert", "shaders/selection.frag");

    GLuint font_shader_program_id = create_shader(
        "shaders/font.vert", "shaders/font.frag");

    GLuint marquee_outline_shader_program_id = create_shader(
        "shaders/marquee.vert", "shaders/marquee.frag");

    GLuint marquee_inside_shader_program_id = create_shader(
        "shaders/marquee.vert", "shaders/marquee_inside.frag");

    GLuint lambert_shader_program_id = create_shader(
        "shaders/default.vert", "shaders/lambert.frag");


    Array keys;
    array_init(keys, sizeof(char*) * 64, 128);

    const char* key2 = "what";
    const char* key3 = "noidea";
    const char* key1 = "something";
    const char* key4 = "meshy";

    array_append(keys, &key1);
    array_append(keys, &key2);
    array_append(keys, &key3);
    array_append(keys, &key4);

    Array values;
    array_init(values, sizeof(float) * 64, 128);

    float val1 = 10;
    float val2 = 11;
    float val3 = 12;
    float val4 = 13;

    array_append(values, &val1);
    array_append(values, &val2);
    array_append(values, &val3);
    array_append(values, &val4);

    Dict test_dict;
    dict_init(test_dict, keys, values);
    float result;
    u32 index;


    /*const char* test_123 = "what";*/
    /*float newval = 666;*/
    /*dict_map(test_dict, test_123, &newval);*/

    /*resulty = dict_get(test_dict, "what", index);*/
    /*if(resulty != NULL)*/
    /*{*/
        /*result = *(float*)resulty;*/
        /*print("%f", result);*/
        /*print("%i", index);*/
    /*}*/


    // WORLD
    glm::vec3 pos = glm::vec3(10, 8, 10);
    global_cam.position = pos;
    global_cam.target = glm::vec3(0, 0, 0);
    updateCameraCoordinateFrame(global_cam);

    u32 max_meshes = 10;
    mesh_data_array.element_size = sizeof(Mesh);
    array_init(mesh_data_array, sizeof(Mesh), max_meshes);
    mesh_data_array.resize_func = array_defaul_resizer;

    u32 max_init_selection= 100;
    array_init(selected_mesh_indices, sizeof(u32), max_init_selection);

    xor_state.a = 10;

    double current_frame = glfwGetTime();
    double last_frame= current_frame;

    Mesh suzanne_mesh = objloader_create_mesh("assets/suzanne.obj");
    suzanne_mesh.model_matrix = glm::translate(suzanne_mesh.model_matrix,
                                               glm::vec3(0,5,0));
    suzanne_mesh.model_matrix = glm::scale(suzanne_mesh.model_matrix,
                                           glm::vec3(2,2,2));
    suzanne_mesh.shader_id = lambert_shader_program_id;
    array_append(mesh_data_array, &suzanne_mesh);

    Mesh suzanne_mesh2 = objloader_create_mesh("assets/suzanne.obj");
    suzanne_mesh2.model_matrix = glm::translate(suzanne_mesh2.model_matrix,
                                               glm::vec3(5,5,0));
    suzanne_mesh2.model_matrix = glm::scale(suzanne_mesh2.model_matrix,
                                           glm::vec3(2,2,2));
    suzanne_mesh2.shader_id = lambert_shader_program_id;
    array_append(mesh_data_array, &suzanne_mesh2);

    // Crash with 3rd
    Mesh suzanne_mesh3 = objloader_create_mesh("assets/suzanne.obj");
    suzanne_mesh3.model_matrix = glm::translate(suzanne_mesh3.model_matrix,
                                               glm::vec3(-5,5,0));
    suzanne_mesh3.model_matrix = glm::scale(suzanne_mesh3.model_matrix,
                                           glm::vec3(2,2,2));
    suzanne_mesh3.shader_id = lambert_shader_program_id;
    array_append(mesh_data_array, &suzanne_mesh3);

    Mesh teapot_mesh = objloader_create_mesh("assets/teapot2.obj");
    teapot_mesh.model_matrix = glm::scale(teapot_mesh.model_matrix,
                                          glm::vec3(0.5,0.5,0.5));
    teapot_mesh.model_matrix = glm::translate(teapot_mesh.model_matrix,
                                              glm::vec3(0,-10,0));
    teapot_mesh.model_matrix = glm::rotate(teapot_mesh.model_matrix, 45.0f, glm::vec3(1,1,0));
    teapot_mesh.shader_id = lambert_shader_program_id;
    array_append(mesh_data_array, &teapot_mesh);

    // World grid
    Mesh grid_mesh = grid_create_mesh();
    mesh_initialize_VAO(grid_mesh, 3);

    Mesh manip_mesh = manipulator_create_mesh();
    mesh_initialize_VAO(manip_mesh, 3);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glEnable(GL_LINE_SMOOTH);
    glEnable(GL_STENCIL_TEST);

    unsigned int text_VAO, text_VBO;
    glGenVertexArrays(1, &text_VAO);
    glGenBuffers(1, &text_VBO);

    int* koko = (int*)malloc(1024);

    bool lock_framerate = false;

    while (!glfwWindowShouldClose(window)) {
        glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        current_frame = glfwGetTime();
        delta_time = current_frame - last_frame;
        float time_in_ms = delta_time * 1000.0f;

        // NOTE(kk): Lock framerate
        if(lock_framerate)
        {
            while (time_in_ms < 16.666f)
            {
                current_frame = glfwGetTime();
                delta_time = current_frame - last_frame;
                time_in_ms = delta_time * 1000.0f;
            }
        }

        last_frame = current_frame;

        glfwGetWindowSize(window, &window_width, &window_height);

        glm::mat4 Projection = glm::perspective(
            glm::radians(45.0f),
            (float)window_width / (float)window_height,
            0.1f, 10000.0f
        );

        glm::mat4 view_matrix = get_view_matrix();
        glm::mat4 vp = Projection * view_matrix;

        u32 element_count = mesh_data_array.element_count;

        if (render_selction_buffer)
        {
            render_selection_buffer(window, vp);
        }
        else
        {
            glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
            glStencilFunc(GL_ALWAYS, 1, 0xFF);
            glStencilMask(0xFF);

            for (int i=0; i < element_count; ++i)
            {
                Mesh* mesh = (Mesh*)array_get_index(mesh_data_array, i);
                /* legacy anim test
                u32 offset = 0;
                float ratioX = offset / float(UINT_MAX);

                glm::vec3 position = mesh->model_matrix[3];
                SphericalCoords spherical_coords = getSphericalCoords(position);
                spherical_coords.theta += (1 - ratioX) * 0.01f ;
                glm::vec3 new_position = getCartesianCoords(spherical_coords);
                glm::vec3 diff = new_position - position;
                diff[1] += current_frame / 200.0f;

                mesh->model_matrix = glm::translate(mesh->model_matrix, diff);
                */

                GLuint object_shader = mesh->shader_id;
                drawMesh(*mesh, GL_TRIANGLES, object_shader, vp);
            }


            bool active_selection = 0;
            // STENCIL
            for (int i=0; i < element_count; ++i)
            {
                Mesh* mesh = (Mesh*)array_get_index(mesh_data_array, i);

                glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
                glStencilMask(0x00);
                glDisable(GL_DEPTH_TEST);

                bool is_selected = false;

                for (int j=0; j < selected_mesh_indices.element_count; ++j)
                {
                    u32* selected_idx = (u32*)array_get_index(selected_mesh_indices, j);
                    if(*selected_idx == i)
                        is_selected = true;
                }

                if (is_selected || mesh == mouse_over_mesh)
                {
                    GLuint shader_id = 0;
                    if (is_selected)
                    {
                        shader_id = outline_shader_program_id;
                        manip_mesh.model_matrix = mesh->model_matrix;
                        active_selection = 1;
                    }
                    if (mesh == mouse_over_mesh)
                    {
                        shader_id = hover_shader_program_id;
                    }

                    glUseProgram(shader_id);

                    glm::mat4 mvp = vp * mesh->model_matrix;
                    GLuint matrix_id = glGetUniformLocation(shader_id, "MVP");
                    glUniformMatrix4fv(matrix_id, 1, GL_FALSE, &mvp[0][0]);

                    GLuint uniform_camera_pos = glGetUniformLocation(shader_id, "camera_position");
                    if(uniform_camera_pos)
                        glUniform3fv(uniform_camera_pos, 1, &global_cam.position[0]);

                    glBindVertexArray(mesh->vao);
                    glDrawArrays(GL_TRIANGLES, 0, mesh->vertex_array_length / 3.0f);

                }
                glBindVertexArray(0);
                glStencilMask(0xFF);
                glStencilFunc(GL_ALWAYS, 0, 0xFF);
                glEnable(GL_DEPTH_TEST);

            }
            // World grid
            if(active_selection)
            {
                glDisable(GL_DEPTH_TEST);
                glDisable(GL_CULL_FACE);
                drawMesh(manip_mesh, GL_TRIANGLES, default_shader_program_id, vp);
                glEnable(GL_DEPTH_TEST);
                glEnable(GL_CULL_FACE);
            }
            drawMesh(grid_mesh, GL_LINES, default_shader_program_id, vp);
        }

        // Text
        glm::mat4 ortho_projection = glm::ortho(0.0f, (float)window_width, 0.0f, (float)window_height);

        char text[8];
        sprintf(text, "%.2fms", time_in_ms);
        float redx = fmax(0, time_in_ms - 16.666f);

        glm::vec3 color = glm::vec3(0.3f + redx/10.f, 0.8f, 0.4f);
        glm::vec2 pos = glm::vec2(5, 10);
        float scale = .5f;

        text_draw(text, color, pos, scale, helvetica_characters, ortho_projection, font_shader_program_id);

        scale = 0.5f;
        pos = glm::vec2(window_width/2 - helvetica_characters[0].size.x * strlen(text), 10);
        text_draw("OpenGL test", color, pos, scale, helvetica_characters, ortho_projection, font_shader_program_id);

        if(draw_viewport_marquee)
        {
            v2f p1;
            v2f p2;

            float norm_press_start_y = window_height - press_start_y;
            float norm_last_press_y = window_height - last_press_y;

            p1.x = fmin(press_start_x, last_press_x);
            p1.y = fmin(norm_press_start_y, norm_last_press_y);

            p2.x = fmin(fmax(press_start_x, last_press_x), window_width);
            p2.y = fmin(fmax(norm_press_start_y, norm_last_press_y), window_height);
            marquee.bottom.x = p1.x;
            marquee.bottom.y = p1.y;
            marquee.top.x = p2.x;
            marquee.top.y = p2.y;

            draw_marquee(ortho_projection, marquee_outline_shader_program_id, marquee_inside_shader_program_id);
        }

        glfwSwapBuffers(window);

        // NOTE(kk): render selection back buffer before polling events
        render_selection_buffer(window, vp);
        glfwPollEvents();
    }


    array_free(mesh_data_array);

    glfwTerminate();
    return 0;
}
