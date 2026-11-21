#include "Window.h"
#include "core/Log.h"

#include <stdexcept>

vsp::Window::Window(const WindowDesc& desc)
{
	glfwSetErrorCallback([](int code, const char* description) {
		log::error("GLFW error {}: {}", code, description);
		});

	if (!glfwInit())
	{
		throw std::runtime_error("glfwInit failed");
	}

	if (!glfwVulkanSupported())
	{
		glfwTerminate();
		throw std::runtime_error("glfw Vulkan not supported.");
	}

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // no OpenGL, since we use Vulkan
	glfwWindowHint(GLFW_RESIZABLE, desc.resizable ? GLFW_TRUE : GLFW_FALSE);

	m_window = glfwCreateWindow(desc.width, desc.height, desc.title.c_str(), nullptr, nullptr);
	if (m_window == nullptr) {
		glfwTerminate();
		throw std::runtime_error("glfwCreateWindow failed");
	}

	glfwSetWindowUserPointer(m_window, this);

	glfwSetFramebufferSizeCallback(m_window, [](GLFWwindow* w, int, int) {
		auto* self = static_cast<Window*>(glfwGetWindowUserPointer(w));
		self->m_resized = true;
	});

	glfwSetKeyCallback(m_window, [](GLFWwindow* window, int key, int /*scancode*/, int action, int /*mods*/) {
		if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
			glfwSetWindowShouldClose(window, GLFW_TRUE);
	});
}

vsp::Window::~Window()
{
	if (m_window) glfwDestroyWindow(m_window);
	glfwTerminate();
}

bool vsp::Window::shouldClose() const
{
	return glfwWindowShouldClose(m_window) != 0;
}

void vsp::Window::pollEvents()
{
	glfwPollEvents();
}

void vsp::Window::waitEvents()
{
	glfwWaitEvents();
}

vsp::Window::Extent vsp::Window::framebufferExtent() const
{
	int w = 0, h = 0;
	glfwGetFramebufferSize(m_window, &w, &h);
	return {
		static_cast<uint32_t>(w), static_cast<uint32_t>(h)
	};
}

bool vsp::Window::isMinimized() const
{
	const auto e = framebufferExtent();
	return e.width == 0 || e.height == 0;
}

bool vsp::Window::consumeResizedFlag()
{
	bool was = m_resized;
	m_resized = false;
	return was;
}

void vsp::Window::setTitle(const std::string& title)
{
	glfwSetWindowTitle(m_window, title.c_str());
}