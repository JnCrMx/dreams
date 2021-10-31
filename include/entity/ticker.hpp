#pragma once

#include <entt/entt.hpp>

namespace render
{
	class window;
}

namespace entity
{
	class ticker
	{
		public:
			ticker(entt::registry& reg) : registry(reg) {}
			virtual ~ticker();

			virtual void init(render::window* win);
			virtual void tick(double dt);
		protected:
			entt::registry& registry;
			int width, height;

			virtual void on_key(int key, int scancode, int action, int mods) {}
			virtual void on_scroll(double x, double y) {}
			virtual void on_cursor_pos(double x, double y) {}
	};
}
