#pragma once

#include "vk/SwapChain.h"

#include <vulkan/vulkan.h>

#include <array>
#include <cassert>
#include <cstdint>

namespace vsp {

	class VulkanContext;
	class Window;

	constexpr uint32_t kMaxFramesInFlight = 2;

	// Everything the CPU touches per frame needs one copy per frame in flight.
	struct FrameData {
		VkCommandPool   commandPool = VK_NULL_HANDLE;
		VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
		VkSemaphore     imageAvailable = VK_NULL_HANDLE;   // acquire -> render  (GPU waits)
		VkFence         inFlight = VK_NULL_HANDLE;   // GPU done -> CPU    (CPU waits)
	};

	class Renderer {
	public:
		Renderer() = default;
		~Renderer() { assert(m_ctx == nullptr && "Renderer::shutdown() not called"); }

		Renderer(const Renderer&) = delete;
		Renderer& operator=(const Renderer&) = delete;

		void init(VulkanContext& ctx, Window& window);
		void shutdown();
		void drawFrame(double timeSeconds);

	private:
		void createFrames();
		void destroyFrames();
		void recreateSwapchain();
		void recordCommands(VkCommandBuffer cmd, uint32_t imageIndex, double timeSeconds);

		void createPipelines();
		void destroyPipelines();


		VulkanContext*			m_ctx		= nullptr;
		Window*					m_window	= nullptr;
		Swapchain				m_swapchain;
		std::array<FrameData, kMaxFramesInFlight> m_frames{};
		uint32_t				m_frameIndex = 0;
		bool					m_vsync = true;
		VkPipelineLayout		m_pipelineLayout = VK_NULL_HANDLE;   // what resources shaders get (nothing yet)
		VkPipeline				m_trianglePipeline = VK_NULL_HANDLE;
	};

}