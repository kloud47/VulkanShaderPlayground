#pragma once

#include <vulkan/vulkan.h>

#include <span>
#include <vector>

namespace vsp {

	// Collects the settings of a graphics pipeline, then creates it in one go.
	// Every setter returns *this, so calls can be chained:
	//
	//   VkPipeline p = PipelineBuilder{}
	//       .shaders(vert, frag)
	//       .colorFormat(swapchain.format())
	//       .build(device, layout);
	//
	// Defaults: triangle list, filled polygons, no culling, no blending, no depth.
	class PipelineBuilder {
	public:
		PipelineBuilder& shaders(VkShaderModule vert, VkShaderModule frag);
		PipelineBuilder& vertexInput(std::span<const VkVertexInputBindingDescription>   bindings,
			std::span<const VkVertexInputAttributeDescription> attributes);
		PipelineBuilder& topology(VkPrimitiveTopology topology);
		PipelineBuilder& polygonMode(VkPolygonMode mode);
		PipelineBuilder& cullMode(VkCullModeFlags cull, VkFrontFace frontFace);
		PipelineBuilder& colorFormat(VkFormat format);

		VkPipeline build(VkDevice device, VkPipelineLayout layout) const;

	private:
		VkShaderModule      m_vert = VK_NULL_HANDLE;
		VkShaderModule      m_frag = VK_NULL_HANDLE;
		std::vector<VkVertexInputBindingDescription>   m_bindings;
		std::vector<VkVertexInputAttributeDescription> m_attributes;
		VkPrimitiveTopology m_topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		VkPolygonMode       m_polygonMode = VK_POLYGON_MODE_FILL;
		VkCullModeFlags     m_cullMode = VK_CULL_MODE_NONE;
		VkFrontFace         m_frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		VkFormat            m_colorFormat = VK_FORMAT_UNDEFINED;
	};

}
