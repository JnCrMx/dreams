#pragma once

#include <glm/glm.hpp>

namespace entity::components
{
    struct light
    {
        glm::vec3 direction;
        glm::vec3 color = glm::vec3(1.0, 1.0, 1.0);
        float zNear = 1.0f;
        float zFar = 10.0f;
        bool castShadow = true;
    };
}
