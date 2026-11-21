#include "renderer/Renderer.h"

#include "vk/VulkanContext.h"
#include "vk/VkCheck.h"
#include "vk/VkUtils.h"
#include "vk/ShaderModule.h"
#include "vk/PipelineBuilder.h"
#include "core/Window.h"
#include "core/log.h"

#include <cmath>

void vsp::Renderer::init(VulkanContext& ctx, Window& window)
{
	m_ctx = &ctx;
	m_window = &window;

	const auto fb = window.framebufferExtent();
	m_swapchain.init(ctx, { fb.width, fb.height }, m_vsync);
	createFrames();
	createPipelines();
}

void vsp::Renderer::shutdown()
{
	if (m_ctx == nullptr) return;

	vkDeviceWaitIdle(m_ctx->device());   // never destroy things the GPU is still using
	destroyPipelines();
	destroyFrames();
	m_swapchain.shutdown();
	m_ctx = nullptr;
}

void vsp::Renderer::createFrames()
{
	VkDevice device = m_ctx->device();

	for (FrameData& data : m_frames)
	{
		VkCommandPoolCreateInfo poolInfo{ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
		poolInfo.queueFamilyIndex = m_ctx->graphicsQueueFamily();
		VK_CHECK(vkCreateCommandPool(device, &poolInfo, nullptr, &data.commandPool));

		VkCommandBufferAllocateInfo allocInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
		allocInfo.commandPool = data.commandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = 1;
		VK_CHECK(vkAllocateCommandBuffers(device, &allocInfo, &data.commandBuffer));

		VkSemaphoreCreateInfo semInfo{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
		VK_CHECK(vkCreateSemaphore(device, &semInfo, nullptr, &data.imageAvailable));

		VkFenceCreateInfo fenceInfo{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;   // so the very first wait doesn't block forever
		VK_CHECK(vkCreateFence(device, &fenceInfo, nullptr, &data.inFlight));
	}
}

void vsp::Renderer::destroyFrames()
{
	VkDevice device = m_ctx->device();

	for (FrameData& f : m_frames) {
		vkDestroyFence(device, f.inFlight, nullptr);
		vkDestroySemaphore(device, f.imageAvailable, nullptr);
		vkDestroyCommandPool(device, f.commandPool, nullptr);   // also frees its command buffer
		f = FrameData{};
	}
}

void vsp::Renderer::recreateSwapchain()
{
	while (m_window->isMinimized())   // a 0x0 swapchain is invalid: sleep until restored
		m_window->waitEvents();

	const auto fb = m_window->framebufferExtent();
	m_swapchain.recreate({ fb.width, fb.height }, m_vsync);   // waits idle internally
}

// ======================================================================== the frame


void vsp::Renderer::drawFrame(double timeSeconds)
{
	if (m_window->isMinimized()) {    // nothing to draw into; sleep instead of spinning
		m_window->waitEvents();
		return;
	}

	VkDevice   device = m_ctx->device();
	FrameData& frame = m_frames[m_frameIndex];

	// 1. Wait until the GPU has finished the last frame that used this slot
	VK_CHECK(vkWaitForFences(device, 1, &frame.inFlight, VK_TRUE, UINT64_MAX));

	uint32_t imageIndex = 0;
	const VkResult acquire = vkAcquireNextImageKHR(device, m_swapchain.handle(), UINT64_MAX, frame.imageAvailable, VK_NULL_HANDLE, &imageIndex);

	if (acquire == VK_ERROR_OUT_OF_DATE_KHR) {		// window changed: rebuild, skip this frame
		recreateSwapchain();
		return;
	}
	if (acquire != VK_SUBOPTIMAL_KHR)				// SUBOPTIMAL: still usable, carry on
		VK_CHECK(acquire);

	// 3. Only now that we will definitely submit: reset fence and command pool
	VK_CHECK(vkResetFences(device, 1, &frame.inFlight));
	VK_CHECK(vkResetCommandPool(device, frame.commandPool, 0));

	// 4. Record this frame's commands
	recordCommands(frame.commandBuffer, imageIndex, timeSeconds);

	// 5. Submit: wait for the image to be free, signal when rendering is done
	VkSemaphoreSubmitInfo waitInfo{ VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO };
	waitInfo.semaphore = frame.imageAvailable;
	waitInfo.stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;

	VkSemaphoreSubmitInfo signalInfo{ VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO };
	signalInfo.semaphore = m_swapchain.renderFinished(imageIndex);
	signalInfo.stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;

//	acquire                 GPU work(your notepad)                 present
//		─────────►[WAIT]  ───────────────────────────────►[SIGNAL]  ─────────►
//		imageAvailable                             renderFinished
//		"is the board free yet?"                   "I'm done painting"
	VkCommandBufferSubmitInfo cmdInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO };
	cmdInfo.commandBuffer = frame.commandBuffer;
	
	VkSubmitInfo2 submit{ VK_STRUCTURE_TYPE_SUBMIT_INFO_2 };
	submit.waitSemaphoreInfoCount = 1;
	submit.pWaitSemaphoreInfos = &waitInfo;
	submit.commandBufferInfoCount = 1;
	submit.pCommandBufferInfos = &cmdInfo;
	submit.signalSemaphoreInfoCount = 1;
	submit.pSignalSemaphoreInfos = &signalInfo;

	VK_CHECK(vkQueueSubmit2(m_ctx->graphicsQueue(), 1, &submit, frame.inFlight));
	
	// 6. Present: hand the image back once rendering has finished
	VkSemaphore renderFinished = m_swapchain.renderFinished(imageIndex);
	VkSwapchainKHR swapchian = m_swapchain.handle();


	VkPresentInfoKHR  presentInfo{ VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &renderFinished;
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &swapchian;
	presentInfo.pImageIndices = &imageIndex;

	const VkResult present = vkQueuePresentKHR(m_ctx->graphicsQueue(), &presentInfo);

	// 7. Window changed? Rebuild the swapchain now (after presenting, never mid-frame)
	const bool resized = m_window->consumeResizedFlag();
	if (present == VK_ERROR_OUT_OF_DATE_KHR || present == VK_SUBOPTIMAL_KHR || resized)
		recreateSwapchain();
	else
		VK_CHECK(present);

	// 8. Next slot
	m_frameIndex = (m_frameIndex + 1) % kMaxFramesInFlight;
}

void vsp::Renderer::recordCommands(VkCommandBuffer cmd, uint32_t imageIndex, double timeSeconds)
{
	VkCommandBufferBeginInfo beginInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;   // re-recorded every frame
	VK_CHECK(vkBeginCommandBuffer(cmd, &beginInfo));

	const VkImage image = m_swapchain.image(imageIndex);

	// Barrier 1: "whatever was in it" -> ready to be drawn into
	vkutil::transitionImage(cmd, image,
		VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_NONE,
		VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT);

	// Animated clear color: three sine waves 120 degrees apart, kept dim
	const float t = static_cast<float>(timeSeconds);
	const float r = 0.3f * (0.5f + 0.5f * std::sin(t));
	const float g = 0.3f * (0.5f + 0.5f * std::sin(t + 2.094f));
	const float b = 0.3f * (0.5f + 0.5f * std::sin(t + 4.189f));

	// Dynamic rendering: the image view goes straight in. No VkRenderPass, no VkFramebuffer.
	VkRenderingAttachmentInfo color{ VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
	color.imageView = m_swapchain.view(imageIndex);
	color.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	color.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	color.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	color.clearValue.color = { { r, g, b, 1.0f } };

	VkRenderingInfo renderingInfo{ VK_STRUCTURE_TYPE_RENDERING_INFO };
	renderingInfo.renderArea = { { 0, 0 }, m_swapchain.extent() };
	renderingInfo.layerCount = 1;
	renderingInfo.colorAttachmentCount = 1;
	renderingInfo.pColorAttachments = &color;

	vkCmdBeginRendering(cmd, &renderingInfo); //----------------------------------------------------------------------------------------
	// Phase 2: draw calls go here

	// Which pipeline (shaders + fixed settings) to draw with
	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_trianglePipeline);

	// Dynamic state: cover the whole swapchain image (changes on resize, so set every frame)
	const VkExtent2D extent = m_swapchain.extent();
	VkViewport viewport{};
	viewport.width = static_cast<float>(extent.width);
	viewport.height = static_cast<float>(extent.height);
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(cmd, 0, 1, &viewport);

	VkRect2D scissor{ { 0, 0 }, extent };
	vkCmdSetScissor(cmd, 0, 1, &scissor);

	// 3 vertices, 1 instance. The vertex shader runs 3 times with gl_VertexIndex = 0, 1, 2.
	vkCmdDraw(cmd, 3, 1, 0, 0);


	vkCmdEndRendering(cmd);  //----------------------------------------------------------------------------------------

	// Barrier 2: drawn -> ready to be shown
	vkutil::transitionImage(cmd, image,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
		VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
		VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_NONE);

	VK_CHECK(vkEndCommandBuffer(cmd));
}

void vsp::Renderer::createPipelines()
{
	VkDevice device = m_ctx->device();

	// Pipeline layout = the list of resources the shaders can access. Empty for now:
	// push constants and descriptor sets
	VkPipelineLayoutCreateInfo pipelineLayoutInfo{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
	VK_CHECK(vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &m_pipelineLayout));

	VkShaderModule vert = vkutil::loadShaderModule(device, vkutil::shaderPath("triangle.vert"));
	VkShaderModule frag = vkutil::loadShaderModule(device, vkutil::shaderPath("triangle.frag"));

	// a Builder pattern with a fluent interface (method chaining), used on a temporary object.
	m_trianglePipeline = PipelineBuilder{}
		.shaders(vert, frag)
		.colorFormat(m_swapchain.format())
		.build(device, m_pipelineLayout);

	// The pipeline has its own compiled copy of the code: the modules can go now
	vkDestroyShaderModule(device, frag, nullptr);
	vkDestroyShaderModule(device, vert, nullptr);
}

void vsp::Renderer::destroyPipelines()
{
	VkDevice device = m_ctx->device();
	vkDestroyPipeline(device, m_trianglePipeline, nullptr);
	vkDestroyPipelineLayout(device, m_pipelineLayout, nullptr);
	m_trianglePipeline = VK_NULL_HANDLE;
	m_pipelineLayout = VK_NULL_HANDLE;
}
