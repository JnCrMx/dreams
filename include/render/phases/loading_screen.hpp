#pragma once

#include "render/phase.hpp"
#include "render/font_renderer.hpp"

namespace render::phases
{
    class loading_screen : public phase
    {
        public:
            loading_screen(window* window);
            void preload() override;
            void prepare(std::vector<vk::Image> swapchainImages, std::vector<vk::ImageView> swapchainViews) override;
            void render(int frame, vk::Semaphore imageAvailable, vk::Semaphore renderFinished, vk::Fence fence) override;

            struct LoadingPoint
            {
                std::string name;
                std::shared_future<void> future;
                bool done = false;
            };
            void submitLoadingPoint(LoadingPoint p);
        private:
            std::unique_ptr<texture> background;
            std::unique_ptr<font_renderer> font;

            vk::UniqueRenderPass renderPass;
            vk::UniquePipelineLayout pipelineLayout;
            vk::UniquePipeline pipeline;

            std::vector<vk::Image> swapchainImages;
            std::vector<vk::UniqueFramebuffer> framebuffers;

            vk::UniqueCommandPool pool;
            std::vector<vk::UniqueCommandBuffer> commandBuffers;

            std::vector<LoadingPoint> loadingPoints;
    };
}
