#include <glm/glm.hpp>

typedef struct Camera
{
    glm::vec3 position;
    glm::vec3 target;

    glm::vec3 direction;
    glm::vec3 right;
    glm::vec3 up;

    float fov;
    float focus_dist;
    float aspect_ratio;
    float aperature;

    glm::vec3 horizontalVector;
    glm::vec3 verticalVector;
    glm::vec3 topLeftCorner;
} Camera;


typedef struct SphericalCoords
{
    float theta;
    float phi;
    float radius;
} SphericalCoords;


glm::vec3 random_in_unit_disc() {
    glm::vec3 p;
    do {
        p = glm::vec3(2.0) * glm::vec3(rand(), rand(), 0) - glm::vec3(1, 1, 0);
    } while (glm::dot(p, p) >= 1.0);
    return p;
}




glm::vec3 getCameraDirection(Camera &cam)
{
    glm::vec3 direction = glm::normalize(cam.position - cam.target);
    return direction;
}


glm::vec3 getCameraRightVector(Camera &cam)
{
    glm::vec3 right = glm::normalize(glm::cross(glm::vec3(0, 1, 0), getCameraDirection(cam)));
    return right;
}


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


void camera_update(Camera &cam)
{
    // Create coordinate frame
    glm::vec3 direction = glm::normalize(cam.position - cam.target);
    glm::vec3 right = glm::normalize(glm::cross(glm::vec3(0,1,0), direction));
    glm::vec3 up = glm::cross(direction, right);
    cam.direction = direction;
    cam.right = right;
    cam.up = up;

    float theta = glm::radians(cam.fov);
    float half_height = tan(theta / 2);
    float half_width = cam.aspect_ratio * half_height;

    cam.focus_dist = 1000.0f;

    cam.horizontalVector = 2 * half_width * right * cam.focus_dist;
    cam.verticalVector = 2 * half_height * up* cam.focus_dist;
    cam.topLeftCorner = (cam.position - direction - half_width * right * cam.focus_dist -
                     half_height * up * cam.focus_dist - cam.focus_dist * direction);
}

Ray camera_shoot_ray(Camera &cam, float s, float t)
{
    // aperature
    glm::vec3 rd = glm::vec3(0);
    //if (cam.aperature > 0) {
        //rd = vec3(this->aperature) * random_in_unit_disc();
    //}
    glm::vec3 offset = cam.right * rd.x + cam.up * rd.y;

    glm::vec3 origin = cam.position + offset;
    glm::vec3 direction = (cam.topLeftCorner + s * cam.horizontalVector +
                       t * cam.verticalVector - cam.position - offset);
    Ray r;
    r.origin = origin;
    r.direction = direction;
    return r;
}
