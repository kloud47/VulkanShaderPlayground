#pragma once

#include "../core/Window.h"
#include "../vk/VulkanContext.h"
#include "../vk/Renderer.h"

#include <chrono>

namespace vsp {

	class Application {
	public:
		Application();
		~Application();

		Application(const Application&) = delete;
		Application& operator=(const Application&) = delete;

		void run();      // The main loop:

	private:
		void updateTitle(double dt);

		Window m_window;
		VulkanContext m_vk;
		Renderer      m_renderer;

		double m_time			 = 0.0;
		double m_titleTimer		 = 0.0;
		double m_frameSinceTitle = 0.0;
	};
}