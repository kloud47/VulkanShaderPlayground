#include "vk/PipelineBuilder.h"
#include "vk/VkCheck.h"

#include <stdexcept>

namespace vsp {
	PipelineBuilder& PipelineBuilder::shaders(VkShaderModule vert, VkShaderModule frag)
	{
		m_vert = vert;
		m_frag = frag;
		return *this;
	}

	PipelineBuilder& PipelineBuilder::vertexInput(std::span<const VkVertexInputBindingDescription> bindings, std::span<const VkVertexInputAttributeDescription> attributes)
	{
		m_bindings.assign(bindings.begin(), bindings.end());
		m_attributes.assign(attributes.begin(), attributes.end());
		return *this;
	}

	PipelineBuilder& PipelineBuilder::topology(VkPrimitiveTopology topology) { m_topology = topology;  return *this; }
	PipelineBuilder& PipelineBuilder::polygonMode(VkPolygonMode mode) { m_polygonMode = mode;   return *this; }
	PipelineBuilder& PipelineBuilder::colorFormat(VkFormat format) { m_colorFormat = format; return *this; }

	PipelineBuilder& PipelineBuilder::cullMode(VkCullModeFlags cull, VkFrontFace frontFace)
	{
		m_cullMode = cull;
		m_frontFace = frontFace;
		return *this;
	}

	VkPipeline PipelineBuilder::build(VkDevice device, VkPipelineLayout layout) const
	{
		if (m_vert == VK_NULL_HANDLE || m_frag == VK_NULL_HANDLE || m_colorFormat == VK_FORMAT_UNDEFINED)
			throw std::runtime_error("PipelineBuilder: shaders and colorFormat are required");

		// 1. Programmable stages: which shader runs where. "main" = the GLSL entry point.
		VkPipelineShaderStageCreateInfo stages[2]{};
		stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
		stages[0].module = m_vert;
		stages[0].pName = "main";
		stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		stages[1].module = m_frag;
		stages[1].pName = "main";

		// 2. Vertex input: how vertex buffer bytes map to the shader's `layout(location = N) in` (empty = none)
		VkPipelineVertexInputStateCreateInfo vertexInput{ VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
		vertexInput.vertexBindingDescriptionCount = static_cast<uint32_t>(m_bindings.size());
		vertexInput.pVertexBindingDescriptions = m_bindings.data();
		vertexInput.vertexAttributeDescriptionCount = static_cast<uint32_t>(m_attributes.size());
		vertexInput.pVertexAttributeDescriptions = m_attributes.data();

		// 3. Input assembly: how vertices are grouped (every 3 = one triangle)
		// 3. Input assembly: how vertices are grouped (every 3 = one triangle)
		VkPipelineInputAssemblyStateCreateInfo inputAssembly{ VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
		inputAssembly.topology = m_topology;
		inputAssembly.primitiveRestartEnable = VK_FALSE;		// If you are using TRIANGLE_STRIP or LINE_STRIP, you could turn this on:

		// 4. Viewport/scissor: only the COUNT here; the actual rectangles are set per frame (dynamic state)
		VkPipelineViewportStateCreateInfo viewport{ VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
		viewport.viewportCount = 1;
		viewport.scissorCount = 1;

		// 5. Rasterizer: triangles -> pixels (fill or wireframe, which faces to skip)
		VkPipelineRasterizationStateCreateInfo raster{ VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
		raster.polygonMode = m_polygonMode;
		raster.cullMode = m_cullMode;
		raster.frontFace = m_frontFace;
		raster.lineWidth = 1.0f;

		// 6. Multisampling: off (1 sample per pixel)
		VkPipelineMultisampleStateCreateInfo multisample{ VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
		multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

		// 7. Depth/stencil: off for now (lesson 3.3)
		VkPipelineDepthStencilStateCreateInfo depthStencil{ VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };

		// 8. Color blending: no blending, write all four channels
		VkPipelineColorBlendAttachmentState blendAttachment{};
		blendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
										 VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

		VkPipelineColorBlendStateCreateInfo blend{ VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
		blend.attachmentCount = 1;
		blend.pAttachments = &blendAttachment;

		// 9. Dynamic state: set with vkCmdSetViewport/Scissor while recording, so a resize needs no new pipeline
		// 9. Dynamic state: set with vkCmdSetViewport/Scissor while recording, so a resize needs no new pipeline
		const VkDynamicState dynamicStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
		VkPipelineDynamicStateCreateInfo dynamic{ VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
		dynamic.dynamicStateCount = 2;
		dynamic.pDynamicStates = dynamicStates;

		// 10. Dynamic rendering: tell the pipeline the FORMAT of the image it will draw into
		VkPipelineRenderingCreateInfo rendering{ VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO };
		rendering.colorAttachmentCount = 1;
		rendering.pColorAttachmentFormats = &m_colorFormat;

		VkGraphicsPipelineCreateInfo info{ VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
		info.pNext = &rendering;          // no VkRenderPass: this replaces it
		info.stageCount = 2;
		info.pStages = stages;
		info.pVertexInputState = &vertexInput;
		info.pInputAssemblyState = &inputAssembly;
		info.pViewportState = &viewport;
		info.pRasterizationState = &raster;
		info.pMultisampleState = &multisample;
		info.pDepthStencilState = &depthStencil;
		info.pColorBlendState = &blend;
		info.pDynamicState = &dynamic;
		info.layout = layout;

		VkPipeline pipeline = VK_NULL_HANDLE;
		VK_CHECK(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &info, nullptr, &pipeline));
		return pipeline;
	}
}