#pragma once

#include "entity/components/collision.hpp"

#include <vector>

namespace entity::helpers
{
    bool collide(const components::collision a, const glm::mat4 at, const components::collision b, const glm::mat4 bt);

    std::vector<glm::vec3> convexHull(const std::vector<glm::vec3>& vertices);
}
