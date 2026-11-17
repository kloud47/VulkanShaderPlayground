#include "vk/VulkanContext.h"
#include "vk/VkCheck.h"
#include "../core/Window.h"
#include "../core/log.h"

#include <GLFW/glfw3.h>
#include <stdexcept>
#include <vector>

namespace vsp {

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT, VkDebugUtilsMessageTypeFlagsEXT,
        const VkDebugUtilsMessengerCallbackDataEXT* data, void*)
    {
        log::error("[Vulkan] {}", data->pMessage);
        return VK_FALSE;
    }

    static uint32_t findGraphicsPresentFamily(VkPhysicalDevice gpu, VkSurfaceKHR surface)
    {
        uint32_t count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(gpu, &count, nullptr);
        std::vector<VkQueueFamilyProperties> families(count);
        vkGetPhysicalDeviceQueueFamilyProperties(gpu, &count, families.data());
        for (uint32_t i = 0; i < count; ++i) {
            VkBool32 canPresent = VK_FALSE;
            vkGetPhysicalDeviceSurfaceSupportKHR(gpu, i, surface, &canPresent);

            if ((families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) && canPresent)
                return i;
        }
        return UINT32_MAX;
    }

    void VulkanContext::init(const VulkanContextDesc& desc, Window& window)
    {
        m_validation = desc.enableValidation;
        createInstance(desc);
        createDebugMessenger();
        createSurface(window);
        pickPhysicalDevice();
        createDevice();
        createAllocator();
    }

    void VulkanContext::createInstance(const VulkanContextDesc& desc)
    {
        uint32_t count = 0;
        const char** glfwExts = glfwGetRequiredInstanceExtensions(&count);
        std::vector<const char*> extensions(glfwExts, glfwExts + count);

        std::vector<const char*> layers;
        if (m_validation) {
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
            layers.push_back("VK_LAYER_KHRONOS_validation");
        }

        VkApplicationInfo appInfo{ VK_STRUCTURE_TYPE_APPLICATION_INFO };
        appInfo.pApplicationName = desc.appName;
        appInfo.apiVersion = VK_API_VERSION_1_3;

        VkInstanceCreateInfo info{ VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
        info.pApplicationInfo = &appInfo;
        info.enabledLayerCount = static_cast<uint32_t>(layers.size());
        info.ppEnabledLayerNames = layers.data();
        info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        info.ppEnabledExtensionNames = extensions.data();

        VK_CHECK(vkCreateInstance(&info, nullptr, &m_instance));
        log::info("VkInstance created (validation {})", m_validation ? "ON" : "OFF");
    }

    void VulkanContext::createDebugMessenger()
    {
        if (!m_validation) return;

        VkDebugUtilsMessengerCreateInfoEXT info{ VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT };
        info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        info.pfnUserCallback = debugCallback;

        auto create = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
            vkGetInstanceProcAddr(m_instance, "vkCreateDebugUtilsMessengerEXT"));
        VK_CHECK(create(m_instance, &info, nullptr, &m_debugMessenger));
    }

    void VulkanContext::shutdown()
    {
        if (m_allocator) {
            vmaDestroyAllocator(m_allocator);
            m_allocator = VK_NULL_HANDLE;
        }
        if (m_device) {
            vkDestroyDevice(m_device, nullptr);   // the queue is owned by the device; no separate destroy
            m_device = VK_NULL_HANDLE;
            m_graphicsQueue = VK_NULL_HANDLE;
        }
        if (m_surface)
        {
            vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
            m_surface = VK_NULL_HANDLE;
        }
        if (m_debugMessenger) {
            auto destroy = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
                vkGetInstanceProcAddr(m_instance, "vkDestroyDebugUtilsMessengerEXT"));
            destroy(m_instance, m_debugMessenger, nullptr);
            m_debugMessenger = VK_NULL_HANDLE;
        }
        vkDestroyInstance(m_instance, nullptr);
        m_instance = VK_NULL_HANDLE;
    }

    void VulkanContext::createSurface(Window& window)
    {
        VK_CHECK(glfwCreateWindowSurface(m_instance, window.handle(), nullptr, &m_surface));
    }

    void VulkanContext::pickPhysicalDevice()
    {
        uint32_t physicalDeviceCount = 0;
        VK_CHECK(vkEnumeratePhysicalDevices(m_instance, &physicalDeviceCount, nullptr));

        if (physicalDeviceCount == 0) {
            throw std::runtime_error("No Vulkan physical devices found");
        }

        std::vector<VkPhysicalDevice> gpus(physicalDeviceCount);
        VK_CHECK(vkEnumeratePhysicalDevices(m_instance, &physicalDeviceCount, gpus.data()));
        
        log::info("GPU found: {}", physicalDeviceCount);

        for (VkPhysicalDevice gpu : gpus)
        {
            VkPhysicalDeviceProperties props;
            vkGetPhysicalDeviceProperties(gpu, &props);

            const uint32_t family = findGraphicsPresentFamily(gpu, m_surface);
            const bool     usable = props.apiVersion >= VK_API_VERSION_1_3 && family != UINT32_MAX;
            const bool     discrete = props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;  // prefer discrete; integrated is the fallback

            log::info("{} | {} | Vulkan {}.{} {}", props.deviceName, discrete ? "discrete": "integrated/other", VK_API_VERSION_MAJOR(props.apiVersion), VK_API_VERSION_MINOR(props.apiVersion),
                usable ? "" : "(not usable)");

            if (!usable) continue;

            if (m_physicalDevice == VK_NULL_HANDLE || discrete)
            {
                m_physicalDevice = gpu;
                m_graphicsFamily = family;
                m_gpuProperties = props;
            }
            if (discrete) break;
        }

        if (m_physicalDevice == VK_NULL_HANDLE)
            throw std::runtime_error("No GPU supports Vulkan 1.3 with graphics + present");

        log::info("Selected GPU: {} (queue family {})", m_gpuProperties.deviceName, m_graphicsFamily);
    }

    void VulkanContext::createDevice()
    {
        const float priority = 1.0f;
        VkDeviceQueueCreateInfo queueInfo{};
        queueInfo.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueInfo.queueFamilyIndex = m_graphicsFamily;
        queueInfo.queueCount       = 1;
        queueInfo.pQueuePriorities = &priority;
        
        // 2. Feature chain:  features2 ──pNext──► vulkan13features
        VkPhysicalDeviceVulkan13Features f13{};
        f13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
        f13.dynamicRendering = VK_TRUE;
        f13.synchronization2 = VK_TRUE;

        VkPhysicalDeviceFeatures2 f2{};
        f2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        f2.pNext = &f13;
        f2.features.samplerAnisotropy = VK_TRUE;    // sharper textures at angles (6.2)
        f2.features.fillModeNonSolid = VK_TRUE;     // wireframe mode (4.2)

        // Device Extensions
        const char* extensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

        VkDeviceCreateInfo logicalDevice{};
        logicalDevice.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        logicalDevice.pNext = &f2;                          // features go through pNext...
        logicalDevice.queueCreateInfoCount = 1;
        logicalDevice.pQueueCreateInfos = &queueInfo;
        logicalDevice.enabledExtensionCount = 1;
        logicalDevice.ppEnabledExtensionNames = extensions;
        // ...so pEnabledFeatures must stay nullptr (the {} already set it)

        VK_CHECK(vkCreateDevice(m_physicalDevice, &logicalDevice, nullptr, &m_device));

        // Get the queue: family m_graphicsFamily, queue index 0
        vkGetDeviceQueue(m_device, m_graphicsFamily, 0, &m_graphicsQueue);

        log::info("VkDevice created, features: dynamicRendering, synchronization2, samplerAnisotropy, fillModeNonSolid");
    }

    void VulkanContext::createAllocator()
    {
        VmaAllocatorCreateInfo info{};
        info.physicalDevice = m_physicalDevice;
        info.device = m_device;
        info.instance = m_instance;
        info.vulkanApiVersion = VK_API_VERSION_1_3;

        VK_CHECK(vmaCreateAllocator(&info, &m_allocator));
        log::info("VMA allocator created");
    }

} 