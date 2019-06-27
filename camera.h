#include <glm/glm.hpp>

typedef struct Camera
{
    float theta;
    float phi;
    float radius;

    glm::vec3 target;
} Camera;


glm::vec3 getCartesianPosition(Camera &cam)
{
    // NOTE(kk): Most of math resources on the subject refer to Y axis as Z
    float posX = cam.target.x + cam.radius * sin(cam.phi) * cos(cam.theta);
    float posY = cam.target.z + cam.radius * cos(cam.phi);
    float posZ = cam.target.y + cam.radius * sin(cam.phi) * sin(cam.theta);
    return glm::vec3(posX, posY, posZ);
}

typedef struct CameraCoordFrame
{
    glm::vec3 direction;
    glm::vec3 right;
    glm::vec3 up;
} CameraCoordFrame;


void getCameraCoordinateFrame(CameraCoordFrame &coords, glm::vec3 position, glm::vec3 target)
{
    glm::vec3 direction = glm::normalize(position - target);
    glm::vec3 right = glm::normalize(glm::cross(glm::vec3(0, 1, 0), direction));
    glm::vec3 up = glm::normalize(glm::cross(direction, right));

    coords.direction = direction;
    coords.right = right;
    coords.up = up;
}
