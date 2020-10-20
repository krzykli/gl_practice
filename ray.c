#ifndef RAYH
#define RAYH

float EPSILON = 0.0000001f;

struct Material
{
    u32 MaterialID;
};

struct Ray
{
    glm::vec3 origin;
    glm::vec3 direction;
};


struct HitRecord
{
    float t;
    glm::vec3 p;
    glm::vec3 normal;
    /*Material *matrial;*/
};

struct Triangle
{
    glm::vec3 A;
    glm::vec3 B;
    glm::vec3 C;
};


glm::vec3 ray_point_at_distance(Ray& ray, float t) {
    return ray.origin + t * ray.direction;
}


bool ray_intersect_triangle(Ray &ray, Triangle& tri, float t_min, float t_max, HitRecord &rec)
{
    float u, v, distance;

    glm::vec3 AB = (tri.B - tri.A);
    glm::vec3 AC = (tri.C - tri.A);
    glm::vec3 normal = glm::normalize(glm::cross(AB, AC));

    glm::vec3 rayCrossEdge = glm::cross(ray.direction, AC);

    float det = glm::dot(AB, rayCrossEdge);

    if (det > -EPSILON && det < EPSILON)
        return 0;

    float inv_det = 1.0f / det;

    glm::vec3 rayToVert = ray.origin - tri.A;
    u = glm::dot(rayToVert, rayCrossEdge) * inv_det;
    if (u < 0.0f || u > 1.0f)
        return 0;

    glm::vec3 qvec = glm::cross(rayToVert, AB);
    v = glm::dot(ray.direction, qvec) * inv_det;
    if (v < 0.0f || u + v > 1.0f)
        return 0;

    distance = glm::dot(AC, qvec) * inv_det;
    if (distance > t_min && distance < t_max) {
        rec.p = ray_point_at_distance(ray, distance);
        rec.normal = normal;
        rec.t = distance;
        return true;
    }

    return false;
}

void swapf(float &a, float &b)
{
     float temp = a;
     a = b;
     b = temp;
}


// https://www.scratchapixel.com/code.php?id=10&origin=/lessons/3d-basic-rendering/minimal-ray-tracer-rendering-simple-shapes
bool ray_intersect_box(const Ray &r, glm::vec3 vmin, glm::vec3 vmax)
{
    float tmin = (vmin.x - r.origin.x) / r.direction.x;
    float tmax = (vmax.x - r.origin.x) / r.direction.x;

    if (tmin > tmax) swapf(tmin, tmax);

    float tymin = (vmin.y - r.origin.y) / r.direction.y;
    float tymax = (vmax.y - r.origin.y) / r.direction.y;

    if (tymin > tymax) swapf(tymin, tymax);

    if ((tmin > tymax) || (tymin > tmax))
        return false;

    if (tymin > tmin)
        tmin = tymin;

    if (tymax < tmax)
        tmax = tymax;

    float tzmin = (vmin.z - r.origin.z) / r.direction.z;
    float tzmax = (vmax.z - r.origin.z) / r.direction.z;

    if (tzmin > tzmax) swapf(tzmin, tzmax);

    if ((tmin > tzmax) || (tzmin > tmax))
        return false;

    if (tzmin > tmin)
        tmin = tzmin;

    if (tzmax < tmax)
        tmax = tzmax;

    return true;
}


glm::vec2 pixel_to_NDC(float x, float y, float window_width, float window_height)
{
     float x_ndc = (x + 0.5) / window_width;
     float y_ndc = (y + 0.5) / window_height;
     return glm::vec2(x_ndc, y_ndc);
}


glm::vec2 pixel_to_screen(float x_ndc, float y_ndc)
{
     float x_screen = (2 * x_ndc - 1);
     float y_screen = (1 - 2 * y_ndc);
     return glm::vec2(x_screen, y_screen);
}


#endif // RAYH
