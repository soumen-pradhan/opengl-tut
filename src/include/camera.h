#pragma once

#include <glm/glm.hpp>

struct Camera {
    glm::vec3 pos, front, right, up;
    float speed, fov;
    float lastX, lastY;
    float mouseSensitivity;
    float yaw, pitch;
};
