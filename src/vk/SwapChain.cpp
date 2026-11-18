#include "vk/SwapChain.h"

#include "vk/VulkanContext.h"
#include "vk/VkCheck.h"
#include "core/log.h"

#include <algorithm>


namespace vsp {
namespace {
	VkSurfaceFormatKHR chooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& available)
	{
		for (const auto& f : available)
		{
			if (f.format == VK_FORMAT_B8G8R8A8_SRGB && f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			{
				return f;
			}
			return available[0];
		}
	}
	// vsync on  -> FIFO (always supported, no tearing, cool laptop)
	// vsync off -> MAILBOX, else IMMEDIATE, else FIFO
	VkPresentModeKHR   choosePresentMode(const std::vector<VkPresentModeKHR>& available, bool vsync)
	{
		if (vsync) return VK_PRESENT_MODE_FIFO_KHR;

		auto has = [&](VkPresentModeKHR m) {
			return std::find(available.begin(), available.end(), m) != available.end();
			};
		if (has(VK_PRESENT_MODE_MAILBOX_KHR))   return VK_PRESENT_MODE_MAILBOX_KHR;
		if (has(VK_PRESENT_MODE_IMMEDIATE_KHR)) return VK_PRESENT_MODE_IMMEDIATE_KHR;
		return VK_PRESENT_MODE_FIFO_KHR;
	}
	// If the surface dictates a size, we must use it. Otherwise clamp the window's size.
	VkExtent2D         chooseExtent(const VkSurfaceCapabilitiesKHR& caps, VkExtent2D framebuffer)
	{
		if (caps.currentExtent.width != UINT32_MAX) return caps.currentExtent;
		return {
			std::clamp(framebuffer.width,  caps.minImageExtent.width,  caps.maxImageExtent.width),
			std::clamp(framebuffer.height, caps.minImageExtent.height, caps.maxImageExtent.height),
		};
	}
	// One more than the minimum, so we're not stuck waiting on the driver. maxImageCount 0 = no limit.
	uint32_t    chooseImageCount(const VkSurfaceCapabilitiesKHR& caps)
	{
		uint32_t count = caps.minImageCount + 1;
		if (caps.maxImageCount > 0 && count > caps.maxImageCount)
			count = caps.maxImageCount;
		return count;
	}

}

void Swapchain::init(VulkanContext& ctx, VkExtent2D framebufferExtent, bool vsync)
{
	m_ctx = &ctx;
	create(framebufferExtent, vsync, VK_NULL_HANDLE);
}

void Swapchain::recreate(VkExtent2D framebufferExtent, bool vsync)
{
	vkDeviceWaitIdle(m_ctx->device()); // GPU must be done with the old images
	VkSwapchainKHR oldSwapchain = m_swapchain;
	destroyViews();
	create(framebufferExtent, vsync, oldSwapchain);			// overwrites m_swapchain with the new one
	vkDestroySwapchainKHR(m_ctx->device(), oldSwapchain, nullptr);
}

void Swapchain::shutdown()
{
	if (m_ctx == nullptr) return;

	destroyViews();
	if (m_swapchain != VK_NULL_HANDLE) {
		vkDestroySwapchainKHR(m_ctx->device(), m_swapchain, nullptr);   // also frees m_images
		m_swapchain = VK_NULL_HANDLE;
	}
	m_images.clear();
}

void Swapchain::create(VkExtent2D framebufferExtent, bool vsync, VkSwapchainKHR oldSwapchain)
{
	VkPhysicalDevice gpu	 = m_ctx->physicalDevice();
	VkSurfaceKHR     surface = m_ctx->surface();
	VkDevice		 device  = m_ctx->device();

	// Asking the surface three Questions:
	VkSurfaceCapabilitiesKHR caps{};
	VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(gpu, surface, &caps));

	uint32_t count = 0;
	VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, surface, &count, nullptr));
	std::vector<VkSurfaceFormatKHR> formats(count);
	VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, surface, &count, formats.data()));

	VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(gpu, surface, &count, nullptr));
	std::vector<VkPresentModeKHR> modes(count);
	VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(gpu, surface, &count, modes.data()));

	// Choose:
	const VkSurfaceFormatKHR surfaceFormat = chooseSurfaceFormat(formats);
	m_format		= surfaceFormat.format;
	m_colorSpace	= surfaceFormat.colorSpace;
	m_presentMode	= choosePresentMode(modes, vsync);
	m_extent		= chooseExtent(caps, framebufferExtent);

	// Create the swapchain
	VkSwapchainCreateInfoKHR swapchainInfo{ VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
	swapchainInfo.surface			= surface;
	swapchainInfo.minImageCount		= chooseImageCount(caps);
	swapchainInfo.imageFormat		= m_format;
	swapchainInfo.imageColorSpace	= m_colorSpace;
	swapchainInfo.imageExtent		= m_extent;
	swapchainInfo.imageArrayLayers  = 1;
	swapchainInfo.imageUsage		= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	swapchainInfo.imageSharingMode  = VK_SHARING_MODE_EXCLUSIVE;
	swapchainInfo.preTransform		= caps.currentTransform;
	swapchainInfo.compositeAlpha	= VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapchainInfo.presentMode		= m_presentMode;
	swapchainInfo.clipped			= VK_TRUE;
	swapchainInfo.oldSwapchain		= oldSwapchain;
	
	VK_CHECK(vkCreateSwapchainKHR(device, &swapchainInfo, nullptr, &m_swapchain));

	// ---- 4. Get its images (the driver may create more than minImageCount)
	VK_CHECK(vkGetSwapchainImagesKHR(device, m_swapchain, &count, nullptr));
	m_images.resize(count);
	VK_CHECK(vkGetSwapchainImagesKHR(device, m_swapchain, &count, m_images.data()));

	// ---- 5. One view per image: "a 2D color image, this format, mip 0, layer 0"
	m_views.resize(m_images.size());
	for (size_t i = 0; i < m_images.size(); ++i) {
		VkImageViewCreateInfo viewInfo{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
		viewInfo.image = m_images[i];
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = m_format;
		viewInfo.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };  // aspect, baseMip, mipCount, baseLayer, layerCount

		VK_CHECK(vkCreateImageView(device, &viewInfo, nullptr, &m_views[i]));
	}

	log::info("Swapchain {}x{}  {}  {}  {} images",
		m_extent.width, m_extent.height,
		string_VkFormat(m_format), string_VkPresentModeKHR(m_presentMode), m_images.size());
}

void Swapchain::destroyViews()
{
	if (!m_ctx)
		return;

	VkDevice device = m_ctx->device();
	for (VkImageView view : m_views)
	{
		if (view != VK_NULL_HANDLE)
			vkDestroyImageView(device, view, nullptr);
	}
	m_views.clear();
}

}