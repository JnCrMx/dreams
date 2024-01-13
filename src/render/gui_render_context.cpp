#include "render/gui_render_context.hpp"

namespace render
{
    gui_render_context::gui_render_context(vk::CommandBuffer commandBuffer, int frame, font_renderer* fontRenderer)
        : m_fontRenderer(fontRenderer), m_commandBuffer(commandBuffer), m_frame(frame)
    {
    }

    void gui_render_context::draw_text(std::string_view text, float x, float y, float scale, glm::vec4 color)
    {
        m_fontRenderer->renderText(m_commandBuffer, m_frame, text, x, y, scale, color);
    }
}
