#include <cmath>

#include <stdio.h>
#include <stdlib.h>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "types.h"
#include "camera.h"

static Camera global_cam;
static double delta_time;


void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    /*if (key == GLFW_KEY_W)*/
    /*{*/
    /*global_cam.pos.z = global_cam.pos.z + 50 * delta_time;*/
    /*}*/
    /*else if (key == GLFW_KEY_A)*/
    /*{*/
    /*global_cam.pos.x = global_cam.pos.x + 50 * delta_time;*/
    /*}*/
    /*else if (key == GLFW_KEY_S)*/
    /*{*/
    /*global_cam.pos.z = global_cam.pos.z - 50 * delta_time;*/
    /*}*/
    /*else if (key == GLFW_KEY_D)*/
    /*{*/
    /*global_cam.pos.x = global_cam.pos.x - 50 * delta_time;*/
    /*}*/
}


static double lastPressX, lastPressY;

static bool rotate_mode = false;
static bool pan_mode = false;

static double theta = 0;
static double phi = 0;

static void cursorPositionCallback(GLFWwindow* window, double xpos, double ypos)
{
    if(rotate_mode) {
        theta += (xpos - lastPressX) * 0.01f;
        phi += (ypos - lastPressY) * 0.01f;
    }
    else if(pan_mode) {
        CameraCoordFrame coordFrame;
        getCameraCoordinateFrame(coordFrame, global_cam.pos, global_cam.target);

        float pan_x = (xpos - lastPressX) * 0.01f;
        float pan_y = (ypos - lastPressY) * 0.01f;

        glm::vec3 offset = coordFrame.right * pan_x + coordFrame.up * pan_y;
        global_cam.target = global_cam.target + offset;
        global_cam.pos = global_cam.pos + offset;

    }
    /*theta = fmax(fmin(theta, 3.14 * 2), 0.0f);*/
    /*phi = fmax(fmin(phi, 3.14), 0.0f);*/
    lastPressX = xpos;
    lastPressY = ypos;
}

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{

    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS && mods & GLFW_MOD_ALT)
    {
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);
        lastPressX = xpos;
        lastPressY = ypos;
        rotate_mode = true;
    }
    else if(button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS && mods & GLFW_MOD_CONTROL)
    {
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);
        lastPressX = xpos;
        lastPressY = ypos;
        pan_mode = true;
    }
    else if(button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
    {
        rotate_mode = false;
        pan_mode = false;
    }
}

void scrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    glm::vec3 direction = glm::normalize(global_cam.pos - global_cam.target);
    global_cam.pos = global_cam.pos + direction * float(yoffset) * 0.8f;
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
static const GLfloat cubeData[] = {
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

static const GLfloat colorData[] = {
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


int main()
{
    if (!glfwInit())
        return -1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    u32 window_width = 640;
    u32 window_height = 480;

    GLFWwindow* window = glfwCreateWindow(
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

    glEnable(GL_DEPTH_TEST); // enable depth-testing
    glDepthFunc(GL_LESS); // depth-testing interprets a smaller value as "closer"
    glEnable(GL_CULL_FACE); // depth-testing interprets a smaller value as "closer"

    GLuint vbo;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, 36 * 3 * sizeof(float), cubeData, GL_STATIC_DRAW);

    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);


    GLuint colorbuffer;
    glGenBuffers(1, &colorbuffer);
    glBindBuffer(GL_ARRAY_BUFFER, colorbuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(colorData), colorData, GL_STATIC_DRAW);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, NULL);


    GLuint vertex_shader_id = glCreateShader(GL_VERTEX_SHADER);
    u8 rv = compileShader(vertex_shader_id, "default.vert");
    assert(rv);

    GLuint fragment_shader_id= glCreateShader(GL_FRAGMENT_SHADER);
    rv = compileShader(fragment_shader_id, "default.frag");
    assert(rv);

    GLuint shader_program_id = glCreateProgram();
    glAttachShader(shader_program_id, vertex_shader_id);
    glAttachShader(shader_program_id, fragment_shader_id);
    glLinkProgram(shader_program_id);

    // WORLD
    global_cam.pos = glm::vec3(5, 5, 5);
    global_cam.target = glm::vec3(0, 0, 0);

    glm::mat4 Projection = glm::perspective(
            glm::radians(45.0f), (float)window_width / (float)window_height, 0.1f, 100.0f);

    GLuint MatrixID = glGetUniformLocation(shader_program_id, "MVP");

    double current_frame = glfwGetTime();
    double last_frame= current_frame;

    glm::mat4 Model = glm::mat4(1.0f);

    while (!glfwWindowShouldClose(window)) {
        current_frame = glfwGetTime();
        delta_time = current_frame - last_frame;
        last_frame = current_frame;

        float radius = sqrt(pow(global_cam.pos.x, 2) + pow(global_cam.pos.y, 2) + pow(global_cam.pos.z, 2));
        // NOTE(kk): Most of math resources on the subject refer to Y axis as Z
        float eyeX = global_cam.target.x + radius*sin(phi)*cos(theta);
        float eyeY = global_cam.target.z + radius*cos(phi); // <-
        float eyeZ = global_cam.target.y + radius*sin(phi)*sin(theta); // <-

        global_cam.pos = glm::vec3(eyeX, eyeY, eyeZ);

        CameraCoordFrame coordFrame;
        getCameraCoordinateFrame(coordFrame, global_cam.pos, global_cam.target);

        glm::mat4 View = glm::lookAt(
            global_cam.pos,
            global_cam.target,
            coordFrame.up
        );

        glm::mat4 mvp = Projection * View * Model;

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(shader_program_id);
        glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &mvp[0][0]);

        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, 12 * 3);

        glfwPollEvents();

        glfwSwapBuffers(window);
    }

    glfwTerminate();
    return 0;
}
