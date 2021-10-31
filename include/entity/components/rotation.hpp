#pragma once

#include <glm/glm.hpp>
#define GLM_FORCE_RADIANS
#include <glm/gtc/matrix_transform.hpp>

namespace entity::components
{
	struct rotation
	{
		double yaw;
		double pitch;

		glm::mat4 matrix()
		{
			return glm::rotate(glm::mat4(1.0), (float)yaw, glm::vec3(0.0, 1.0, 0.0));
		}
	};
}
