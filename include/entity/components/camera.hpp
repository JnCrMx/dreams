#pragma once

#include "entity/id.hpp"

#include <glm/glm.hpp>

namespace entity::components
{
	struct target_camera
	{
		// fixed
		entity_id target;
		glm::vec3 offset = glm::vec3(0.0, 2.0, 0.0);

		// fixed
		bool input = true;
		float minDistance = 2.0f;
		float maxDistance = 20.0f;

		// fixed
		float fov = glm::radians(45.0f);
		float zNear = 0.1f;
		float zFar = 100.0f;

		// user-input
		float distance = 10.0f;
		float yaw = 0.0f;
		float pitch = 0.0f;
	};
}
