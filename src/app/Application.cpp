#include "Application.h"

#include <format>

#include "../core/log.h"

vsp::Application::Application()
	: m_window(WindowDesc{})
{
	log::info("Application started");
}

vsp::Application::~Application()
{
	log::info("Application shutting down");
}

void vsp::Application::run()
{
	using clock = std::chrono::steady_clock;
	auto last = clock::now();

	while (!m_window.shouldClose()) {
		m_window.pollEvents();

		if (m_window.consumeResizedFlag())
		{
			auto e = m_window.framebufferExtent();
			vsp::log::info("Resized to {}x{}", e.width, e.height);
		}

		// ---- Delta Time ---
		auto now = clock::now();
		double dt = std::chrono::duration(now - last).count();
		last = now;
		m_time += dt;
		updateTitle(dt);
	}
}

void vsp::Application::updateTitle(double dt)
{
	m_titleTimer += dt;
	++m_frameSinceTitle;

	if (m_titleTimer >= 0.5)
	{
		double fps = m_frameSinceTitle / m_titleTimer;
		double ms = 1000.0 / fps;
		m_window.setTitle(std::format("Vulkan Shader Playground | {:.0f} FPS | {:.2f} ms", fps, ms));
		m_titleTimer = 0.0;
		m_frameSinceTitle = 0;
	}
}
