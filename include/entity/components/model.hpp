#pragma once

#include <glm/glm.hpp>
#include <entt/core/hashed_string.hpp>

namespace entity::components
{
	struct model
	{
		entt::hashed_string::hash_type model_name;
		entt::hashed_string::hash_type texture_name;
	};
}
