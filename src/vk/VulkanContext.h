#pragma once

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
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

	VkInstance       instance()            const { return m_instance; }
	VkSurfaceKHR     surface()             const { return m_surface; }
	VkPhysicalDevice physicalDevice()      const { return m_physicalDevice; }
	VkDevice         device()              const { return m_device; }
	VkQueue          graphicsQueue()       const { return m_graphicsQueue; }
	uint32_t         graphicsQueueFamily() const { return m_graphicsFamily; }
	VmaAllocator     allocator()           const { return m_allocator; }

private:
	void createInstance(const VulkanContextDesc& desc);
	void createDebugMessenger();
	void createSurface(Window& window);
	void pickPhysicalDevice();
	void createDevice();
	void createAllocator();

	VkInstance               m_instance =		VK_NULL_HANDLE;
	VkDebugUtilsMessengerEXT m_debugMessenger = VK_NULL_HANDLE;
	bool                     m_validation =		false;

	VkSurfaceKHR			 m_surface =		VK_NULL_HANDLE;
	VkPhysicalDevice		 m_physicalDevice = VK_NULL_HANDLE;
	VkDevice				 m_device =			VK_NULL_HANDLE;
	VkQueue					 m_graphicsQueue =  VK_NULL_HANDLE;
	uint32_t         m_graphicsFamily = UINT32_MAX;
	VmaAllocator     m_allocator = VK_NULL_HANDLE;
	VkPhysicalDeviceProperties m_gpuProperties{};   // limits are handy later
};

}