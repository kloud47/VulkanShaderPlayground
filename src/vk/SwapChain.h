#pragma once

#include <vulkan/vulkan.h>

#include <cassert>
#include <vector>

namespace vsp {

class VulkanContext;

class Swapchain {
public:
	Swapchain() = default;
	~Swapchain() { assert(m_swapchain == VK_NULL_HANDLE && "Swapchain::shutdown() not called"); }

	Swapchain(const Swapchain&) = delete;
	Swapchain& operator=(const Swapchain&) = delete;

	void init(VulkanContext& ctx, VkExtent2D framebufferExtent, bool vsync);
	void recreate(VkExtent2D framebufferExtent, bool vsync);
	void shutdown();

    VkSwapchainKHR handle()     const { return m_swapchain; }
    VkFormat       format()     const { return m_format; }
    VkExtent2D     extent()     const { return m_extent; }
    uint32_t       imageCount() const { return static_cast<uint32_t>(m_images.size()); }
    VkImage        image(uint32_t i) const { return m_images[i]; }
    VkImageView    view(uint32_t i)  const { return m_views[i]; }
    VkSemaphore    renderFinished(uint32_t i) const { return m_renderFinished[i]; }

private:
    void create(VkExtent2D framebufferExtent, bool vsync, VkSwapchainKHR oldSwapchain);
    void destroyPerImageResources();

    VulkanContext*           m_ctx = nullptr;
    VkSwapchainKHR           m_swapchain = VK_NULL_HANDLE;
    VkFormat                 m_format = VK_FORMAT_UNDEFINED;
    VkColorSpaceKHR          m_colorSpace{};
    VkPresentModeKHR         m_presentMode{};
    VkExtent2D               m_extent{};
    std::vector<VkImage>     m_images;
    std::vector<VkImageView> m_views;
    std::vector<VkSemaphore> m_renderFinished;  
};

}