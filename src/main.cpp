#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <vulkan/vk_enum_string_helper.h>

#include <format>
#include <iostream>

#include "core/Window.h"
#include "app/Application.h"
#include  "core/Log.h"

int main()
{
    uint32_t version = 0;
    vkEnumerateInstanceVersion(&version);
    std::cout << std::format("Vulkan loader {}.{}.{}\n",
        VK_API_VERSION_MAJOR(version), VK_API_VERSION_MINOR(version), VK_API_VERSION_PATCH(version));

    glm::vec3 v{1.0f, 2.0f, 3.0f};
    std::cout << std::format("glm length {:.3f}\n", glm::length(v));

    std::cout << std::format("result string {}\n", string_VkResult(VK_SUCCESS));

    if (!glfwInit())
        return 1;
    std::cout << std::format("GLFW {} vulkanSupported={}\n", glfwGetVersionString(), glfwVulkanSupported());
    glfwTerminate();

    std::cout << VSP_ASSETS_DIR << '\n';

    try {
        vsp::Application app; // Window is constructed here:
        app.run();
    }
    catch (const std::exception& e) {
        vsp::log::error("Fatal: {}", e.what());
        return 1;
    }
    return 0;
}