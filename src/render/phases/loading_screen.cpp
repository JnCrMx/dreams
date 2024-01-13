#include "render/phases/loading_screen.hpp"

#include "render/window.hpp"
#include "render/debug.hpp"
#include "render/utils.hpp"
#include "utils.hpp"

#include <chrono>
#include <spdlog/spdlog.h>

#include <freetype2/ft2build.h>
#include <freetype/freetype.h>

namespace render::phases
{
	loading_screen::loading_screen(window* window) : phase(window)
	{
	}

	void loading_screen::preload()
	{
		FT_Library ft;
		FT_Error err = FT_Init_FreeType(&ft);
		font = std::make_unique<font_renderer>("/usr/share/fonts/liberation/LiberationSans-Regular.ttf", 128, device, allocator);

		pool = device.createCommandPoolUnique(vk::CommandPoolCreateInfo(vk::CommandPoolCreateFlagBits::eResetCommandBuffer, graphicsFamily));

		background = std::make_unique<texture>(device, allocator, 1920, 1080, vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferSrc);
		loader->loadTexture(background.get(), "loading_screen/background.png");

		vk::AttachmentDescription attachment({}, win->swapchainFormat.format, vk::SampleCountFlagBits::e1,
			vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore,
			vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare,
			vk::ImageLayout::eUndefined, vk::ImageLayout::ePresentSrcKHR);
		vk::AttachmentReference ref(0, vk::ImageLayout::eColorAttachmentOptimal);
		vk::SubpassDependency dep(VK_SUBPASS_EXTERNAL, 0,
			vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::PipelineStageFlagBits::eColorAttachmentOutput,
			{}, vk::AccessFlagBits::eColorAttachmentWrite);
		vk::SubpassDescription subpass({}, vk::PipelineBindPoint::eGraphics, {}, ref);
		vk::RenderPassCreateInfo renderpass_info({}, attachment, subpass, dep);
		renderPass = device.createRenderPassUnique(renderpass_info);
		debugName(device, renderPass.get(), "Loading Screen Render Pass");

		{
			vk::PipelineLayoutCreateInfo layout_info({}, {}, {});
			pipelineLayout = device.createPipelineLayoutUnique(layout_info);
			debugName(device, pipelineLayout.get(), "Loading Screen Pipeline Layout");
		}
		{
			vk::UniqueShaderModule vertexShader = createShader(device, "loading.vert");
			vk::UniqueShaderModule fragmentShader = createShader(device, "loading.frag");
			std::array<vk::PipelineShaderStageCreateInfo, 2> shaders = {
				vk::PipelineShaderStageCreateInfo({}, vk::ShaderStageFlagBits::eVertex, vertexShader.get(), "main"),
				vk::PipelineShaderStageCreateInfo({}, vk::ShaderStageFlagBits::eFragment, fragmentShader.get(), "main")
			};

			vk::PipelineVertexInputStateCreateInfo vertex_input({}, {}, {});
			vk::PipelineInputAssemblyStateCreateInfo input_assembly({}, vk::PrimitiveTopology::eTriangleList);
			vk::PipelineTessellationStateCreateInfo tesselation({}, {});

			vk::Viewport v{};
			vk::Rect2D s{};
			vk::PipelineViewportStateCreateInfo viewport({}, v, s);

			vk::PipelineRasterizationStateCreateInfo rasterization({}, false, false, vk::PolygonMode::eFill, vk::CullModeFlagBits::eNone, vk::FrontFace::eCounterClockwise, false, 0.0f, 0.0f, 0.0f, 1.0f);
			vk::PipelineMultisampleStateCreateInfo multisample({}, vk::SampleCountFlagBits::e1);
			vk::PipelineDepthStencilStateCreateInfo depthStencil({}, false, false);

			vk::PipelineColorBlendAttachmentState attachment(false);
			attachment.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
			vk::PipelineColorBlendStateCreateInfo colorBlend({}, false, vk::LogicOp::eClear, attachment);

			std::array<vk::DynamicState, 2> dynamicStates{vk::DynamicState::eViewport, vk::DynamicState::eScissor};
			vk::PipelineDynamicStateCreateInfo dynamic({}, dynamicStates);

			vk::GraphicsPipelineCreateInfo pipeline_info({}, shaders, &vertex_input,
				&input_assembly, &tesselation, &viewport, &rasterization, &multisample, &depthStencil, &colorBlend, &dynamic, pipelineLayout.get(), renderPass.get());
			pipeline = device.createGraphicsPipelineUnique({}, pipeline_info).value;
			debugName(device, pipeline.get(), "Loading Screen Pipeline");
		}

		font->preload(ft, loader, renderPass.get());
	}

	void loading_screen::prepare(std::vector<vk::Image> swapchainImages, std::vector<vk::ImageView> swapchainViews)
	{
		commandBuffers = device.allocateCommandBuffersUnique(
			vk::CommandBufferAllocateInfo(pool.get(), vk::CommandBufferLevel::ePrimary, swapchainImages.size()));
		this->swapchainImages = swapchainImages;

		for(int i=0; i<swapchainViews.size(); i++)
		{
			vk::FramebufferCreateInfo framebuffer_info({}, renderPass.get(), swapchainViews[i],
				win->swapchainExtent.width, win->swapchainExtent.height, 1);
			framebuffers.push_back(device.createFramebufferUnique(framebuffer_info));
			debugName(device, framebuffers.back().get(), "Loading Screen Framebuffer #"+std::to_string(i));

			debugName(device, commandBuffers[i].get(), "Loading Screen Command Buffer #"+std::to_string(i));
		}
		font->prepare(swapchainViews.size());
	}

	void loading_screen::submitLoadingPoint(LoadingPoint p)
	{
		loadingPoints.push_back(p);
	}

	void loading_screen::render(int frame, vk::Semaphore imageAvailable, vk::Semaphore renderFinished, vk::Fence fence)
	{
		for(auto& p : loadingPoints)
		{
			if(!p.done && utils::is_ready(p.future))
			{
				p.done = true;
			}
		}

		auto donePoints = std::count_if(loadingPoints.begin(), loadingPoints.end(), [](auto p){
			return p.done;
		});
		auto totalPoints = loadingPoints.size();

		auto time = std::chrono::high_resolution_clock::now().time_since_epoch();
		auto seconds = std::chrono::duration_cast<std::chrono::seconds>(time);
		auto partialSeconds = std::chrono::duration<float>(time-seconds);

		vk::UniqueCommandBuffer& commandBuffer = commandBuffers[frame];

		commandBuffer->begin(vk::CommandBufferBeginInfo());

		vk::ClearValue color(std::array<float, 4>{partialSeconds.count(), 0.5f, 0.0f, 1.0f});
		commandBuffer->beginRenderPass(vk::RenderPassBeginInfo(renderPass.get(), framebuffers[frame].get(),
			vk::Rect2D({0, 0}, win->swapchainExtent), color), vk::SubpassContents::eInline);
		commandBuffer->bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline.get());

		vk::Viewport viewport(0.0f, 0.0f, win->swapchainExtent.width, win->swapchainExtent.height, 0.0f, 1.0f);
		vk::Rect2D scissor({0,0}, win->swapchainExtent);
		commandBuffer->setViewport(0, viewport);
		commandBuffer->setScissor(0, scissor);

		vk::DebugUtilsLabelEXT label("Test!", std::array<float, 4>{1.0f, 0.5f, 0.0f, 1.0f});
		commandBuffer->insertDebugUtilsLabelEXT(label);
		commandBuffer->draw(3, 1, 0, 0);

		font->renderText(commandBuffer.get(), frame, "FPS: "+std::to_string(win->currentFPS), 0, 0, 0.05f, glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
		font->renderText(commandBuffer.get(), frame, "MOIN MOIN", 1, 0, 0.1f, glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
		font->renderText(commandBuffer.get(), frame, std::to_string(donePoints)+"/"+std::to_string(totalPoints), 0, 0.1, 0.1f, glm::vec4(0.0f, 0.0f, 1.0f, 1.0f));
		font->finish(frame);

		commandBuffer->endRenderPass();
		commandBuffer->end();

		vk::PipelineStageFlags waitFlags = vk::PipelineStageFlagBits::eColorAttachmentOutput;
		vk::SubmitInfo submit_info(imageAvailable, waitFlags, commandBuffer.get(), renderFinished);
		graphicsQueue.submit(submit_info, fence);
	}
}
