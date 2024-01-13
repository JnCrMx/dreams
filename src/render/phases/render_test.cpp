#include "render/phases/render_test.hpp"
#include "render/window.hpp"
#include "render/utils.hpp"
#include "render/debug.hpp"
#include "config.hpp"
#include "utils.hpp"
#include "entity/components/renderable.hpp"
#include "entity/components/rotation.hpp"

#define GLM_FORCE_RADIANS
#include <glm/gtc/matrix_transform.hpp>

#include <spdlog/spdlog.h>

using namespace config;

namespace render::phases
{
    render_test::render_test(window* window, entt::registry& entities) : phase(window), entities(entities)
    {
        lightView = entities.view<const entity::components::position, const entity::components::light>();
        modelView = entities.view<const entity::components::position, const entity::components::model>();
        hitboxView = entities.view<const entity::components::position, const entity::components::model, entity::components::collision>();
    }

    render_test::~render_test()
    {
        for(int i=0; i<lightUniformPointers.size(); i++)
        {
            allocator.unmapMemory(lightUniformAllocations[i]);
            allocator.destroyBuffer(lightUniformBuffers[i], lightUniformAllocations[i]);
        }
        for(int i=0; i<globalUniformPointers.size(); i++)
        {
            allocator.unmapMemory(globalUniformAllocations[i]);
            allocator.destroyBuffer(globalUniformBuffers[i], globalUniformAllocations[i]);
        }
        for(int i=0; i<globalShadowUniformPointers.size(); i++)
        {
            allocator.unmapMemory(globalShadowUniformAllocations[i]);
            allocator.destroyBuffer(globalShadowUniformBuffers[i], globalShadowUniformAllocations[i]);
        }
        for(int i=0; i<modelUniformPointers.size(); i++)
        {
            allocator.unmapMemory(modelUniformAllocations[i]);
            allocator.destroyBuffer(modelUniformBuffers[i], modelUniformAllocations[i]);
        }
    }

    void render_test::preload()
    {
        pool = device.createCommandPoolUnique(vk::CommandPoolCreateInfo(vk::CommandPoolCreateFlagBits::eResetCommandBuffer, graphicsFamily));

        {
            vk::AttachmentDescription shadowAttachment({}, vk::Format::eD32Sfloat, vk::SampleCountFlagBits::e1,
                vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore,
                vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare,
                vk::ImageLayout::eUndefined, vk::ImageLayout::eShaderReadOnlyOptimal);
            vk::AttachmentReference ref(0, vk::ImageLayout::eDepthStencilAttachmentOptimal);
            vk::SubpassDescription subpass({}, vk::PipelineBindPoint::eGraphics, 0, {}, 0, {}, 0, &ref, 0, {});
            std::array<vk::SubpassDependency, 1> dependencies = {
                vk::SubpassDependency(0, VK_SUBPASS_EXTERNAL, vk::PipelineStageFlagBits::eLateFragmentTests, vk::PipelineStageFlagBits::eFragmentShader,
                    vk::AccessFlagBits::eDepthStencilAttachmentWrite, vk::AccessFlagBits::eShaderRead)
            };

            vk::RenderPassCreateInfo renderpass_info({}, shadowAttachment, subpass, dependencies);

            shadowRenderPass = device.createRenderPassUnique(renderpass_info);
            debugName(device, shadowRenderPass.get(), "Render Test Shadow Render Pass");
        }
        {
            std::array<vk::AttachmentDescription, 5> attachments = {
        /*0*/    vk::AttachmentDescription({}, vk::Format::eR8G8B8A8Srgb, CONFIG.sampleCount, // color
                    vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eDontCare,
                    vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare,
                    vk::ImageLayout::eUndefined, vk::ImageLayout::eGeneral),
        /*1*/    vk::AttachmentDescription({}, vk::Format::eR32G32B32A32Sfloat, CONFIG.sampleCount, // shade
                    vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eDontCare,
                    vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare,
                    vk::ImageLayout::eUndefined, vk::ImageLayout::eGeneral),
        /*2*/    vk::AttachmentDescription({}, vk::Format::eD32Sfloat, CONFIG.sampleCount, // depth
                    vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eDontCare,
                    vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare,
                    vk::ImageLayout::eUndefined, vk::ImageLayout::eGeneral),

        /*3*/    vk::AttachmentDescription({}, vk::Format::eR8G8B8A8Srgb, vk::SampleCountFlagBits::e1, // color revolve
                    vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eStore,
                    vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare,
                    vk::ImageLayout::eUndefined, vk::ImageLayout::eShaderReadOnlyOptimal),
        /*4*/    vk::AttachmentDescription({}, vk::Format::eR32G32B32A32Sfloat, vk::SampleCountFlagBits::e1, // shade revolve
                    vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eStore,
                    vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare,
                    vk::ImageLayout::eUndefined, vk::ImageLayout::eShaderReadOnlyOptimal),
            };

            vk::AttachmentReference colorWriteRef(0, vk::ImageLayout::eColorAttachmentOptimal);
            vk::AttachmentReference shadingWriteRef(1, vk::ImageLayout::eColorAttachmentOptimal);
            vk::AttachmentReference depthRef(2, vk::ImageLayout::eDepthStencilAttachmentOptimal);

            vk::AttachmentReference colorReadRef(3, vk::ImageLayout::eShaderReadOnlyOptimal);
            vk::AttachmentReference shadingReadRef(4, vk::ImageLayout::eShaderReadOnlyOptimal);

            vk::AttachmentReference colorRevolveRef(3, vk::ImageLayout::eColorAttachmentOptimal);
            vk::AttachmentReference shadeRevolveRef(4, vk::ImageLayout::eColorAttachmentOptimal);

            std::array<vk::AttachmentReference, 2> pass1Refs = {colorWriteRef, shadingWriteRef};
            std::array<vk::AttachmentReference, 2> pass1Revolve = {colorRevolveRef, shadeRevolveRef};
            vk::SubpassDescription subpass({}, vk::PipelineBindPoint::eGraphics, {}, pass1Refs, pass1Revolve, &depthRef, {});

            std::array<vk::SubpassDependency, 2> dependencies = {
                vk::SubpassDependency(VK_SUBPASS_EXTERNAL, 0,
                    vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests,
                    vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests,
                    {}, vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentWrite),
                vk::SubpassDependency(0, VK_SUBPASS_EXTERNAL,
                    vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests,
                    vk::PipelineStageFlagBits::eColorAttachmentOutput,
                    vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentWrite, vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eColorAttachmentRead)
            };
            vk::RenderPassCreateInfo renderpass_info({}, attachments, subpass, dependencies);

            mainRenderPass = device.createRenderPassUnique(renderpass_info);
            debugName(device, mainRenderPass.get(), "Render Test Main Render Pass");
        }
        {
            std::array<vk::AttachmentDescription, 3> attachments = {
        /*0*/    vk::AttachmentDescription({}, vk::Format::eR8G8B8A8Srgb, vk::SampleCountFlagBits::e1, // color
                    vk::AttachmentLoadOp::eLoad, vk::AttachmentStoreOp::eDontCare,
                    vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare,
                    vk::ImageLayout::eShaderReadOnlyOptimal, vk::ImageLayout::eGeneral),
        /*1*/    vk::AttachmentDescription({}, vk::Format::eR32G32B32A32Sfloat, vk::SampleCountFlagBits::e1, // shade
                    vk::AttachmentLoadOp::eLoad, vk::AttachmentStoreOp::eDontCare,
                    vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare,
                    vk::ImageLayout::eShaderReadOnlyOptimal, vk::ImageLayout::eGeneral),
        /*2*/    vk::AttachmentDescription({}, win->swapchainFormat.format, vk::SampleCountFlagBits::e1, // final
                    vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore,
                    vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare,
                    vk::ImageLayout::eUndefined, vk::ImageLayout::ePresentSrcKHR)
            };
            vk::AttachmentReference colorReadRef(0, vk::ImageLayout::eShaderReadOnlyOptimal);
            vk::AttachmentReference shadingReadRef(1, vk::ImageLayout::eShaderReadOnlyOptimal);
            vk::AttachmentReference finalWriteRef(2, vk::ImageLayout::eColorAttachmentOptimal);

            std::array<vk::AttachmentReference, 2> pass1In = {colorReadRef, shadingReadRef};
            vk::SubpassDescription subpass({}, vk::PipelineBindPoint::eGraphics, pass1In, finalWriteRef, {}, nullptr, {});

            std::array<vk::SubpassDependency, 2> dependencies = {
                vk::SubpassDependency(VK_SUBPASS_EXTERNAL, 0,
                    vk::PipelineStageFlagBits::eColorAttachmentOutput,
                    vk::PipelineStageFlagBits::eColorAttachmentOutput,
                    {}, vk::AccessFlagBits::eColorAttachmentWrite),
                vk::SubpassDependency(0, VK_SUBPASS_EXTERNAL,
                    vk::PipelineStageFlagBits::eColorAttachmentOutput,
                    vk::PipelineStageFlagBits::eColorAttachmentOutput,
                    vk::AccessFlagBits::eColorAttachmentWrite, vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eColorAttachmentRead)
            };
            vk::RenderPassCreateInfo renderpass_info({}, attachments, subpass, dependencies);

            shadeRenderPass = device.createRenderPassUnique(renderpass_info);
            debugName(device, shadeRenderPass.get(), "Render Test Shade Render Pass");
        }
        {
            vk::AttachmentDescription attachment({}, win->swapchainFormat.format, vk::SampleCountFlagBits::e1,
                vk::AttachmentLoadOp::eLoad, vk::AttachmentStoreOp::eStore,
                vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare,
                vk::ImageLayout::ePresentSrcKHR, vk::ImageLayout::ePresentSrcKHR);
            vk::AttachmentReference ref(0, vk::ImageLayout::eColorAttachmentOptimal);
            vk::SubpassDependency dep(VK_SUBPASS_EXTERNAL, 0,
                vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::PipelineStageFlagBits::eColorAttachmentOutput,
                vk::AccessFlagBits::eColorAttachmentWrite, vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eColorAttachmentRead);
            vk::SubpassDescription subpass({}, vk::PipelineBindPoint::eGraphics, {}, ref);
            vk::RenderPassCreateInfo renderpass_info({}, attachment, subpass, dep);
            overlayPass = device.createRenderPassUnique(renderpass_info);
            debugName(device, overlayPass.get(), "Render Test Overlay Render Pass");
        }

        {
            std::array<vk::DescriptorSetLayoutBinding, 2> bindings = {
                vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eUniformBufferDynamic, 1, vk::ShaderStageFlagBits::eVertex),
                vk::DescriptorSetLayoutBinding(1, vk::DescriptorType::eUniformBufferDynamic, 1, vk::ShaderStageFlagBits::eVertex),
            };
            vk::DescriptorSetLayoutCreateInfo layout_info({}, bindings);
            mainDescriptorLayout = device.createDescriptorSetLayoutUnique(layout_info);
            debugName(device, mainDescriptorLayout.get(), "Render Test Main Descriptor Layout");
        }
        {
            std::array<vk::DescriptorSetLayoutBinding, 3> bindings = {
                vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eInputAttachment, 1, vk::ShaderStageFlagBits::eFragment),
                vk::DescriptorSetLayoutBinding(1, vk::DescriptorType::eInputAttachment, 1, vk::ShaderStageFlagBits::eFragment),
                vk::DescriptorSetLayoutBinding(2, vk::DescriptorType::eUniformBufferDynamic, 1, vk::ShaderStageFlagBits::eFragment)
            };
            vk::DescriptorSetLayoutCreateInfo layout_info({}, bindings);
            shadeDescriptorLayout = device.createDescriptorSetLayoutUnique(layout_info);
            debugName(device, shadeDescriptorLayout.get(), "Render Test Shading Descriptor Layout");
        }
        {
            std::array<vk::DescriptorSetLayoutBinding, 1> bindings = {
                vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment),
            };
            vk::DescriptorSetLayoutCreateInfo layout_info({}, bindings);
            shadowMapDescriptorLayout = device.createDescriptorSetLayoutUnique(layout_info);
            debugName(device, shadowMapDescriptorLayout.get(), "Render Test Shadow Map Descriptor Layout");
        }
        {
            std::array<vk::DescriptorSetLayoutBinding, 1> bindings = {
                vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment),
            };
            vk::DescriptorSetLayoutCreateInfo layout_info({}, bindings);
            textureDescriptorLayout = device.createDescriptorSetLayoutUnique(layout_info);
            debugName(device, textureDescriptorLayout.get(), "Render Test Texture Descriptor Layout");
        }

        {
            std::array<vk::DescriptorSetLayout, 2> layouts = {
                mainDescriptorLayout.get(), textureDescriptorLayout.get()
            };
            vk::PipelineLayoutCreateInfo layout_info({}, layouts, {});
            mainPipelineLayout = device.createPipelineLayoutUnique(layout_info);
            debugName(device, mainPipelineLayout.get(), "Render Test Main Pipeline Layout");
        }
        {
            std::array<vk::DescriptorSetLayout, 2> layouts = {
                shadeDescriptorLayout.get(), shadowMapDescriptorLayout.get()
            };
            vk::PipelineLayoutCreateInfo layout_info({}, layouts, {});
            shadePipelineLayout = device.createPipelineLayoutUnique(layout_info);
            debugName(device, shadePipelineLayout.get(), "Render Test Shade Pipeline Layout");
        }
        {
            vk::PipelineInputAssemblyStateCreateInfo input_assembly({}, vk::PrimitiveTopology::eTriangleList);
            vk::PipelineTessellationStateCreateInfo tesselation({}, {});

            vk::Viewport v{};
            vk::Rect2D s{};
            vk::PipelineViewportStateCreateInfo viewport({}, v, s);

            vk::PipelineRasterizationStateCreateInfo rasterization({}, false, false, vk::PolygonMode::eFill, vk::CullModeFlagBits::eBack, vk::FrontFace::eCounterClockwise, false, 0.0f, 0.0f, 0.0f, 1.0f);

            std::array<vk::DynamicState, 2> dynamicStates{vk::DynamicState::eViewport, vk::DynamicState::eScissor};
            vk::PipelineDynamicStateCreateInfo dynamic({}, dynamicStates);

            {
                vk::VertexInputBindingDescription inputBinding(0, sizeof(vertex_data), vk::VertexInputRate::eVertex);
                auto inputAttributes = vertex_data::attributes(0);
                vk::PipelineVertexInputStateCreateInfo vertex_input({}, inputBinding, inputAttributes);

                vk::UniqueShaderModule vertexShader = createShader(device, "test/render.vert");
                vk::UniqueShaderModule fragmentShader = createShader(device, "test/render.frag");
                std::array<vk::PipelineShaderStageCreateInfo, 2> shaders = {
                    vk::PipelineShaderStageCreateInfo({}, vk::ShaderStageFlagBits::eVertex, vertexShader.get(), "main"),
                    vk::PipelineShaderStageCreateInfo({}, vk::ShaderStageFlagBits::eFragment, fragmentShader.get(), "main")
                };

                vk::PipelineDepthStencilStateCreateInfo depthStencil({}, true, true, vk::CompareOp::eLess, false, false);

                std::array<vk::PipelineColorBlendAttachmentState, 2> attachments = {
                    vk::PipelineColorBlendAttachmentState(false).setColorWriteMask(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA),
                    vk::PipelineColorBlendAttachmentState(false).setColorWriteMask(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA)
                };
                vk::PipelineColorBlendStateCreateInfo colorBlend({}, false, vk::LogicOp::eClear, attachments);
                vk::PipelineMultisampleStateCreateInfo multisample({}, CONFIG.sampleCount);

                vk::GraphicsPipelineCreateInfo pipeline_info({}, shaders, &vertex_input,
                    &input_assembly, &tesselation, &viewport, &rasterization, &multisample, &depthStencil, &colorBlend, &dynamic, mainPipelineLayout.get(), mainRenderPass.get(), 0);
                mainPipeline = device.createGraphicsPipelineUnique({}, pipeline_info).value;
                debugName(device, mainPipeline.get(), "Render Test Main Pipeline");
            }
            {
                vk::PipelineVertexInputStateCreateInfo vertex_input({}, {}, {});
                vk::PipelineInputAssemblyStateCreateInfo input_assembly({}, vk::PrimitiveTopology::eLineList);

                vk::UniqueShaderModule vertexShader = createShader(device, "test/hitbox.vert");
                vk::UniqueShaderModule fragmentShader = createShader(device, "test/hitbox.frag");
                std::array<vk::PipelineShaderStageCreateInfo, 2> shaders = {
                    vk::PipelineShaderStageCreateInfo({}, vk::ShaderStageFlagBits::eVertex, vertexShader.get(), "main"),
                    vk::PipelineShaderStageCreateInfo({}, vk::ShaderStageFlagBits::eFragment, fragmentShader.get(), "main")
                };

                vk::PipelineRasterizationStateCreateInfo rasterization({}, false, false, vk::PolygonMode::eLine, vk::CullModeFlagBits::eNone, vk::FrontFace::eCounterClockwise, false, 0.0f, 0.0f, 0.0f, 5.0f);
                vk::PipelineDepthStencilStateCreateInfo depthStencil({}, true, true, vk::CompareOp::eLessOrEqual, false, false);

                std::array<vk::PipelineColorBlendAttachmentState, 2> attachments = {
                    vk::PipelineColorBlendAttachmentState(false).setColorWriteMask(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA),
                    vk::PipelineColorBlendAttachmentState(false).setColorWriteMask(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA)
                };
                vk::PipelineColorBlendStateCreateInfo colorBlend({}, false, vk::LogicOp::eClear, attachments);
                vk::PipelineMultisampleStateCreateInfo multisample({}, CONFIG.sampleCount);

                vk::GraphicsPipelineCreateInfo pipeline_info({}, shaders, &vertex_input,
                    &input_assembly, &tesselation, &viewport, &rasterization, &multisample, &depthStencil, &colorBlend, &dynamic, mainPipelineLayout.get(), mainRenderPass.get(), 0);
                hitboxPipeline = device.createGraphicsPipelineUnique({}, pipeline_info).value;
                debugName(device, hitboxPipeline.get(), "Render Test Hitbox Pipeline");
            }
            {
                vk::VertexInputBindingDescription inputBinding(0, sizeof(vertex_data), vk::VertexInputRate::eVertex);
                auto inputAttributes = vertex_data::attributes(0);
                vk::PipelineVertexInputStateCreateInfo vertex_input({}, inputBinding, inputAttributes);

                vk::UniqueShaderModule vertexShader = createShader(device, "test/render.vert");
                vk::UniqueShaderModule fragmentShader = createShader(device, "test/shadow.frag");
                std::array<vk::PipelineShaderStageCreateInfo, 2> shaders = {
                    vk::PipelineShaderStageCreateInfo({}, vk::ShaderStageFlagBits::eVertex, vertexShader.get(), "main"),
                    vk::PipelineShaderStageCreateInfo({}, vk::ShaderStageFlagBits::eFragment, fragmentShader.get(), "main")
                };

                vk::PipelineRasterizationStateCreateInfo rasterization({}, false, false, vk::PolygonMode::eFill, vk::CullModeFlagBits::eFront, vk::FrontFace::eCounterClockwise, false, 0.0f, 0.0f, 0.0f, 1.0f);
                vk::PipelineMultisampleStateCreateInfo multisample({}, vk::SampleCountFlagBits::e1);
                vk::PipelineDepthStencilStateCreateInfo depthStencil({}, true, true, vk::CompareOp::eLess, false, false);

                vk::PipelineColorBlendStateCreateInfo colorBlend({}, false, vk::LogicOp::eClear, {});

                vk::GraphicsPipelineCreateInfo pipeline_info({}, shaders, &vertex_input,
                    &input_assembly, &tesselation, &viewport, &rasterization, &multisample, &depthStencil, &colorBlend, &dynamic, mainPipelineLayout.get(), shadowRenderPass.get(), 0);
                shadowPipeline = device.createGraphicsPipelineUnique({}, pipeline_info).value;
                debugName(device, shadowPipeline.get(), "Render Test Shadow Pipeline");
            }
            {
                vk::PipelineVertexInputStateCreateInfo vertex_input({}, {}, {});

                vk::UniqueShaderModule vertexShader = createShader(device, "test/shade.vert");
                vk::UniqueShaderModule fragmentShader = createShader(device, "test/shade.frag");
                std::array<vk::PipelineShaderStageCreateInfo, 2> shaders = {
                    vk::PipelineShaderStageCreateInfo({}, vk::ShaderStageFlagBits::eVertex, vertexShader.get(), "main"),
                    vk::PipelineShaderStageCreateInfo({}, vk::ShaderStageFlagBits::eFragment, fragmentShader.get(), "main")
                };

                vk::PipelineDepthStencilStateCreateInfo depthStencil({}, false, false);

                vk::PipelineColorBlendAttachmentState attachment(true, vk::BlendFactor::eOne, vk::BlendFactor::eOne, vk::BlendOp::eAdd);
                attachment.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
                vk::PipelineColorBlendStateCreateInfo colorBlend({}, false, vk::LogicOp::eClear, attachment);
                vk::PipelineMultisampleStateCreateInfo multisample({}, vk::SampleCountFlagBits::e1);

                vk::GraphicsPipelineCreateInfo pipeline_info({}, shaders, &vertex_input,
                    &input_assembly, &tesselation, &viewport, &rasterization, &multisample, &depthStencil, &colorBlend, &dynamic, shadePipelineLayout.get(), shadeRenderPass.get(), 0);
                shadePipeline = device.createGraphicsPipelineUnique({}, pipeline_info).value;
                debugName(device, shadePipeline.get(), "Render Test Shading Pipeline");
            }
        }
        shadowSampler = device.createSamplerUnique(vk::SamplerCreateInfo(
            {}, vk::Filter::eLinear, vk::Filter::eLinear,
            vk::SamplerMipmapMode::eNearest,
            vk::SamplerAddressMode::eClampToBorder, vk::SamplerAddressMode::eClampToBorder, vk::SamplerAddressMode::eClampToBorder,
            {}, false, {}, false, vk::CompareOp::eNever, 0, 0, vk::BorderColor::eFloatOpaqueWhite));
        textureSampler = device.createSamplerUnique(vk::SamplerCreateInfo(
            {}, vk::Filter::eLinear, vk::Filter::eLinear,
            vk::SamplerMipmapMode::eNearest,
            vk::SamplerAddressMode::eRepeat, vk::SamplerAddressMode::eRepeat, vk::SamplerAddressMode::eRepeat,
            {}, false, {}, false, vk::CompareOp::eNever, 0, 0, vk::BorderColor::eFloatOpaqueWhite));

        {
            vk::DescriptorPoolSize size(vk::DescriptorType::eCombinedImageSampler, load_textures.size());
            vk::DescriptorPoolCreateInfo pool_info({}, load_textures.size(), size);
            textureDescriptorPool = device.createDescriptorPoolUnique(pool_info);


            std::vector<vk::DescriptorSetLayout> layouts(load_textures.size());
            std::fill(layouts.begin(), layouts.end(), textureDescriptorLayout.get());

            vk::DescriptorSetAllocateInfo set_info(textureDescriptorPool.get(), layouts);
            textureDescriptorSets = device.allocateDescriptorSets(set_info);
        }

        std::vector<vk::WriteDescriptorSet> writes(load_textures.size());
        std::vector<vk::DescriptorImageInfo> imageInfos(load_textures.size());
        textures.resize(load_textures.size());
        for(int i=0; i<load_textures.size(); i++)
        {
            auto& h = load_textures[i];
            textures[i]= std::make_unique<texture>(device, allocator, resource_loader::getImageSize(h.data()));
            textureSets[h] = textureDescriptorSets[i];

            loadingFutures.push_back(loader->loadTexture(textures[i].get(), h.data()));

            imageInfos[i] = vk::DescriptorImageInfo(textureSampler.get(), textures[i]->imageView.get(), vk::ImageLayout::eShaderReadOnlyOptimal);
            writes[i] = vk::WriteDescriptorSet(textureDescriptorSets[i], 0, 0, vk::DescriptorType::eCombinedImageSampler, imageInfos[i]);
        }
        device.updateDescriptorSets(writes, {});

        for(int i=0; i<load_models.size(); i++)
        {
            auto& h = load_models[i];
            models[h] = std::make_unique<model>(device, allocator);
            loadingFutures.push_back(loader->loadModel(models[h].get(), h.data()));
        }

        FT_Library ft;
        FT_Error err = FT_Init_FreeType(&ft);
        font = std::make_unique<font_renderer>("/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf", 128, device, allocator);
        font->preload(ft, loader, overlayPass.get());
        loadingFutures.push_back(font->textureReady);
    }

    void render_test::prepare(std::vector<vk::Image> swapchainImages, std::vector<vk::ImageView> swapchainViews)
    {
        int imageCount = swapchainImages.size();
        std::array<vk::DescriptorPoolSize, 8> sizes = {
            vk::DescriptorPoolSize(vk::DescriptorType::eInputAttachment, 2*imageCount),
            vk::DescriptorPoolSize(vk::DescriptorType::eUniformBufferDynamic, 1*imageCount),

            vk::DescriptorPoolSize(vk::DescriptorType::eUniformBufferDynamic, 1*imageCount),
            vk::DescriptorPoolSize(vk::DescriptorType::eUniformBufferDynamic, 1*imageCount),
            vk::DescriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, 1*imageCount),

            vk::DescriptorPoolSize(vk::DescriptorType::eUniformBufferDynamic, 1*imageCount),
            vk::DescriptorPoolSize(vk::DescriptorType::eUniformBufferDynamic, 1*imageCount),

            vk::DescriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, maxLights*imageCount),
        };
        vk::DescriptorPoolCreateInfo pool_info({}, (3+maxLights)*imageCount, sizes);
        descriptorPool = device.createDescriptorPoolUnique(pool_info);

        std::vector<vk::DescriptorSetLayout> layouts(imageCount);
        vk::DescriptorSetAllocateInfo set_info(descriptorPool.get(), layouts);

        std::fill(layouts.begin(), layouts.end(), shadeDescriptorLayout.get());
        shadeDescriptorSets = device.allocateDescriptorSets(set_info);

        std::fill(layouts.begin(), layouts.end(), mainDescriptorLayout.get());
        mainDescriptorSets = device.allocateDescriptorSets(set_info);
        shadowDescriptorSets = device.allocateDescriptorSets(set_info);

        for(int i=0; i<swapchainViews.size(); i++)
        {
            std::vector<vk::DescriptorSetLayout> layouts(maxLights);
            std::fill(layouts.begin(), layouts.end(), shadowMapDescriptorLayout.get());
            vk::DescriptorSetAllocateInfo set_info(descriptorPool.get(), layouts);
            auto a = device.allocateDescriptorSets(set_info);
            shadowMapDescriptorSets.push_back(a);
        }

        commandBuffers = device.allocateCommandBuffersUnique(
            vk::CommandBufferAllocateInfo(pool.get(), vk::CommandBufferLevel::ePrimary, swapchainImages.size()));
        this->swapchainImages = swapchainImages;

        std::vector<vk::WriteDescriptorSet> writes;
        std::vector<vk::DescriptorImageInfo> imageInfos; imageInfos.reserve(1024);
        std::vector<vk::DescriptorBufferInfo> bufferInfos; bufferInfos.reserve(1024);

        shadowBuffers.resize(swapchainViews.size());
        shadowFramebuffers.resize(swapchainViews.size());

        for(int i=0; i<swapchainViews.size(); i++)
        {
            colorBuffers.push_back(std::make_unique<texture>(device, allocator, win->swapchainExtent.width, win->swapchainExtent.height,
                vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eInputAttachment, vk::Format::eR8G8B8A8Srgb, CONFIG.sampleCount));
            shadeBuffers.push_back(std::make_unique<texture>(device, allocator, win->swapchainExtent.width, win->swapchainExtent.height,
                vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eInputAttachment, vk::Format::eR32G32B32A32Sfloat, CONFIG.sampleCount));
            depthBuffers.push_back(std::make_unique<texture>(device, allocator, win->swapchainExtent.width, win->swapchainExtent.height,
                vk::ImageUsageFlagBits::eDepthStencilAttachment, vk::Format::eD32Sfloat, CONFIG.sampleCount, false, vk::ImageAspectFlagBits::eDepth));

            colorRevolveBuffers.push_back(std::make_unique<texture>(device, allocator, win->swapchainExtent.width, win->swapchainExtent.height,
                vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransientAttachment | vk::ImageUsageFlagBits::eInputAttachment, vk::Format::eR8G8B8A8Srgb, vk::SampleCountFlagBits::e1, false));
            shadeRevolveBuffers.push_back(std::make_unique<texture>(device, allocator, win->swapchainExtent.width, win->swapchainExtent.height,
                vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransientAttachment | vk::ImageUsageFlagBits::eInputAttachment, vk::Format::eR32G32B32A32Sfloat, vk::SampleCountFlagBits::e1, false));

            {
                colorBuffers.back()->name("Render Test Color #"+std::to_string(i));
                shadeBuffers.back()->name("Render Test Shade #"+std::to_string(i));
                depthBuffers.back()->name("Render Test Depth #"+std::to_string(i));
                colorRevolveBuffers.back()->name("Render Test Color Revolve #"+std::to_string(i));
                shadeRevolveBuffers.back()->name("Render Test Shade Revolve #"+std::to_string(i));
            }

            for(int j=0; j<maxLights; j++)
            {
                shadowBuffers[i].push_back(std::make_unique<texture>(device, allocator, CONFIG.shadowResolution, CONFIG.shadowResolution,
                    vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eSampled, vk::Format::eD32Sfloat, vk::SampleCountFlagBits::e1, false, vk::ImageAspectFlagBits::eDepth));
                shadowBuffers[i].back()->name("Render Test Light Depth #"+std::to_string(i)+":"+std::to_string(j));
            }

            {
                std::array<vk::ImageView, 5> imageViews = {
                    colorBuffers.back()->imageView.get(),
                    shadeBuffers.back()->imageView.get(),
                    depthBuffers.back()->imageView.get(),

                    colorRevolveBuffers.back()->imageView.get(),
                    shadeRevolveBuffers.back()->imageView.get()
                };
                vk::FramebufferCreateInfo framebuffer_info({}, mainRenderPass.get(), imageViews,
                    win->swapchainExtent.width, win->swapchainExtent.height, 1);
                mainFramebuffers.push_back(device.createFramebufferUnique(framebuffer_info));
                debugName(device, mainFramebuffers.back().get(), "Render Test Main Framebuffer #"+std::to_string(i));
            }
            {
                std::array<vk::ImageView, 3> imageViews = {
                    colorRevolveBuffers.back()->imageView.get(),
                    shadeRevolveBuffers.back()->imageView.get(),
                    swapchainViews[i]
                };
                vk::FramebufferCreateInfo framebuffer_info({}, shadeRenderPass.get(), imageViews,
                    win->swapchainExtent.width, win->swapchainExtent.height, 1);
                shadeFramebuffers.push_back(device.createFramebufferUnique(framebuffer_info));
                debugName(device, shadeFramebuffers.back().get(), "Render Test Shade Framebuffer #"+std::to_string(i));
            }
            {
                for(int j=0; j<maxLights; j++)
                {
                    vk::FramebufferCreateInfo framebuffer_info({}, shadowRenderPass.get(), shadowBuffers[i][j]->imageView.get(),
                        CONFIG.shadowResolution, CONFIG.shadowResolution, 1);
                    shadowFramebuffers[i].push_back(device.createFramebufferUnique(framebuffer_info));
                    debugName(device, shadowFramebuffers[i].back().get(), "Render Test Shadow Framebuffer #"+std::to_string(i)+":"+std::to_string(j));
                }
            }
            {
                vk::FramebufferCreateInfo framebuffer_info({}, overlayPass.get(), swapchainViews[i],
                    win->swapchainExtent.width, win->swapchainExtent.height, 1);
                overlayFramebuffers.push_back(device.createFramebufferUnique(framebuffer_info));
                debugName(device, overlayFramebuffers.back().get(), "Render Test Overlay Framebuffer #"+std::to_string(i));
            }

            debugName(device, commandBuffers[i].get(), "Render Test Command Buffer #"+std::to_string(i));

            {
                vk::BufferCreateInfo buffer_info({}, maxLights*sizeof(LightInfo), vk::BufferUsageFlagBits::eUniformBuffer, vk::SharingMode::eExclusive);
                vma::AllocationCreateInfo alloc_info({}, vma::MemoryUsage::eCpuToGpu);
                auto [lb, la] = allocator.createBuffer(buffer_info, alloc_info);
                lightUniformBuffers.push_back(lb);
                lightUniformAllocations.push_back(la);
                lightUniformPointers.push_back((LightInfo*)allocator.mapMemory(la));
                debugName(device, lightUniformBuffers[i], "Render Test Light Uniform Buffer #"+std::to_string(i));
            }
            {
                vk::BufferCreateInfo buffer_info({}, sizeof(GlobalInfo), vk::BufferUsageFlagBits::eUniformBuffer, vk::SharingMode::eExclusive);
                vma::AllocationCreateInfo alloc_info({}, vma::MemoryUsage::eCpuToGpu);
                auto [gb, ga] = allocator.createBuffer(buffer_info, alloc_info);
                globalUniformBuffers.push_back(gb);
                globalUniformAllocations.push_back(ga);
                globalUniformPointers.push_back((GlobalInfo*)allocator.mapMemory(ga));
                debugName(device, globalUniformBuffers[i], "Render Test Global Uniform Buffer #"+std::to_string(i));
            }
            {
                vk::BufferCreateInfo buffer_info({}, maxLights*sizeof(GlobalInfo), vk::BufferUsageFlagBits::eUniformBuffer, vk::SharingMode::eExclusive);
                vma::AllocationCreateInfo alloc_info({}, vma::MemoryUsage::eCpuToGpu);
                auto [gb, ga] = allocator.createBuffer(buffer_info, alloc_info);
                globalShadowUniformBuffers.push_back(gb);
                globalShadowUniformAllocations.push_back(ga);
                globalShadowUniformPointers.push_back((GlobalInfo*)allocator.mapMemory(ga));
                debugName(device, globalShadowUniformBuffers[i], "Render Test Global Shadow Uniform Buffer #"+std::to_string(i));
            }
            {
                vk::BufferCreateInfo buffer_info({}, maxObjects*sizeof(ModelInfo), vk::BufferUsageFlagBits::eUniformBuffer, vk::SharingMode::eExclusive);
                vma::AllocationCreateInfo alloc_info({}, vma::MemoryUsage::eCpuToGpu);
                auto [mb, ma] = allocator.createBuffer(buffer_info, alloc_info);
                modelUniformBuffers.push_back(mb);
                modelUniformAllocations.push_back(ma);
                modelUniformPointers.push_back((ModelInfo*)allocator.mapMemory(ma));
                debugName(device, modelUniformBuffers[i], "Render Test Model Uniform Buffer #"+std::to_string(i));
            }

            imageInfos.push_back(vk::DescriptorImageInfo({}, colorRevolveBuffers.back()->imageView.get(), vk::ImageLayout::eShaderReadOnlyOptimal));
            imageInfos.push_back(vk::DescriptorImageInfo({}, shadeRevolveBuffers.back()->imageView.get(), vk::ImageLayout::eShaderReadOnlyOptimal));

            writes.push_back(vk::WriteDescriptorSet(shadeDescriptorSets[i], 0, 0, 2, vk::DescriptorType::eInputAttachment, imageInfos.data()+(2+maxLights)*i));

            bufferInfos.push_back(vk::DescriptorBufferInfo(lightUniformBuffers[i], 0, sizeof(LightInfo)));
            writes.push_back(vk::WriteDescriptorSet(shadeDescriptorSets[i], 2, 0, vk::DescriptorType::eUniformBufferDynamic, {},  bufferInfos.back()));

            for(int j=0; j<maxLights; j++)
            {
                imageInfos.push_back(vk::DescriptorImageInfo(shadowSampler.get(), shadowBuffers[i][j]->imageView.get(), vk::ImageLayout::eShaderReadOnlyOptimal));
                writes.push_back(vk::WriteDescriptorSet(shadowMapDescriptorSets[i][j], 0, 0, vk::DescriptorType::eCombinedImageSampler, imageInfos.back()));
            }

            bufferInfos.push_back(vk::DescriptorBufferInfo(globalUniformBuffers[i], 0, sizeof(GlobalInfo)));
            writes.push_back(vk::WriteDescriptorSet(mainDescriptorSets[i], 0, 0, vk::DescriptorType::eUniformBufferDynamic, {},  bufferInfos.back()));

            bufferInfos.push_back(vk::DescriptorBufferInfo(modelUniformBuffers[i], 0, sizeof(ModelInfo)));
            writes.push_back(vk::WriteDescriptorSet(mainDescriptorSets[i], 1, 0, vk::DescriptorType::eUniformBufferDynamic, {},  bufferInfos.back()));


            bufferInfos.push_back(vk::DescriptorBufferInfo(globalShadowUniformBuffers[i], 0, sizeof(GlobalInfo)));
            writes.push_back(vk::WriteDescriptorSet(shadowDescriptorSets[i], 0, 0, vk::DescriptorType::eUniformBufferDynamic, {},  bufferInfos.back()));

            bufferInfos.push_back(vk::DescriptorBufferInfo(modelUniformBuffers[i], 0, sizeof(ModelInfo)));
            writes.push_back(vk::WriteDescriptorSet(shadowDescriptorSets[i], 1, 0, vk::DescriptorType::eUniformBufferDynamic, {},  bufferInfos.back()));
        }

        device.updateDescriptorSets(writes, {});

        font->prepare(imageCount);
    }

    void render_test::init()
    {

    }

    void render_test::set_camera(entity::entity_id entity)
    {
        camera = entity;
    }

    void render_test::render(int frame, vk::Semaphore imageAvailable, vk::Semaphore renderFinished, vk::Fence fence)
    {
        vk::UniqueCommandBuffer& commandBuffer = commandBuffers[frame];
        commandBuffer->begin(vk::CommandBufferBeginInfo());
        vk::DebugUtilsLabelEXT label{};

        std::map<entity::entity_id, int> entityDescriptors;
        {
            int j=0;
            for(auto [e2, p2, m2] : modelView.each())
            {
                modelUniformPointers[frame][j].transform = glm::mat4(1.0)
                    * glm::translate(glm::mat4(1.0), (glm::vec3)p2);
                entityDescriptors[e2] = j;

                const entity::components::rotation* rotation;
                if((rotation=entities.try_get<const entity::components::rotation>(e2)) != nullptr)
                {
                    modelUniformPointers[frame][j].transform *= glm::rotate(glm::mat4(1.0), (float)rotation->yaw, glm::vec3(0.0, 1.0, 0.0));
                    //spdlog::debug("Rotation {}", rotation->yaw);
                }

                j++;
                if(j > maxObjects)
                    break;
            }
        }

        {
            vk::Viewport viewport(0.0f, 0.0f, (float)CONFIG.shadowResolution, (float)CONFIG.shadowResolution, 0.0f, 1.0f);
            vk::Rect2D scissor({0,0}, {CONFIG.shadowResolution, CONFIG.shadowResolution});
            commandBuffer->setViewport(0, viewport);
            commandBuffer->setScissor(0, scissor);
        }
        commandBuffer->beginDebugUtilsLabelEXT(label.setPLabelName("Shadow Render"));
        int l=0;
        std::map<entity::entity_id, int> lightDescriptors;
        for(auto [entity, position, light] : lightView.each())
        {
            lightDescriptors[entity] = l;

            vk::ClearValue depthClear = vk::ClearDepthStencilValue(1.0f, 0);
            commandBuffer->beginRenderPass(vk::RenderPassBeginInfo(shadowRenderPass.get(), shadowFramebuffers[frame][l].get(),
                vk::Rect2D({0, 0}, {CONFIG.shadowResolution, CONFIG.shadowResolution}), depthClear), vk::SubpassContents::eInline);

            //globalShadowUniformPointers[frame]->projection = glm::perspective(light.fov, 1.0f, light.zNear, light.zFar);
            globalShadowUniformPointers[frame][l].projection = glm::ortho(-5.0f, 5.0f, -5.0f, 5.0f, light.zNear, light.zFar);
            globalShadowUniformPointers[frame][l].view = glm::lookAt((glm::vec3)position, glm::vec3(0.0, 0.0, 0.0), glm::vec3(0.0, -1.0, 0.0));
            commandBuffer->bindPipeline(vk::PipelineBindPoint::eGraphics, shadowPipeline.get());

            int j=0;
            for(auto [e2, p2, m2] : modelView.each())
            {
                const entity::components::renderable r = entities.get<entity::components::renderable>(e2);
                if(!r.shadowCaster)
                    continue;

                auto& model = models[m2.model_name];
                commandBuffer->bindVertexBuffers(0, model->vertexBuffer, {0L});
                commandBuffer->bindIndexBuffer(model->indexBuffer, 0, vk::IndexType::eUint32);

                int q = entityDescriptors[e2];
                commandBuffer->bindDescriptorSets(vk::PipelineBindPoint::eGraphics, mainPipelineLayout.get(), 0, shadowDescriptorSets[frame],
                    {(uint32_t)(l*sizeof(GlobalInfo)), (uint32_t)(q*sizeof(ModelInfo))});
                commandBuffer->drawIndexed(model->indexCount, 1, 0, 0, 0);

                j++;
                if(j > maxObjects)
                    break;
            }
            commandBuffer->endRenderPass();

            l++;
            if(l>maxLights)
                return;
        }
        commandBuffer->endDebugUtilsLabelEXT();

         std::array<vk::ClearValue, 4> mainClear = {
            vk::ClearColorValue(std::array<float, 4>{0.0f, 0.0f, 0.0f, 0.0f}),
            vk::ClearColorValue(std::array<float, 4>{0.0f, 0.0f, 0.0f, 0.0f}),
            vk::ClearDepthStencilValue(1.0f, 0),
            vk::ClearColorValue(std::array<float, 4>{0.0f, 0.0f, 0.0f, 0.0f})
        };

        {
            vk::Viewport viewport(0.0f, 0.0f, win->swapchainExtent.width, win->swapchainExtent.height, 0.0f, 1.0f);
            vk::Rect2D scissor({0,0}, win->swapchainExtent);
            commandBuffer->setViewport(0, viewport);
            commandBuffer->setScissor(0, scissor);
        }
        commandBuffer->beginDebugUtilsLabelEXT(label.setPLabelName("Main Render"));
        commandBuffer->beginRenderPass(vk::RenderPassBeginInfo(mainRenderPass.get(), mainFramebuffers[frame].get(),
            vk::Rect2D({0, 0}, win->swapchainExtent), mainClear), vk::SubpassContents::eInline);
        commandBuffer->bindPipeline(vk::PipelineBindPoint::eGraphics, mainPipeline.get());

        entity::components::target_camera cam = entities.get<entity::components::target_camera>(camera);
        entity::components::position camTarget = entities.get<entity::components::position>(cam.target);
        globalUniformPointers[frame]->projection = glm::perspective(cam.fov,
            win->swapchainExtent.width / (float) win->swapchainExtent.height, cam.zNear, cam.zFar);
        globalUniformPointers[frame]->view =
            glm::scale(glm::mat4(1.0), glm::vec3(1.0, -1.0, 1.0)) *
            glm::translate(glm::mat4(1.0), glm::vec3(0.0, 0.0, -cam.distance)) *
            glm::rotate(glm::mat4(1.0), cam.pitch, glm::vec3(1.0, 0.0, 0.0)) *
            glm::rotate(glm::mat4(1.0), cam.yaw, glm::vec3(0.0, 1.0, 0.0)) *
            glm::translate(glm::mat4(1.0), -(glm::vec3)camTarget - cam.offset) *
            glm::mat4(1.0);

        for(auto [e2, p2, m2] : modelView.each())
        {
            auto& model = models[m2.model_name];
            auto& textureSet = textureSets[m2.texture_name];
            int q = entityDescriptors[e2];

            commandBuffer->bindVertexBuffers(0, model->vertexBuffer, {0L});
            commandBuffer->bindIndexBuffer(model->indexBuffer, 0, vk::IndexType::eUint32);

            commandBuffer->bindDescriptorSets(vk::PipelineBindPoint::eGraphics, mainPipelineLayout.get(), 0, mainDescriptorSets[frame],
                {0U, (uint32_t)(q*sizeof(ModelInfo))});
            commandBuffer->bindDescriptorSets(vk::PipelineBindPoint::eGraphics, mainPipelineLayout.get(), 1, textureSet, {});
            commandBuffer->drawIndexed(model->indexCount, 1, 0, 0, 0);
        }

        commandBuffer->bindPipeline(vk::PipelineBindPoint::eGraphics, hitboxPipeline.get());
        for(auto [e2, p2, m2, c2] : hitboxView.each())
        {
            int q = entityDescriptors[e2];

            auto& model = models[m2.model_name];

            glm::vec3 m = model->min;
            glm::vec3 p = model->max;

            const std::array<const glm::vec3, 8> corners = {
                glm::vec3(m.x, m.y, m.z),
                glm::vec3(p.x, m.y, m.z),
                glm::vec3(m.x, p.y, m.z),
                glm::vec3(m.x, m.y, p.z),
                glm::vec3(p.x, p.y, m.z),
                glm::vec3(m.x, p.y, p.z),
                glm::vec3(p.x, m.y, p.z),
                glm::vec3(p.x, p.y, p.z),
            };
            c2.min = glm::vec3(std::numeric_limits<float>::max());
            c2.max = glm::vec3(std::numeric_limits<float>::lowest());
            for(const auto v : corners)
            {
                glm::vec3 t = modelUniformPointers[frame][q].transform * glm::vec4(v, 1.0);
                c2.min = glm::min(c2.min, t);
                c2.max = glm::max(c2.max, t);
            }
            modelUniformPointers[frame][q].min = glm::vec4(c2.min, 0.0);
            modelUniformPointers[frame][q].max = glm::vec4(c2.max, 0.0);

            commandBuffer->bindDescriptorSets(vk::PipelineBindPoint::eGraphics, mainPipelineLayout.get(), 0, mainDescriptorSets[frame],
                {0U, (uint32_t)(q*sizeof(ModelInfo))});
            commandBuffer->draw(24, 1, 0, 0);
        }

        commandBuffer->endRenderPass();
        commandBuffer->endDebugUtilsLabelEXT();

        commandBuffer->beginDebugUtilsLabelEXT(label.setPLabelName("Shading"));
        std::array<vk::ClearValue, 3> shadeClear = {
            vk::ClearColorValue(std::array<float, 4>{0.0f, 0.0f, 0.0f, 0.0f}),
            vk::ClearColorValue(std::array<float, 4>{0.0f, 0.0f, 0.0f, 0.0f}),
            vk::ClearColorValue(std::array<float, 4>{0.0f, 0.0f, 0.0f, 0.0f})
        };
        commandBuffer->beginRenderPass(vk::RenderPassBeginInfo(shadeRenderPass.get(), shadeFramebuffers[frame].get(),
            vk::Rect2D({0, 0}, win->swapchainExtent), shadeClear), vk::SubpassContents::eInline);
        commandBuffer->bindPipeline(vk::PipelineBindPoint::eGraphics, shadePipeline.get());
        for(auto [entity, position, light] : lightView.each())
        {
            int l = lightDescriptors[entity];

            lightUniformPointers[frame][l].position = glm::vec4((glm::vec3)position, 1.0);
            lightUniformPointers[frame][l].color = glm::vec4(light.color, 1.0);
            lightUniformPointers[frame][l].lightMatrix = globalShadowUniformPointers[frame][l].projection * globalShadowUniformPointers[frame][l].view;
            lightUniformPointers[frame][l].globalInverse = glm::inverse(globalUniformPointers[frame]->projection * globalUniformPointers[frame]->view);

            commandBuffer->bindDescriptorSets(vk::PipelineBindPoint::eGraphics, shadePipelineLayout.get(), 0, shadeDescriptorSets[frame], {(uint32_t)(l*sizeof(LightInfo))});
            commandBuffer->bindDescriptorSets(vk::PipelineBindPoint::eGraphics, shadePipelineLayout.get(), 1, shadowMapDescriptorSets[frame][l], {});
            commandBuffer->draw(6, 1, 0, 0);
        }
        commandBuffer->endRenderPass();
        commandBuffer->endDebugUtilsLabelEXT();

        commandBuffer->beginDebugUtilsLabelEXT(label.setPLabelName("Overlay Render"));
        commandBuffer->beginRenderPass(vk::RenderPassBeginInfo(overlayPass.get(), overlayFramebuffers[frame].get(),
            vk::Rect2D({0, 0}, win->swapchainExtent), {}), vk::SubpassContents::eInline);

        auto budget = allocator.getHeapBudgets().front();
        auto usage = ((double)budget.usage) / ((double)budget.budget);

        font->renderText(commandBuffer.get(), frame, "FPS: "+utils::to_fixed_string<1>(win->currentFPS), 0.05f, 0.05f + 0*0.05f, 0.05f);
        font->renderText(commandBuffer.get(), frame, "VRAM: "+utils::to_fixed_string<1>(usage*100.0)+"%", 0.05f, 0.05f + 1*0.05f, 0.05f);
        font->finish(frame);
        commandBuffer->endRenderPass();
        commandBuffer->endDebugUtilsLabelEXT();

        commandBuffer->end();

        vk::PipelineStageFlags waitFlags = vk::PipelineStageFlagBits::eColorAttachmentOutput;
        vk::SubmitInfo submit_info(imageAvailable, waitFlags, commandBuffer.get(), renderFinished);
        graphicsQueue.submit(submit_info, fence);
    }
}
