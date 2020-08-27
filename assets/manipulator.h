
#ifndef MANIPULATORH
#define MANIPULATORH

Mesh manipulator_create_mesh()
{

    Mesh manip_mesh = objloader_create_mesh("assets/arrow.obj");
    manip_mesh.model_matrix = glm::mat4(1);
    return manip_mesh;
}

#endif // MANIPULATORH
