#pragma once

#include <vulkan/vulkan.h>

#include <cstdint>
#include <string>
#include <vector>

namespace vsp::vkutil {

	// Read a compiled .spv file. SPIR-V is a stream of 32-bit words.
	std::vector<uint32_t> readSpirv(const std::string& path);

	// Load a .spv file and wrap it in a VkShaderModule.
	// The module is only needed while building a pipeline: destroy it right after.
	VkShaderModule loadShaderModule(VkDevice device, const std::string& path);

	// Full path of a compiled shader, e.g. shaderPath("triangle.vert") -> ".../shaders/triangle.vert.spv"
	std::string shaderPath(const std::string& name);
}
