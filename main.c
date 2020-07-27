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

#include <ft2build.h>
#include FT_FREETYPE_H
FT_Library  library;

#include "types.h"
#include "debug.h"
#include "camera.h"
#include "array.h"
#include "mesh.h"

#include "assets/grid.h"
#include "assets/cube.h"
#include "io/objloader.h"

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

static bool render_selction_buffer = false;

static Mesh* selected_mesh = NULL;
static Mesh* mouse_over_mesh = NULL;

const float TWO_M_PI = M_PI*2.0f;
const float M_PI_OVER_TWO = M_PI/2.0f;

GLuint picker_shader_program_id;
GLuint hover_shader_program_id;
GLuint selection_shader_program_id;



u32 get_selected_mesh_index(byte* color)
{
    u32 mesh_id = 0;
    mesh_id += (255-color[0]) << 0;
    mesh_id += (255-color[1]) << 8;
    mesh_id += (255-color[2]) << 16;
    mesh_id += (255-color[3]) << 24;
    return mesh_id;
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

    GLMesh gl_mesh;
    gl_mesh.mesh = &mesh;
    gl_mesh.vao = vao;
    return gl_mesh;
}


void drawGLMesh(GLMesh &gl_mesh, GLenum mode, GLuint shader_program_id, glm::mat4 vp)
{
    glUseProgram(shader_program_id);

    glm::mat4 mvp = vp * gl_mesh.mesh->model_matrix;

    GLuint matrix_id = glGetUniformLocation(
        shader_program_id, "MVP");
    glUniformMatrix4fv(matrix_id, 1, GL_FALSE, &mvp[0][0]);

    GLfloat time = glfwGetTime();
    GLuint time_id = glGetUniformLocation(shader_program_id, "time");
    glUniform1f(time_id, time);

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


void render_selection_buffer(GLFWwindow* window, glm::mat4 vp, GLMesh* glmesh)
{
    // Instance mesh
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    u32 element_count = mesh_data_array.element_count;
    for (u32 i=0; i < element_count; ++i)
    {
        Mesh* cube_mesh = (Mesh*)array_get_index(mesh_data_array, i);
        glmesh->mesh = cube_mesh;

        byte bytes[4];
        decompose_u32(i, bytes);
        // Create an ID from mesh index
        glm::vec4 picker_color = glm::vec4(255 - bytes[0], 255 - bytes[1], 255 - bytes[2], 255 - bytes[3]);

        // Draw
        glUseProgram(picker_shader_program_id);
        glm::mat4 mvp = vp * glmesh->mesh->model_matrix;
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
        glBindVertexArray(glmesh->vao);
        glDrawArrays(GL_TRIANGLES, 0, glmesh->mesh->vertex_array_length / 3.0f);
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
        /*printf("over %i: %p\n", mesh_idx, mouse_over_mesh);*/
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
        v2i pixel_coords = get_mouse_pixel_coords(window);
        glReadBuffer(GL_BACK);

        byte pixel_color[4];
        glReadPixels(pixel_coords.x, pixel_coords.y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, &pixel_color);

        u32 mesh_id = get_selected_mesh_index(pixel_color);
        printf("Pixel color %hhu %hhu %hhu %hhu, mesh id %u\n", pixel_color[0], pixel_color[1], pixel_color[2], pixel_color[3], mesh_id);

        if(mesh_id != UINT_MAX)
        {
            printf("Selected %i\n", mesh_id);
            selected_mesh = (Mesh*)array_get_index(mesh_data_array, mesh_id);
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
        /*Mesh cube_mesh = cube_create_random_on_sphere(xor_state);*/
        Mesh cube_mesh = cube_create_random_on_plane(xor_state);
        array_append(mesh_data_array, &cube_mesh);
        /*printf("count %i\n", mesh_data_array.element_count);*/
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
        if (selected_mesh != NULL)
        {
            focus_on_mesh(selected_mesh);
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


struct Character {
    unsigned int textureID;  // ID handle of the glyph texture
    glm::ivec2   size;       // Size of glyph
    glm::ivec2   bearing;    // Offset from baseline to left/top of glyph
    unsigned int advance;    // Offset to advance to next glyph
};

static Character characters[128];

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

    // FREETYPE INIT
    u32 error;
    error = FT_Init_FreeType(&library);
    if (error)
    {
        printf("Error initializing FreeType\n");
    }

    FT_Face face;
    error = FT_New_Face(library,
                        "/System/Library/Fonts/Helvetica.ttc",
                        0,
                        &face );
    if (error)
    {
         printf("Failed to initialize font\n");
    }
    /*error = FT_Set_Char_Size(*/
          /*face,    [> handle to face object           <]*/
          /*0,       [> char_width in 1/64th of points  <]*/
          /*16*64,   [> char_height in 1/64th of points <]*/
          /*96,     [> dpi horizontal device resolution    <]*/
          /*96);   [> dpi vertical device resolution      <]*/

    int font_size = 48;
    FT_Set_Pixel_Sizes(face, 0, font_size);  

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // disable byte-alignment restriction
    for (unsigned char c = 0; c < 128; ++c)
    {
        // load character glyph 
        if (FT_Load_Char(face, c, FT_LOAD_RENDER))
        {
            printf("ERROR::FREETYTPE: Failed to load Glyph\n");
            continue;
        }
        // generate texture
        unsigned int texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RED,
            face->glyph->bitmap.width,
            face->glyph->bitmap.rows,
            0,
            GL_RED,
            GL_UNSIGNED_BYTE,
            face->glyph->bitmap.buffer
        );
        // set texture options
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        // now store character for later use
        Character character = {
            texture,
            glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
            glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
            face->glyph->advance.x
        };
        characters[byte(c)] = character;
    }
    FT_Done_Face(face);
    FT_Done_FreeType(library);

    // END FREETYPE INIT

    Array vertex_array;
    array_init(vertex_array, sizeof(float), 1024*1024);
    Array uv_array;
    array_init(uv_array, sizeof(float), 1024*1024);
    Array normals_array;
    array_init(normals_array, sizeof(float), 1024*1024);

    const char* file_path = "assets/teapot2.obj";
    objloader_load(file_path, vertex_array, uv_array, normals_array);


    // VERTEX SHADERS
    GLuint default_vert_id = glCreateShader(GL_VERTEX_SHADER);
    u8 rv = compileShader(default_vert_id, "shaders/default.vert");
    assert(rv);

    GLuint noop_vert_id = glCreateShader(GL_VERTEX_SHADER);
    rv = compileShader(noop_vert_id, "shaders/noop.vert");
    assert(rv);

    GLuint outline_vert_id = glCreateShader(GL_VERTEX_SHADER);
    rv = compileShader(outline_vert_id, "shaders/outline.vert");
    assert(rv);

    GLuint font_vert_id = glCreateShader(GL_VERTEX_SHADER);
    rv = compileShader(font_vert_id, "shaders/font.vert");
    assert(rv);

    // FRAGMENT SHADERS
    GLuint default_frag_id = glCreateShader(GL_FRAGMENT_SHADER);
    rv = compileShader(default_frag_id, "shaders/default.frag");
    assert(rv);

    GLuint outline_frag_id = glCreateShader(GL_FRAGMENT_SHADER);
    rv = compileShader(outline_frag_id, "shaders/outline.frag");
    assert(rv);

    GLuint picker_frag_id = glCreateShader(GL_FRAGMENT_SHADER);
    rv = compileShader(picker_frag_id, "shaders/picker.frag");
    assert(rv);

    GLuint selection_frag_id = glCreateShader(GL_FRAGMENT_SHADER);
    rv = compileShader(selection_frag_id, "shaders/selection.frag");
    assert(rv);

    GLuint hover_frag_id = glCreateShader(GL_FRAGMENT_SHADER);
    rv = compileShader(hover_frag_id, "shaders/hover.frag");
    assert(rv);

    GLuint font_frag_id = glCreateShader(GL_FRAGMENT_SHADER);
    rv = compileShader(font_frag_id, "shaders/font.frag");
    assert(rv);

    GLuint lambert_frag_id = glCreateShader(GL_FRAGMENT_SHADER);
    rv = compileShader(font_frag_id, "shaders/lambert.frag");
    assert(rv);

    GLuint default_shader_program_id = glCreateProgram();
    glAttachShader(default_shader_program_id, default_vert_id);
    glAttachShader(default_shader_program_id, default_frag_id);
    glLinkProgram(default_shader_program_id);

    GLuint outline_shader_program_id = glCreateProgram();
    glAttachShader(outline_shader_program_id, outline_vert_id);
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

    hover_shader_program_id = glCreateProgram();
    glAttachShader(hover_shader_program_id, default_vert_id);
    glAttachShader(hover_shader_program_id, hover_frag_id);
    glLinkProgram(hover_shader_program_id);

    selection_shader_program_id = glCreateProgram();
    glAttachShader(selection_shader_program_id, default_vert_id);
    glAttachShader(selection_shader_program_id, selection_frag_id);
    glLinkProgram(selection_shader_program_id);

    GLuint font_shader_program_id = glCreateProgram();
    glAttachShader(font_shader_program_id, font_vert_id);
    glAttachShader(font_shader_program_id, font_frag_id);
    glLinkProgram(font_shader_program_id);

    GLuint lambert_shader_program_id = glCreateProgram();
    glAttachShader(font_shader_program_id, font_vert_id);
    glAttachShader(lambert_shader_program_id, lambert_frag_id);
    glLinkProgram(lambert_shader_program_id);

    // WORLD
    glm::vec3 pos = glm::vec3(10, 10, -10);
    global_cam.position = pos;
    global_cam.target = glm::vec3(0, 0, 0);
    updateCameraCoordinateFrame(global_cam);

    u32 max_cubes = 10;
    mesh_data_array.element_size = sizeof(Mesh);
    array_init(mesh_data_array, sizeof(Mesh), max_cubes);
    mesh_data_array.resize_func = array_defaul_resizer;

    xor_state.a = 10;

    double current_frame = glfwGetTime();
    double last_frame= current_frame;

    Mesh teapot_mesh;
    teapot_mesh.vertex_array_length = vertex_array.element_count;
    teapot_mesh.vertex_positions = (float*)vertex_array.base_ptr;
    teapot_mesh.vertex_normals = (float*)normals_array.base_ptr;
    teapot_mesh.vertex_colors = NULL;
    teapot_mesh.model_matrix  = glm::mat4(1.0);
    /*array_append(mesh_data_array, &teapot_mesh);*/
    GLMesh teapot_glmesh = create_gl_mesh_instance(teapot_mesh, 3);

    // Instance mesh
    Mesh cube_mesh = cube_create_mesh();
    GLMesh cube_glmesh = create_gl_mesh_instance(cube_mesh, 3);

    // World grid
    Mesh grid_mesh = grid_create_mesh();
    GLMesh grid_glmesh = create_gl_mesh_instance(grid_mesh, 3);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glEnable(GL_LINE_SMOOTH);
    glEnable(GL_STENCIL_TEST);

    while (!glfwWindowShouldClose(window)) {
        glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        current_frame = glfwGetTime();
        delta_time = current_frame - last_frame;
        float time_in_ms = delta_time * 1000.0f;
        // NOTE(kk): Lock framerate
        while (time_in_ms < 16.666f)
        {
            current_frame = glfwGetTime();
            delta_time = current_frame - last_frame;
            time_in_ms = delta_time * 1000.0f;
        }
        last_frame = current_frame;

        glfwGetWindowSize(window, &window_width, &window_height);
        glm::mat4 vp = get_view_matrix(window_width, window_height);

        u32 element_count = mesh_data_array.element_count;

        if (render_selction_buffer)
        {
            render_selection_buffer(window, vp, &cube_glmesh);
        }
        else
        {
            glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
            glStencilFunc(GL_ALWAYS, 1, 0xFF);
            glStencilMask(0xFF);

            for (int i=0; i < element_count; ++i)
            {
                Mesh* cube_mesh = (Mesh*)array_get_index(mesh_data_array, i);
                u32 offset = 0;
                float ratioX = offset / float(UINT_MAX);

                glm::vec3 position = cube_mesh->model_matrix[3];
                SphericalCoords spherical_coords = getSphericalCoords(position);
                spherical_coords.theta += (1 - ratioX) * 0.01f ;
                glm::vec3 new_position = getCartesianCoords(spherical_coords);
                glm::vec3 diff = new_position - position;
                diff[1] += current_frame / 200.0f;

                cube_mesh->model_matrix = glm::translate(cube_mesh->model_matrix, diff);

                cube_glmesh.mesh = cube_mesh;

                /*printf("%p is %p\n", cube_mesh, mouse_over_mesh);*/
                GLuint object_shader;
                if(cube_mesh == mouse_over_mesh)
                {
                    object_shader = hover_shader_program_id;
                }
                else if (cube_mesh == selected_mesh)
                {
                    object_shader = selection_shader_program_id;
                }
                else
                {
                    object_shader = default_shader_program_id;
                }
                drawGLMesh(cube_glmesh, GL_TRIANGLES, object_shader, vp);

                if (cube_mesh == selected_mesh)
                {
                    glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
                    glStencilMask(0x00);
                    glDisable(GL_DEPTH_TEST);

                    cube_glmesh.mesh = cube_mesh;
                    glm::mat4 mvp = vp * cube_glmesh.mesh->model_matrix;

                    glUseProgram(outline_shader_program_id);
                    GLuint matrix_id = glGetUniformLocation(outline_shader_program_id, "MVP");
                    glUniformMatrix4fv(matrix_id, 1, GL_FALSE, &mvp[0][0]);
                    glBindVertexArray(cube_glmesh.vao);
                    glDrawArrays(GL_TRIANGLES, 0, cube_glmesh.mesh->vertex_array_length / 3.0f);

                    glBindVertexArray(0);
                    glStencilMask(0xFF);
                    glStencilFunc(GL_ALWAYS, 0, 0xFF);
                    glEnable(GL_DEPTH_TEST);
                }

            }
            // World grid
            glUseProgram(default_shader_program_id);
            glBindVertexArray(grid_glmesh.vao);
            drawGLMesh(teapot_glmesh, GL_TRIANGLES, lambert_shader_program_id, vp);
            drawGLMesh(grid_glmesh, GL_LINES, default_shader_program_id, vp);
        }


        // Draw text
        // https://learnopengl.com/In-Practice/Text-Rendering
        glEnable(GL_BLEND);
        glEnable(GL_CULL_FACE);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glm::mat4 projection = glm::ortho(0.0f, (float)window_width, 0.0f, (float)window_height);

        unsigned int VAO, VBO;
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        // The 2D quad requires 6 vertices of 4 floats each, so we reserve 6 *
        // 4 floats of memory. Because we'll be updating the content of the
        // VBO's memory quite often we'll allocate the memory with
        // GL_DYNAMIC_DRAW
        glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);

        float redx = fmax(0, time_in_ms - 16.666f);

        glm::vec3 color = glm::vec3(0.3f + redx/10.f, 0.8f, 0.4f);
        glm::vec2 pos = glm::vec2(5, 10);
        float scale = .5f;

        glUseProgram(font_shader_program_id);
        glUniform3f(glGetUniformLocation(font_shader_program_id, "textColor"), color.x, color.y, color.z);
        glUniformMatrix4fv(
            glGetUniformLocation(font_shader_program_id, "projection"), 1, GL_FALSE, &projection[0][0]);
        glActiveTexture(GL_TEXTURE0);
        glBindVertexArray(VAO);

        char text[8];
        sprintf(text, "%.2fms", time_in_ms);
        int len = strlen(text);

        for (int i = 0; i < len; ++i){
            char c = text[i];
            Character ch = characters[c];
            float xpos = pos.x + ch.bearing.x * scale;
            float ypos = pos.y - (ch.size.y - ch.bearing.y) * scale;

            float w = ch.size.x * scale;
            float h = ch.size.y * scale;

            float vertices[6][4] = {
                { xpos,     ypos + h,   0.0f, 0.0f },
                { xpos,     ypos,       0.0f, 1.0f },
                { xpos + w, ypos,       1.0f, 1.0f },

                { xpos,     ypos + h,   0.0f, 0.0f },
                { xpos + w, ypos,       1.0f, 1.0f },
                { xpos + w, ypos + h,   1.0f, 0.0f }
            };
            // render glyph texture over quad
            glBindTexture(GL_TEXTURE_2D, ch.textureID);
            // update content of VBO memory
            glBindBuffer(GL_ARRAY_BUFFER, VBO);
            glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices); 
            glBindBuffer(GL_ARRAY_BUFFER, 0);
            // render quad
            glDrawArrays(GL_TRIANGLES, 0, 6);
            // now advance cursors for next glyph (note that advance is number of 1/64 pixels)
            pos.x += (ch.advance >> 6) * scale; // bitshift by 6 to get value in pixels (2^6 = 64)
        }
        glBindVertexArray(0);
        glBindTexture(GL_TEXTURE_2D, 0);
        glDisable(GL_BLEND);

        glfwSwapBuffers(window);

        // NOTE(kk): render selection back buffer before polling events
        render_selection_buffer(window, vp, &cube_glmesh);
        glfwPollEvents();
    }


    array_free(mesh_data_array);

    glfwTerminate();
    return 0;
}
