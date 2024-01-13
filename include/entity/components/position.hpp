#pragma once

#include <glm/glm.hpp>

namespace entity::components
{
    struct position
    {
        double x;
        double y;
        double z;

        operator glm::vec3() const
        {
            return glm::vec3(x, y, z);
        }
    };
}
