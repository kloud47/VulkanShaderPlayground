#pragma once

#include <vulkan/vulkan.h>
#include <cassert>

namespace vsp {

class Window;

#ifndef NDEBUG
inline constexpr bool kEnableValidation = true;
#else
inline constexpr bool kEnableValidation = false;
#endif

struct VulkanContextDesc {
	const char* appName = "Vulkan Shader Playground";
	bool enableValidation = kEnableValidation;
};

class VulkanContext {
public:
	VulkanContext() = default;
	~VulkanContext() { assert(m_instance == VK_NULL_HANDLE && "VulkanContext::shutdown() not called"); }

	VulkanContext(const VulkanContext&) = delete;
	VulkanContext& operator=(const VulkanContext&) = delete;

	void init(const VulkanContextDesc& desc, Window& window);
	void shutdown();

	VkInstance instance() const { return m_instance; }
	// 1.2 adds: physicalDevice(), device(), graphicsQueue(), graphicsQueueFamily(), allocator(), surface()

private:
	void createInstance(const VulkanContextDesc& desc);
	void createDebugMessenger();

	VkInstance               m_instance = VK_NULL_HANDLE;
	VkDebugUtilsMessengerEXT m_debugMessenger = VK_NULL_HANDLE;
	bool                     m_validation = false;
};

}