#include "entity/helpers/collision.hpp"

#include <algorithm>
#include <array>
#include <stdexcept>

namespace entity::helpers
{
	bool collide(const components::collision a, const glm::mat4 at, const components::collision b, const glm::mat4 bt)
	{
		return false;
	}

	float distance(const glm::vec3 a, const glm::vec3 b, const glm::vec3 p)
	{
		auto q = a;
		auto u = b-a;
		return glm::length(glm::cross(q-p, u)) / glm::length(u);
	}

	std::array<glm::vec3, 3> createSimplex(const std::vector<glm::vec3>& vertices)
	{
		glm::vec3 min, max;
		for(int i=0; i<3; i++)
		{
			auto [mm, mx] = std::minmax_element(vertices.begin(), vertices.end(), [i](glm::vec3 a, glm::vec3 b){return a[i] < b[i];});
			min = *mm;
			max = *mx;

			if(min != max)
				break;
			if(i==2) throw std::runtime_error("degenerate point cloud");
		}

		glm::vec3 dist = *std::max_element(vertices.begin(), vertices.end(), [min, max](auto a, auto b){
			return distance(min, max, a) < distance(min, max, b);
		});
	}

	std::vector<glm::vec3> convexHull(const std::vector<glm::vec3>& vertices)
	{

	}
}
