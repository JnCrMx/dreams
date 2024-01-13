#pragma once

#include "render/font_renderer.hpp"

namespace render
{
    class gui_render_context
    {
        public:
            gui_render_context(vk::CommandBuffer commandBuffer, int frame, font_renderer* fontRenderer);

            void draw_text(std::string_view text, float x, float y, float scale = 1.0f, glm::vec4 color = glm::vec4(1.0, 1.0, 1.0, 1.0));
        private:
            font_renderer* m_fontRenderer;
            vk::CommandBuffer m_commandBuffer;
            int m_frame;
    };
}
