#pragma once

#include "render/gui_render_context.hpp"
#include "render/phase.hpp"
#include "render/texture.hpp"
#include "render/model.hpp"
#include "render/font_renderer.hpp"

#include "entity/components/light.hpp"
#include "entity/components/position.hpp"
#include "entity/components/model.hpp"
#include "entity/components/camera.hpp"
#include "entity/components/collision.hpp"

#include <entt/entt.hpp>
#include <memory>

using namespace entt::literals;

namespace render::phases
{
    class render_test : public phase
    {
        static constexpr std::array<entt::hashed_string, 4> load_models = {
            "plane.obj"_hs, "soraka.obj"_hs, "monkey.obj"_hs, "cube.obj"_hs
        };
        static constexpr std::array<entt::hashed_string, 3> load_textures = {
            "soraka.png"_hs, "ground.png"_hs, "gray.png"_hs
        };

        public:
            render_test(window* window, entt::registry& entities, std::function<void(gui_render_context&)> guiRenderCallback = {});
            ~render_test();

            void preload() override;
            void prepare(std::vector<vk::Image> swapchainImages, std::vector<vk::ImageView> swapchainViews) override;
            void init() override;
            void render(int frame, vk::Semaphore imageAvailable, vk::Semaphore renderFinished, vk::Fence fence) override;

            void set_camera(entity::entity_id entity);
        private:
            std::vector<std::vector<std::unique_ptr<texture>>> shadowBuffers;
            vk::UniqueSampler shadowSampler;

            std::vector<std::unique_ptr<texture>> colorBuffers;
            std::vector<std::unique_ptr<texture>> shadeBuffers;
            std::vector<std::unique_ptr<texture>> depthBuffers;

            std::vector<std::unique_ptr<texture>> colorRevolveBuffers;
            std::vector<std::unique_ptr<texture>> shadeRevolveBuffers;

            vk::UniqueRenderPass shadowRenderPass;
            vk::UniqueRenderPass mainRenderPass;
            vk::UniqueRenderPass shadeRenderPass;
            vk::UniqueRenderPass overlayPass;

            vk::UniqueDescriptorPool descriptorPool;

            vk::UniqueDescriptorSetLayout mainDescriptorLayout;
            std::vector<vk::DescriptorSet> mainDescriptorSets;
            std::vector<vk::DescriptorSet> shadowDescriptorSets;

            vk::UniqueDescriptorSetLayout shadeDescriptorLayout;
            std::vector<vk::DescriptorSet> shadeDescriptorSets;

            vk::UniqueDescriptorSetLayout shadowMapDescriptorLayout;
            std::vector<std::vector<vk::DescriptorSet>> shadowMapDescriptorSets;

            vk::UniqueDescriptorPool textureDescriptorPool;
            vk::UniqueDescriptorSetLayout textureDescriptorLayout;
            std::vector<vk::DescriptorSet> textureDescriptorSets;

            vk::UniqueSampler textureSampler;

            vk::UniquePipelineLayout shadowPipelineLayout;
            vk::UniquePipelineLayout mainPipelineLayout;
            vk::UniquePipelineLayout shadePipelineLayout;
            vk::UniquePipeline shadowPipeline;
            vk::UniquePipeline mainPipeline;
            vk::UniquePipeline hitboxPipeline;
            vk::UniquePipeline shadePipeline;

            std::vector<vk::Image> swapchainImages;
            std::vector<std::vector<vk::UniqueFramebuffer>> shadowFramebuffers;
            std::vector<vk::UniqueFramebuffer> mainFramebuffers;
            std::vector<vk::UniqueFramebuffer> shadeFramebuffers;
            std::vector<vk::UniqueFramebuffer> overlayFramebuffers;

            vk::UniqueCommandPool pool;
            std::vector<vk::UniqueCommandBuffer> commandBuffers;

            std::unique_ptr<font_renderer> font;

            std::vector<std::unique_ptr<texture>> textures;
            std::map<entt::hashed_string::hash_type, vk::DescriptorSet> textureSets;
            std::map<entt::hashed_string::hash_type, std::unique_ptr<model>> models;

            struct GlobalInfo
            {
                glm::mat4 projection;
                glm::mat4 view;
            };
            std::vector<vk::Buffer> globalUniformBuffers;
            std::vector<vma::Allocation> globalUniformAllocations;
            std::vector<GlobalInfo*> globalUniformPointers;

            std::vector<vk::Buffer> globalShadowUniformBuffers;
            std::vector<vma::Allocation> globalShadowUniformAllocations;
            std::vector<GlobalInfo*> globalShadowUniformPointers;

            struct ModelInfo
            {
                glm::mat4 transform;
                glm::vec4 min;
                glm::vec4 max;
            };
            std::vector<vk::Buffer> modelUniformBuffers;
            std::vector<vma::Allocation> modelUniformAllocations;
            std::vector<ModelInfo*> modelUniformPointers;

            struct LightInfo
            {
                glm::vec4 position;
                glm::vec4 direction;
                glm::vec4 color;

                glm::mat4 lightMatrix;
                glm::mat4 globalInverse;
            };
            std::vector<vk::Buffer> lightUniformBuffers;
            std::vector<vma::Allocation> lightUniformAllocations;
            std::vector<LightInfo*> lightUniformPointers;

            entt::registry& entities;
            entity::entity_id camera;
            decltype(std::declval<entt::registry>().view<
                const entity::components::position,
                const entity::components::light>()) lightView;
            decltype(std::declval<entt::registry>().view<
                const entity::components::position,
                const entity::components::model>()) modelView;
            decltype(std::declval<entt::registry>().view<
                const entity::components::position,
                const entity::components::model,
                entity::components::collision>()) hitboxView;
            std::function<void(gui_render_context&)> guiRenderCallback;

            static constexpr int maxLights = 8;
            static constexpr int maxObjects = 2048;
    };
}
