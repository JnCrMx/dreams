#pragma once

#include <glm/glm.hpp>

namespace entity::components
{
    struct collision
    {
        glm::vec3 min;
        glm::vec3 max;
    };
}
