#include "vk/VulkanContext.h"
#include "vk/VkCheck.h"
#include "../core/Window.h"
#include "../core/log.h"

#include <GLFW/glfw3.h>
#include <vector>

namespace vsp {

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT, VkDebugUtilsMessageTypeFlagsEXT,
        const VkDebugUtilsMessengerCallbackDataEXT* data, void*)
    {
        log::error("[Vulkan] {}", data->pMessage);
        return VK_FALSE;
    }

    void VulkanContext::init(const VulkanContextDesc& desc, Window&)
    {
        m_validation = desc.enableValidation;
        createInstance(desc);
        createDebugMessenger();
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
        if (m_debugMessenger) {
            auto destroy = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
                vkGetInstanceProcAddr(m_instance, "vkDestroyDebugUtilsMessengerEXT"));
            destroy(m_instance, m_debugMessenger, nullptr);
            m_debugMessenger = VK_NULL_HANDLE;
        }
        vkDestroyInstance(m_instance, nullptr);
        m_instance = VK_NULL_HANDLE;
    }

} 