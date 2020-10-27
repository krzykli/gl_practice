// TODOs
// - Manipulators
// - cleanup text rendering
// - Indexed draws
// - picker shader fixes
// - dict struct
// - lights
// - transform stack?
// - bboxes
// - UI widgets
// - gradient background
//
#include <cmath>
#include <thread>
#include <pthread.h>

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtc/matrix_inverse.hpp>

#include "types.h"
#include "debug.h"
#include "ray.c"
#include "camera.h"
#include "array.h"
#include "dict.h"
#include "mesh.c"
#include "text.h"
#include "background.c"

#include "io/objloader.h"

#include "assets/grid.h"
#include "assets/cube.h"
#include "assets/manipulator.h"

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

static float RAY_MAX_DISTANCE = 999999999.0f;

static bool render_selction_buffer = false;
static bool draw_viewport_marquee = false;

static Array selected_mesh_indices;
static Mesh* mouse_over_mesh = NULL;

static Array rays;

static bool is_running = true;
static bool render_view = false;

const float TWO_M_PI = M_PI*2.0f;
const float M_PI_OVER_TWO = M_PI/2.0f;

GLuint default_shader_program_id;
GLuint hover_shader_program_id;
GLuint picker_shader_program_id;
GLuint selection_shader_program_id;

enum tool {NONE, TRANSLATE};
const char* ToolNames[] = {"NONE", "TRANSLATE"};
static tool current_tool = NONE;

enum selection_mode {OBJECT, FACE, VERTEX};
static selection_mode current_selection_mode = OBJECT;


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

/*typedef struct BVH*/
/*{*/
     
/*};*/


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


void draw_marquee(GLuint vao, GLuint vbo, glm::mat4 ortho_projection, GLuint outline_shader_program_id, GLuint inside_shader_program_id)
{
    v2f start = marquee.bottom;
    v2f end = marquee.top;

    float vertices[4][2] = {
        {start.x, end.y},
        {start.x, start.y},
        {end.x, start.y},
        {end.x, end.y}
    };

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glBindVertexArray(vao);

    // update array buffer with new point data
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glUseProgram(inside_shader_program_id);
    glUniformMatrix4fv(
        glGetUniformLocation(inside_shader_program_id, "ortho_projection"), 1, GL_FALSE, &ortho_projection[0][0]);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    byte pad = 2;
    float vertices_padded[4][2] = {
        {start.x - pad , end.y + pad},
        {start.x - pad , start.y - pad},
        {end.x + pad , start.y - pad},
        {end.x + pad , end.y + pad}
    };

    // update array buffer with new point data
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices_padded), vertices_padded);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glUseProgram(outline_shader_program_id);
    glUniformMatrix4fv(
        glGetUniformLocation(outline_shader_program_id, "ortho_projection"), 1, GL_FALSE, &ortho_projection[0][0]);
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
    glBindVertexArray(0);
    glUseProgram(0);
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
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if(current_tool == NONE)
    {
        if(current_selection_mode == OBJECT)
        {
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
                glBindVertexArray(0);
            }
        }
    }
    else if(current_tool == TRANSLATE)
    {
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

    /*print("Press %f, %f", xpos, ypos);*/
    v2i pixel_coords = get_mouse_pixel_coords(window);

    int buffer_width, buffer_height;
    glfwGetFramebufferSize(window, &buffer_width, &buffer_height);
    float u, v;
    u = pixel_coords.x / (float)buffer_width;
    v = pixel_coords.y / (float)buffer_height;
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
            camera_update(global_cam);
            print("Press %f, %f", xpos, ypos);
            v2i pixel_coords = get_mouse_pixel_coords(window);

            int buffer_width, buffer_height;
            glfwGetFramebufferSize(window, &buffer_width, &buffer_height);
            float u, v;
            u = pixel_coords.x / (float)buffer_width;
            v = pixel_coords.y / (float)buffer_height;

            Ray r = camera_shoot_ray(global_cam, u, v);
            /*array_append(rays, &r);*/

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
    if (key == GLFW_KEY_ESCAPE)
    {
        is_running = false;
    }
    else if (key == GLFW_KEY_Q)
    {
        current_tool = NONE;
    }
    else if (key == GLFW_KEY_W)
    {
        current_tool = TRANSLATE;
    }
    else if (key == GLFW_KEY_UP)
    {
        /*Mesh cube_mesh = cube_create_random_on_sphere(xor_state);*/
        Mesh cube_mesh = cube_create_random_on_plane(xor_state);
        mesh_init(cube_mesh, 3);
        cube_mesh.shader_id = default_shader_program_id;
        array_append(mesh_data_array, &cube_mesh);
    }
    else if (key == GLFW_KEY_DOWN)
    {
        for (int i=1; i < 10; ++i)
        {
            array_pop(mesh_data_array);
        }
    }

    if(action == GLFW_PRESS)
    {
        if (key == GLFW_KEY_A)
        {
            render_selction_buffer = true;
        }
        else if (key == GLFW_KEY_SPACE)
        {
            render_view = !render_view;
        }
        else if (key == GLFW_KEY_F)
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
            camera_update(global_cam);
        }
    }
    else if (action == GLFW_RELEASE)
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


void prepare_meshes_for_render()
{
    for (int i=0; i < mesh_data_array.element_count; ++i)
    {
        Mesh* mesh = (Mesh*)array_get_index(mesh_data_array, i);
        glm::mat4 inverse_model_matrix = glm::inverse(mesh->model_matrix);
        glm::mat4 inverse_transpose_model_matrix = glm::inverseTranspose(mesh->model_matrix);
        mesh->inverse_model_matrix = inverse_model_matrix;
        mesh->inverse_transpose_model_matrix = inverse_transpose_model_matrix;
    }
}

// TODO multithread buckets
void trace_ray(float u, float v, HitRecord &closest_hit)
{
    Ray r = camera_shoot_ray(global_cam, u, v);
    for (int i=0; i < mesh_data_array.element_count; ++i)
    {
        Mesh* mesh = (Mesh*)array_get_index(mesh_data_array, i);
        glm::mat4 inverse_model_matrix = mesh->inverse_model_matrix;
        glm::vec3 vmin = glm::vec3(mesh->bbox[0], mesh->bbox[1], mesh->bbox[2]);
        glm::vec3 vmax = glm::vec3(mesh->bbox[3], mesh->bbox[4], mesh->bbox[5]);

        // Modify the ray to intersect transformed mesh data
        Ray changed_ray;
        changed_ray.origin = glm::vec3(inverse_model_matrix * glm::vec4(r.origin, 1));

        // Homogenous coordinate must be set to 0 for vectors
        /*https://online.ucsd.edu/courses/course-v1:CSE+168X+2020-SP/courseware/Unit_3/L10/1?activate_block_id=block-v1%3ACSE%2B168X%2B2020-SP%2Btype%40html%2Bblock%40video_l10v1*/
        changed_ray.direction = glm::vec3(inverse_model_matrix * glm::vec4(r.direction, 0));

        bool box_hit = ray_intersect_box(changed_ray, vmin, vmax);
        if(!box_hit)
            continue;

        for (u32 c=0; c<mesh->vertex_array_length; c += 9)
        {
            // Move to prepare mesh and construct render data triangles
            Triangle tri;
            tri.A = glm::vec3(mesh->vertex_positions[c], mesh->vertex_positions[c+1], mesh->vertex_positions[c+2]);
            tri.B = glm::vec3(mesh->vertex_positions[c+3], mesh->vertex_positions[c+4], mesh->vertex_positions[c+5]);
            tri.C = glm::vec3(mesh->vertex_positions[c+6], mesh->vertex_positions[c+7], mesh->vertex_positions[c+8]);

            HitRecord this_hit_record;
            this_hit_record.t = RAY_MAX_DISTANCE;
            this_hit_record.p = glm::vec3(0);
            this_hit_record.normal= glm::vec3(0);
            bool intersect = ray_intersect_triangle(changed_ray, tri, 0.001f, 10000.0f, this_hit_record);

            if (intersect && this_hit_record.t < closest_hit.t)
            {
                closest_hit.t = this_hit_record.t;
                closest_hit.p = glm::vec3(mesh->model_matrix * glm::vec4(this_hit_record.p, 1));
                closest_hit.normal = glm::normalize(glm::vec3(mesh->inverse_transpose_model_matrix * glm::vec4(this_hit_record.normal, 1)));
            }
        }
    }
}

typedef struct RenderThreadArgs
{
    float  u;
    float  v;
    HitRecord hit_record;
} RenderThreadArgs;



typedef struct ImageBuffer
{
     u32* buffer;
     u32 width;
     u32 height;
} ImageBuffer;


typedef struct Bucket
{
     u32 xmin;
     u32 ymin;
     u32 xmax;
     u32 ymax;
} Bucket;


typedef struct RenderRequest
{
    ImageBuffer image_buffer;
    Bucket bucket;
} RenderRequest;


typedef struct RenderQueue
{
    u32 request_count;
    RenderRequest *requests;
    volatile u32 requests_rendered;
    volatile u32 next_request_index;
} RenderQueue;


u32* get_image_pixel(ImageBuffer image, u32 x, u32 y)
{
     return image.buffer + y * image.width + x;
}

u32 lock_add(volatile u32 *a, u32 b)
{
    pthread_t tid = pthread_self();
    // print("From the function, the thread id %d", tid);
    return __sync_fetch_and_add(a, b);
}


bool raycast(RenderQueue *queue)
{
    u32 request_index = lock_add(&queue->next_request_index, 1);
    // print("Index %i", request_index);
    // print("queue %i", queue->request_count);
    if(request_index > queue->request_count)
    {
        // print("thread done");
        return false;
    }
    RenderRequest rr = queue->requests[request_index];

    ImageBuffer image = rr.image_buffer;
    Bucket bucket = rr.bucket;
    float u, v;
    // print("Rendering %ux%u", image.width, image.height);
    for(int j=bucket.ymin; j < bucket.ymax; ++j)
    {
        for(int i=bucket.xmin; i < bucket.xmax; ++i)
        {
            u = i / (float)image.width;
            v = j / (float)image.height;

            HitRecord hit_result;
            hit_result.t = RAY_MAX_DISTANCE;
            hit_result.p = glm::vec3(0);
            hit_result.normal= glm::vec3(0);

            trace_ray(u, v, hit_result);

            if(hit_result.t != RAY_MAX_DISTANCE)
            {

                glm::vec3 normal = hit_result.normal;
                u8 colorR = (u8)((normal.x + 1) / 2 * 255.0f);
                u8 colorG = (u8)((normal.y + 1) / 2 * 255.0f);
                u8 colorB = (u8)((normal.z + 1) / 2 * 255.0f);
                u8 alpha = 255;

                u32 final = colorR | (colorG << 8) | (colorB << 16) | (alpha << 24);

                /*print("normal %f %f %f", normal.x, normal.y, normal.z);*/
                /*print("normal rescaled %f %f %f", (normal.x + 1) / 2, (normal.y + 1) / 2, (normal.z + 1) / 2);*/
                /*print("%u %u %u - %04X", colorR, colorG, colorB, final);*/
                image.buffer[j * image.width + i] = final;
            }
            else
            {
                image.buffer[j * image.width + i] = 0x0;
            }
        }
    }
    lock_add(&queue->requests_rendered, 1);
    return true;
}


void* raycast_thread(void* args)
{
     RenderQueue *queue = (RenderQueue*)args;
     while(raycast(queue)) {};
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

    GLuint background_shader_program_id = create_shader(
        "shaders/background.vert", "shaders/background.frag");

    GLuint render_shader_program_id = create_shader(
        "shaders/render.vert", "shaders/render.frag");

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
    glm::vec3 cam_start_pos = glm::vec3(10, 8, 10);
    glm::vec3 cam_start_target = glm::vec3(0, 0, 0);
    global_cam.position = cam_start_pos;
    global_cam.target = cam_start_target;
    global_cam.aspect_ratio = (float)window_width / (float)window_height;
    global_cam.fov = 45.0f;
    camera_update(global_cam);

    u32 max_meshes = 10;
    mesh_data_array.element_size = sizeof(Mesh);
    array_init(mesh_data_array, sizeof(Mesh), max_meshes);
    mesh_data_array.resize_func = array_defaul_resizer;

    u32 max_init_selection= 100;
    array_init(selected_mesh_indices, sizeof(u32), max_init_selection);

    u32 max_rays= 100;
    array_init(rays, sizeof(Ray), max_rays);

    xor_state.a = 10;

    double current_frame = glfwGetTime();
    double last_frame= current_frame;

    Mesh suzanne_mesh = objloader_create_mesh("assets/suzanne.obj");
    suzanne_mesh.model_matrix = glm::translate(suzanne_mesh.model_matrix,
                                               glm::vec3(0,5,0));
    /*suzanne_mesh.model_matrix = glm::scale(suzanne_mesh.model_matrix,*/
                                           /*glm::vec3(2,2,2));*/
    suzanne_mesh.shader_id = lambert_shader_program_id;
    array_append(mesh_data_array, &suzanne_mesh);

    Mesh suzanne_mesh2 = objloader_create_mesh("assets/suzanne.obj");
    suzanne_mesh2.model_matrix = glm::translate(suzanne_mesh2.model_matrix,
                                               glm::vec3(5,5,0));
    /*suzanne_mesh2.model_matrix = glm::scale(suzanne_mesh2.model_matrix,*/
                                           /*glm::vec3(2,2,2));*/
    suzanne_mesh2.shader_id = lambert_shader_program_id;
    array_append(mesh_data_array, &suzanne_mesh2);

    // Crash with 3rd
    Mesh suzanne_mesh3 = objloader_create_mesh("assets/suzanne.obj");
    suzanne_mesh3.model_matrix = glm::translate(suzanne_mesh3.model_matrix,
                                               glm::vec3(-5,5,0));
    /*suzanne_mesh3.model_matrix = glm::scale(suzanne_mesh3.model_matrix,*/
                                           /*glm::vec3(2,2,2));*/
    suzanne_mesh3.shader_id = lambert_shader_program_id;
    array_append(mesh_data_array, &suzanne_mesh3);

    Mesh teapot_mesh = objloader_create_mesh("assets/teapot2.obj");
    /*teapot_mesh.model_matrix = glm::scale(teapot_mesh.model_matrix,*/
                                          /*glm::vec3(0.5,0.5,0.5));*/
    teapot_mesh.model_matrix = glm::translate(teapot_mesh.model_matrix,
                                              glm::vec3(0,-10,0));
    //teapot_mesh.model_matrix = glm::rotate(teapot_mesh.model_matrix, 45.0f, glm::vec3(1,1,0));
    teapot_mesh.shader_id = lambert_shader_program_id;
    array_append(mesh_data_array, &teapot_mesh);

    // World grid
    Mesh grid_mesh = grid_create_mesh();
    mesh_init(grid_mesh, 3);

    Mesh manip_mesh = manipulator_create_mesh();
    manip_mesh.shader_id = lambert_shader_program_id;
    mesh_init(manip_mesh, 3);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glEnable(GL_LINE_SMOOTH);
    glEnable(GL_STENCIL_TEST);

    unsigned int text_VAO, text_VBO;
    glGenVertexArrays(1, &text_VAO);
    glGenBuffers(1, &text_VBO);

    GLuint background_VAO = background_init_vao();

    // marquee init VAO
    GLuint marquee_VAO;
    glGenVertexArrays(1, &marquee_VAO);
    glBindVertexArray(marquee_VAO);

    GLuint marquee_VBO;
    glGenBuffers(1, &marquee_VBO);
    glBindBuffer(GL_ARRAY_BUFFER, marquee_VBO);
    glBufferData(GL_ARRAY_BUFFER,
                 8 * sizeof(float),
                 NULL,  // will be replaced by sub data
                 GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, NULL);
    glBindVertexArray(0);


    int frame_width, frame_height;
    glfwGetWindowSize(window, &frame_width, &frame_height);

    u32 buffer_width = frame_width / 2;
    u32 buffer_height = frame_height / 2;

    ImageBuffer render_image;
    render_image.buffer = (u32*)malloc(buffer_width * buffer_height * sizeof(u32));
    render_image.width = buffer_width;
    render_image.height = buffer_height;

    unsigned int render_texture;
    glGenTextures(1, &render_texture);
    glBindTexture(GL_TEXTURE_2D, render_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glUseProgram(render_shader_program_id);
    glUniform1i(glGetUniformLocation(render_shader_program_id, "texture1"), 0);
    glUseProgram(0);

    glBindTexture(GL_TEXTURE_2D, 0);

    // https://learnopengl.com/code_viewer_gh.php?code=src/1.getting_started/4.2.textures_combined/textures_combined.cpp
    float render_vertices[] = {
        // positions          // colors           // texture coords
        -1.0f, 1.0f, 0.0f,   1.0f, 0.0f, 0.0f,   0.0f, 1.0f, // top right
        -1.0f, -1.0f, 0.0f,  0.0f, 1.0f, 0.0f,   0.0f, 0.0f, // bottom right
        1.0f, -1.0f, 0.0f,   0.0f, 0.0f, 1.0f,   1.0f, 0.0f, // bottom left
        1.0f,  1.0f, 0.0f,   1.0f, 1.0f, 0.0f,   1.0f, 1.0f  // top left 
    };

    u32 render_indices[] = {
        0, 1, 3, // first triangle
        1, 2, 3  // second triangle
    };

    GLuint render_VAO, render_VBO, render_EBO;
    glGenVertexArrays(1, &render_VAO);
    glGenBuffers(1, &render_VBO);
    glGenBuffers(1, &render_EBO);

    glBindVertexArray(render_VAO);

    glBindBuffer(GL_ARRAY_BUFFER, render_VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(render_vertices), render_vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, render_EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(render_indices), render_indices, GL_STATIC_DRAW);

    glBindTexture(GL_TEXTURE_2D, render_texture);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, render_image.width, render_image.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glBindTexture(GL_TEXTURE_2D, 0);

    // position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // color attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    // texture coord attribute
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);


    bool lock_framerate = false;
    while (is_running) {
        if (glfwWindowShouldClose(window))
        {
             is_running = false;
             break;
        }

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

        // background
        glUseProgram(background_shader_program_id);
        glDisable(GL_DEPTH_TEST);
            glBindVertexArray(background_VAO);
            glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
            glBindVertexArray(0);
        glEnable(GL_DEPTH_TEST);
        glUseProgram(0);
        // background

        last_frame = current_frame;

        glfwGetWindowSize(window, &window_width, &window_height);

        glm::mat4 Projection = glm::perspective(
            glm::radians(global_cam.fov),
            global_cam.aspect_ratio,
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

            if(render_view)
            {
                camera_update(global_cam);
                int core_count = std::thread::hardware_concurrency();
                //print("CPU count %i", core_count);
                //print("Preparing meshes...");
                prepare_meshes_for_render();

                u32 bucket_size = 32;
                u32 buckets_x = ceil((render_image.width) / (float)bucket_size);
                u32 buckets_y = ceil((render_image.height) / (float)bucket_size);

                u32 bucket_count = buckets_x * buckets_y;

                RenderRequest requests[bucket_count];

                u32 idx = 0;
                for (u32 i=0; i < buckets_x; ++i)
                {
                    for (u32 j=0; j < buckets_y; ++j)
                    {
                        u32 xmin = i*bucket_size;
                        u32 ymin = j*bucket_size;
                        u32 xmax = fmin((i+1)*bucket_size, render_image.width);
                        u32 ymax = fmin((j+1)*bucket_size, render_image.height);
                        Bucket buc = {xmin, ymin, xmax, ymax};
                        RenderRequest rr;
                        rr.image_buffer= render_image;
                        rr.bucket = buc;
                        requests[idx] = rr;
                        idx++;
                    }
                }

                RenderQueue queue;
                queue.request_count = bucket_count;
                queue.requests = requests;
                queue.requests_rendered = 0;
                queue.next_request_index = 0;

                u32 thread_count = core_count - 1;
                pthread_t threads[thread_count];

                for(u32 core_id = 1; core_id < core_count; ++core_id)
                {
                     pthread_t thread_id;
                     // print("creating thread");
                     pthread_create(&thread_id, NULL, raycast_thread, (void*)&queue);
                     threads[core_id - 1] = thread_id;
                }

                while(raycast(&queue)) {};

                for(u32 i = 0; i < thread_count; ++i)
                {
                    pthread_join(threads[i], NULL);
                }

                last_frame = current_frame;
                // print("Render done in %f ms", time_in_ms);

                //glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
                glDisable(GL_DEPTH_TEST);
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

                // bind textures on corresponding texture units
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, render_texture);

                // NOTE(kk): Change this per bucket?
                glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, render_image.width, render_image.height, GL_RGBA, GL_UNSIGNED_BYTE, render_image.buffer);

                // render container
                glUseProgram(render_shader_program_id);
                glBindVertexArray(render_VAO);
                glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
                glUseProgram(0);
                glBindTexture(GL_TEXTURE_2D, 0);
                glDisable(GL_BLEND);
                glEnable(GL_DEPTH_TEST);
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
                        glm::vec3 scale;
                        glm::quat rotation;
                        glm::vec3 translation;
                        glm::vec3 skew;
                        glm::vec4 perspective;
                        glm::decompose(mesh->model_matrix, scale, rotation, translation, skew, perspective);
                        glm::mat4 rotation_matrix = glm::mat4_cast(rotation);
                        glm::vec3 offset_x = translation;
                        /*glm::mat4 rotation_with_pivot = glm::translate(glm::mat4(1), offset_x) * rotation_matrix * glm::translate(glm::mat4(1), -offset_x);*/
                        manip_mesh.model_matrix = glm::translate(glm::mat4(1), offset_x);
                        manip_mesh.model_matrix = manip_mesh.model_matrix * rotation_matrix;
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

                    /*GLuint uniform_volume = glGetUniformLocation(shader_id, "volume");*/
                    /*if(uniform_volume)*/
                    /*{*/
                        /*float width = mesh->bbox[3] - mesh->bbox[0];*/
                        /*float height = mesh->bbox[4] - mesh->bbox[1];*/
                        /*float depth = mesh->bbox[5] - mesh->bbox[2];*/
                        /*glUniform1f(uniform_volume, width * height * depth);*/
                    /*}*/

                    glBindVertexArray(mesh->vao);
                    glDrawArrays(GL_TRIANGLES, 0, mesh->vertex_array_length / 3.0f);
                    glBindVertexArray(0);
                    glUseProgram(0);
                }
                glBindVertexArray(0);
                glStencilMask(0xFF);
                glStencilFunc(GL_ALWAYS, 0, 0xFF);
                glEnable(GL_DEPTH_TEST);
            }

            if(active_selection)
            {
                for (int i=0; i < selected_mesh_indices.element_count; ++i)
                {
                    u32* selected_idx = (u32*)array_get_index(selected_mesh_indices, i);
                    Mesh* mesh = (Mesh*)array_get_index(mesh_data_array, *selected_idx);
                    mesh_draw_bbox(*mesh, default_shader_program_id, vp);
                }

                // Manipulator
                if(current_tool == TRANSLATE)
                {
                    glDisable(GL_DEPTH_TEST);
                    glDisable(GL_CULL_FACE);
                    glm::mat4 manip_view = view_matrix;
                    vp = Projection * manip_view;
                    drawMesh(manip_mesh, GL_TRIANGLES, default_shader_program_id, vp);
                    glEnable(GL_DEPTH_TEST);
                    glEnable(GL_CULL_FACE);
                }
            }

            // World grid
            Mesh* mesh = (Mesh*)array_get_index(mesh_data_array, 1);
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

        scale = 0.3f;
        char text_tool[32];
        pos = glm::vec2(10, window_height - 15);
        sprintf(text_tool, "Tool: %s", ToolNames[current_tool]);
        text_draw(text_tool, color, pos, scale, helvetica_characters, ortho_projection, font_shader_program_id);

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

            draw_marquee(marquee_VAO, marquee_VBO, ortho_projection, marquee_outline_shader_program_id, marquee_inside_shader_program_id);
        }

        glfwSwapBuffers(window);

        render_selection_buffer(window, vp);
        /*// NOTE(kk): render selection back render_buffer before polling events*/
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &render_VAO);
    glDeleteBuffers(1, &render_VBO);
    glDeleteBuffers(1, &render_EBO);


    array_free(mesh_data_array);
    array_free(selected_mesh_indices);
    free(render_image.buffer);

    glfwTerminate();
    return 0;
}
