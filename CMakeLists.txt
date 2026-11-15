cmake_minimum_required(VERSION 3.25)
project(VulkanShaderPlayground VERSION 0.1.0 LANGUAGES CXX)

# --- Language ---------------------------------------------------------------
set(CMAKE_CXX_STANDARD 20)            # designated initializers, std::format, concepts
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# --- Dependencies -----------------------------------------------------------
find_package(Vulkan REQUIRED COMPONENTS glslc)          # from the SDK (VULKAN_SDK)
find_package(glfw3 CONFIG REQUIRED)                     # from vcpkg
find_package(glm CONFIG REQUIRED)                       # from vcpkg
find_package(VulkanMemoryAllocator CONFIG REQUIRED)     # from vcpkg

# --- Executable -------------------------------------------------------------
# GLOB is convenient for a learning project: new files are picked up on the next
# build. (Big projects list files explicitly for reproducibility.)
file(GLOB_RECURSE VSP_SOURCES CONFIGURE_DEPENDS src/*.cpp src/*.h)
add_executable(VulkanShaderPlayground ${VSP_SOURCES} "src/core/Log.h" "src/core/Window.h" "src/core/Window.cpp" "src/app/Application.h" "src/app/Application.cpp" "src/vk/VkCheck.h" "src/vk/VulkanContext.h" "src/vk/VulkanContext.cpp")

target_include_directories(VulkanShaderPlayground PRIVATE src)

target_link_libraries(VulkanShaderPlayground PRIVATE
    Vulkan::Vulkan
    glfw
    glm::glm
    GPUOpen::VulkanMemoryAllocator)

target_compile_definitions(VulkanShaderPlayground PRIVATE
    GLM_FORCE_DEPTH_ZERO_TO_ONE    # Vulkan depth range is [0,1], not OpenGL's [-1,1] (lesson 3.1)
    GLM_FORCE_RADIANS
    NOMINMAX                       # stop <windows.h> from defining min/max macros
    WIN32_LEAN_AND_MEAN
    VSP_ASSETS_DIR="${CMAKE_SOURCE_DIR}/assets"
    VSP_SHADERS_DIR="${CMAKE_SOURCE_DIR}/shaders")

if(MSVC)
    target_compile_options(VulkanShaderPlayground PRIVATE /W4 /permissive- /utf-8)
endif()
