#pragma once

#include <vulkan/vulkan.h>

namespace vsp::vkutil {
	// One image layout transition + memory dependency (synchronization2).
	// Read it as: "wait for <srcStage> work doing <srcAccess>, change the layout,
	//              then let <dstStage> work doing <dstAccess> start."
	void transitionImage(VkCommandBuffer cmd, VkImage image,
		VkImageLayout oldLayout, VkImageLayout newLayout,
		VkPipelineStageFlags2 srcStage, VkAccessFlags2 srcAccess,
		VkPipelineStageFlags2 dstStage, VkAccessFlags2 dstAccess,
		VkImageAspectFlags aspect = VK_IMAGE_ASPECT_COLOR_BIT);

}