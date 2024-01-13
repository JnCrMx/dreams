#pragma once

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>

namespace config
{
    class config
    {
        public:
            vk::PresentModeKHR preferredPresentMode = vk::PresentModeKHR::eMailbox; //aka VSync
            vk::SampleCountFlagBits sampleCount = vk::SampleCountFlagBits::e2; // aka Anti-aliasing
            uint32_t shadowResolution = 2048;
    };
    inline class config CONFIG;
}
