#include <glm/glm.hpp>

typedef struct Camera
{
    glm::vec3 pos;
    glm::vec3 target;
} Camera;


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
