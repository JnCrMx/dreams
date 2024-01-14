#include <spdlog/spdlog.h>
#include <spdlog/cfg/env.h>

#include <entt/entt.hpp>

#include "render/gui_render_context.hpp"
#include "render/window.hpp"
#include "render/phases/render_test.hpp"

#include "entity/components/light.hpp"
#include "entity/components/position.hpp"
#include "entity/components/rotation.hpp"
#include "entity/components/velocity.hpp"
#include "entity/components/player.hpp"
#include "entity/components/renderable.hpp"
#include "entity/components/model.hpp"
#include "entity/components/camera.hpp"

#include "entity/word_ticker.hpp"

#include "utils.hpp"

int main(int argc, char *argv[])
{
    spdlog::cfg::load_env_levels();
#ifndef NDEBUG
    spdlog::set_level(spdlog::level::debug);
#endif
    spdlog::info("Welcome to your dream!");

    entt::registry registry;
    registry.on_construct<entity::components::model>().connect<&entt::registry::emplace_or_replace<entity::components::renderable>>();

    const auto light = registry.create();
    registry.emplace<entity::components::light>(light, glm::vec3(1.0, 1.0, 1.0), glm::vec3(1.0, 0.5, 0.75), 1e-9f, 50.0f);
    registry.emplace<entity::components::position>(light, 20.0, 35.0, 20.0);
    registry.emplace<entity::components::model>(light, "monkey.obj"_hs, "soraka.png"_hs);
    registry.emplace_or_replace<entity::components::renderable>(light, false, false);

    const auto soraka = registry.create();
    registry.emplace<entity::components::position>(soraka, 0.0, 0.0, 0.0);
    registry.emplace<entity::components::rotation>(soraka);
    registry.emplace<entity::components::velocity>(soraka);
    //registry.emplace<entity::components::collision>(soraka);
    registry.emplace<entity::components::player>(soraka);
    registry.emplace<entity::components::model>(soraka, "soraka.obj"_hs, "soraka.png"_hs);

    const auto ground = registry.create();
    registry.emplace<entity::components::position>(ground, 0.0, 0.0, 0.0);
    registry.emplace<entity::components::collision>(ground);
    registry.emplace<entity::components::model>(ground, "plane.obj"_hs, "ground.png"_hs);

    const auto cube = registry.create();
    registry.emplace<entity::components::position>(cube, -1.0, 0.25, -0.5);
    registry.emplace<entity::components::collision>(cube);
    registry.emplace<entity::components::model>(cube, "cube.obj"_hs, "gray.png"_hs);

    const auto camera = registry.create();
    registry.emplace<entity::components::target_camera>(camera, soraka, glm::vec3(0.0, 2.0, 0.0));
    registry.patch<entity::components::target_camera>(camera, [](entity::components::target_camera& cam){
        cam.yaw = glm::radians(180.0f);
        cam.pitch = glm::radians(30.0f);
    });

    render::window window;
    window.init();

    render::phases::render_test* renderer;
    entity::world_ticker* ticker;
    auto gui = [&renderer, &window](render::gui_render_context& ctx){
        ctx.draw_text("Hello world!", 0.0, 0.0, 0.05f, glm::vec4(1.0, 1.0, 1.0, 1.0));
        ctx.draw_text("Hello world!", 0.0, 2.0 - 0.05f, 0.05f, glm::vec4(1.0, 1.0, 1.0, 1.0));

        ctx.draw_text("FPS: "+utils::to_fixed_string<1>(window.currentFPS), 0.05f, 0.05f + 0*0.05f, 0.05f);
        if(const auto& allocator = renderer->get_allocator()) {
            auto budget = allocator.getHeapBudgets().front();
            auto usage = ((double)budget.usage) / ((double)budget.budget);
            ctx.draw_text("VRAM: "+utils::to_fixed_string<1>(usage*100.0)+"%", 0.05f, 0.05f + 1*0.05f, 0.05f);
        }
    };

    window.set_phase(renderer = new render::phases::render_test(&window, registry, gui), ticker = new entity::world_ticker(registry, soraka, camera));
    renderer->set_camera(camera);

    window.loop();

    return 0;
}
