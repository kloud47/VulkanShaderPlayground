#include "vk/ShaderModule.h"
#include "vk/VkCheck.h"
#include "core/Log.h"

#include <fstream>
#include <stdexcept>

namespace vsp::vkutil {

	std::vector<uint32_t> readSpirv(const std::string& path)
	{
		std::ifstream file(path, std::ios::binary | std::ios::ate);   // ate: start at the end to get the size
		if (!file)
			throw std::runtime_error("Shader not found: " + path + " (did the build run glslc?)");

		const std::streamsize bytes = file.tellg();
		if (bytes <= 0 || bytes % 4 != 0)
			throw std::runtime_error("Not a valid SPIR-V file: " + path);

		std::vector<uint32_t> words(static_cast<size_t>(bytes) / 4);
		file.seekg(0);
		file.read(reinterpret_cast<char*>(words.data()), bytes);
		return words;
	}

	VkShaderModule loadShaderModule(VkDevice device, const std::string& path)
	{
		const std::vector<uint32_t> code = readSpirv(path);

		VkShaderModuleCreateInfo info{ VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
		info.codeSize = code.size() * sizeof(uint32_t);   // in BYTES, not words
		info.pCode = code.data();

		VkShaderModule module = VK_NULL_HANDLE;
		VK_CHECK(vkCreateShaderModule(device, &info, nullptr, &module));
		log::info("Loaded shader {} ({} bytes)", path, info.codeSize);
		return module;
	}

	std::string shaderPath(const std::string& name)
	{
		return std::string(VSP_SHADER_BIN_DIR) + "/" + name + ".spv";
	}

}
