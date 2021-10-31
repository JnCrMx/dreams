#include "entity/word_ticker.hpp"

#include "render/window.hpp"

#include "entity/components/camera.hpp"
#include "entity/components/position.hpp"
#include "entity/components/player.hpp"
#include "entity/components/velocity.hpp"
#include "entity/components/rotation.hpp"
#include "entity/components/collision.hpp"

namespace entity
{
	void world_ticker::init(render::window* win)
	{
		ticker::init(win);
		glfwSetInputMode(win->win, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	}

	void world_ticker::on_scroll(double x, double y)
	{
		registry.patch<components::target_camera>(camera, [y](components::target_camera& cam){
			if(cam.input)
			{
				cam.distance -= y;
				cam.distance = std::clamp(cam.distance, cam.minDistance, cam.maxDistance);
			}
		});
	}

	void world_ticker::on_cursor_pos(double x, double y)
	{
		double cx = 2.0*(x/width) - 1.0;
		double cy = 2.0*(y/height) - 1.0;

		double dx = cx - lastMouseX;
		double dy = cy - lastMouseY;

		registry.patch<components::target_camera>(camera, [this, dx, dy](components::target_camera& cam){
			cam.pitch = std::clamp(cam.pitch + dy, glm::radians(-90.0), glm::radians(90.0));
			cam.yaw += dx;
		});

		lastMouseX = cx;
		lastMouseY = cy;
	}

	void world_ticker::on_key(int key, int scancode, int action, int mods)
	{
		if(key == GLFW_KEY_SPACE)
		{
			registry.patch<components::position>(player, [](components::position& pos){
				pos.y += 0.1;
			});
		}
		if(key == GLFW_KEY_LEFT_SHIFT)
		{
			registry.patch<components::position>(player, [](components::position& pos){
				pos.y -= 0.1;
			});
		}

		if(key == GLFW_KEY_W)
		{
			registry.patch<components::player>(player, [action](components::player& p){
				if(action == GLFW_PRESS)
					p.motionForward = 1.0;
				if(action == GLFW_RELEASE)
					p.motionForward = 0.0;
			});
		}
		if(key == GLFW_KEY_S)
		{
			registry.patch<components::player>(player, [action](components::player& p){
				if(action == GLFW_PRESS)
					p.motionForward = -1.0;
				if(action == GLFW_RELEASE)
					p.motionForward = 0.0;
			});
		}
	}

	void world_ticker::tick(double dt)
	{
		{
			auto view = registry.view<const components::player, components::rotation, components::velocity>();
			for(auto [id, player, rotation, velocity] : view.each())
			{
				if(player.motionForward > 0.0)
				{
					rotation.yaw = glm::radians(180.0)-registry.get<components::target_camera>(camera).yaw;
					
					velocity.x = sin(rotation.yaw) * player.walkingSpeed;
					velocity.z = cos(rotation.yaw) * player.walkingSpeed;
				}
				else if(player.motionForward < 0.0)
				{
					rotation.yaw = -registry.get<components::target_camera>(camera).yaw;
					
					velocity.x = sin(rotation.yaw) * player.walkingSpeed;
					velocity.z = cos(rotation.yaw) * player.walkingSpeed;
				}
			}
		}

		{
			auto view = registry.view<components::velocity, components::position>();
			for(auto [id, velocity, position] : view.each())
			{
				double tx = position.x + velocity.x * dt;
				double ty = position.y + velocity.y * dt;
				double tz = position.z + velocity.z * dt;

				components::collision* collision = registry.try_get<components::collision>(id);
				if(collision)
				{

				}
				else
				{
					position.x = tx;
					position.y = ty;
					position.z = tz;
				}

				velocity.x *= 0.9;
				velocity.y *= 0.9;
				velocity.z *= 0.9;
			}
		}
	}
}