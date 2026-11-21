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
add_executable(VulkanShaderPlayground ${VSP_SOURCES} "src/vk/ShaderModule.h" "src/vk/ShaderModule.cpp" "src/vk/PipelineBuilder.h" "src/vk/PipelineBuilder.cpp")

target_include_directories(VulkanShaderPlayground PRIVATE src)

target_link_libraries(VulkanShaderPlayground PRIVATE
    Vulkan::Vulkan
    glfw
    glm::glm
    GPUOpen::VulkanMemoryAllocator)

# --- Shaders: GLSL -> SPIR-V at build time (lesson 2.1) ----------------------
# Every .vert/.frag/.comp under shaders/ becomes <build>/shaders/<name>.spv,
# e.g. shaders/triangle.vert -> out/build/x64-debug/shaders/triangle.vert.spv
file(GLOB_RECURSE VSP_SHADER_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_SOURCE_DIR}/shaders/*.vert
    ${CMAKE_SOURCE_DIR}/shaders/*.frag
    ${CMAKE_SOURCE_DIR}/shaders/*.comp)

set(VSP_SHADER_BIN_DIR ${CMAKE_BINARY_DIR}/shaders)
set(VSP_SPIRV_FILES "")

foreach(SHADER ${VSP_SHADER_SOURCES})
    get_filename_component(SHADER_NAME ${SHADER} NAME)         # triangle.vert
    set(SPIRV ${VSP_SHADER_BIN_DIR}/${SHADER_NAME}.spv)         # .../triangle.vert.spv
    add_custom_command(
        OUTPUT  ${SPIRV}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${VSP_SHADER_BIN_DIR}
        COMMAND ${Vulkan_GLSLC_EXECUTABLE} --target-env=vulkan1.3 -g -o ${SPIRV} ${SHADER}
        DEPENDS ${SHADER}
        COMMENT "glslc ${SHADER_NAME}"
        VERBATIM)
    list(APPEND VSP_SPIRV_FILES ${SPIRV})
endforeach()

add_custom_target(VspShaders DEPENDS ${VSP_SPIRV_FILES} SOURCES ${VSP_SHADER_SOURCES} "src/vk/ShaderModule.cpp" "src/vk/PipelineBuilder.h" "src/vk/PipelineBuilder.cpp")
add_dependencies(VulkanShaderPlayground VspShaders)   # shaders are (re)compiled before the exe

target_compile_definitions(VulkanShaderPlayground PRIVATE
    GLM_FORCE_DEPTH_ZERO_TO_ONE    # Vulkan depth range is [0,1], not OpenGL's [-1,1] (lesson 3.1)
    GLM_FORCE_RADIANS
    NOMINMAX                       # stop <windows.h> from defining min/max macros
    WIN32_LEAN_AND_MEAN
    VSP_ASSETS_DIR="${CMAKE_SOURCE_DIR}/assets"
    VSP_SHADERS_DIR="${CMAKE_SOURCE_DIR}/shaders"
    VSP_SHADER_BIN_DIR="${VSP_SHADER_BIN_DIR}")   # where the compiled .spv files are

if(MSVC)
    target_compile_options(VulkanShaderPlayground PRIVATE /W4 /permissive- /utf-8)
endif()
