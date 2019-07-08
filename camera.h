#include <glm/glm.hpp>

typedef struct Camera
{
    glm::vec3 position;
    glm::vec3 target;

} Camera;

typedef struct SphericalCoords
{
    float theta;
    float phi;
    float radius;
} SphericalCoords;

typedef struct CoordFrame
{
    glm::vec3 direction;
    glm::vec3 right;
    glm::vec3 up;
} CoordFrame;


glm::vec3 getCartesianCoords(SphericalCoords &sphericalCoords)
{
    // NOTE(kk): Most of math resources on the subject refer to Y axis as Z
    float posX = sphericalCoords.radius * sin(sphericalCoords.phi) * sin(sphericalCoords.theta);
    float posY = sphericalCoords.radius * cos(sphericalCoords.phi);
    float posZ = -sphericalCoords.radius * sin(sphericalCoords.phi) * cos(sphericalCoords.theta);
    return glm::vec3(posX, posY, posZ);
}

SphericalCoords getSphericalCoords(glm::vec3 position)
{
    SphericalCoords coords;
    coords.radius = sqrt(pow(position.x, 2) + pow(position.y, 2) + pow(position.z, 2));
    coords.phi = acos(position.y / coords.radius);
    coords.theta = atan2(position.x, -position.z);
    return coords;
}


void getCameraCoordinateFrame(CoordFrame &coords, glm::vec3 position, glm::vec3 target)
{
    glm::vec3 direction = glm::normalize(position - target);
    glm::vec3 right = glm::normalize(glm::cross(glm::vec3(0, 1, 0), direction));
    glm::vec3 up = glm::cross(direction, right);

    coords.direction = direction;
    coords.right = right;
    coords.up = up;
}
