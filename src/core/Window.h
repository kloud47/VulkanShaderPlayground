#pragma once

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#include <cstdint>
#include <string>

namespace vsp {

struct WindowDesc {
	std::string title = "Vulkan Shader Playground";
	int			width = 1600;
	int			height = 900;
	bool		resizable = true;
};

class Window {
public:
	explicit Window(const WindowDesc& desc = {});
	~Window();
	Window(const Window&) = delete;				// a window can't be copied
	Window& operator=(const Window&) = delete;

	bool shouldClose() const;
	void pollEvents();
	void waitEvents();

	struct Extent { uint32_t width, height; };
	Extent framebufferExtent() const;
	bool isMinimized() const;

	bool consumeResizedFlag();
	void setTitle(const std::string& title);

	GLFWwindow* handle() const { return m_window; }

private:
	GLFWwindow* m_window = nullptr;
	bool		m_resized = false;
};

}