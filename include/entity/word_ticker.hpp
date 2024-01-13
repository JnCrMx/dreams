#pragma once

#include "ticker.hpp"
#include "entity/id.hpp"

namespace entity
{
    class world_ticker : public ticker
    {
        public:
            world_ticker(entt::registry& reg, entity_id player, entity_id camera) : ticker(reg), player(player), camera(camera) {}
            void init(render::window* win) override;
            void tick(double dt) override;
        protected:
            void on_key(int key, int scancode, int action, int mods) override;
            void on_cursor_pos(double x, double y) override;
            void on_scroll(double x, double y) override;
        private:
            entity_id player;
            entity_id camera;

            double lastMouseX = 0, lastMouseY = 0;
    };
}
