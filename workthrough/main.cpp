#if defined(__INTELLISENSE__) || !defined(USE_CPP20_MODULES)
#include <vulkan/vulkan_raii.hpp>
#else
import vulkan_hpp;
#endif
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib>

class HelloTriangleApplication
{
public:
	HelloTriangleApplication() : window(nullptr, glfwDestroyWindow) {}

	void run()
	{
		initWindow();
		initVulkan();
		mainLoop();
		cleanup();
	}

private:
	std::unique_ptr<GLFWwindow, void (*)(GLFWwindow *)> window;

	void initWindow()
	{
#ifdef __APPLE__
		glfwInitVulkanLoader(vkGetInstanceProcAddr);
#endif
		glfwInit();
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

		constexpr uint32_t WIDTH = 800;
		constexpr uint32_t HEIGHT = 600;
		window = std::unique_ptr<GLFWwindow, void (*)(GLFWwindow *)>(
			glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr),
			glfwDestroyWindow);

		if (!window)
			throw std::runtime_error("Failed to create GLFW window");
	}

	void initVulkan()
	{
	}

	void mainLoop()
	{
		while (!glfwWindowShouldClose(window.get()))
		{
			glfwPollEvents();
		}
	}

	void cleanup()
	{
		// glfwDestroyWindow(window.release()); removed because window is now managed by unique_ptr
		glfwTerminate();
	}
};

int main()
{
	try
	{
		HelloTriangleApplication app;
		app.run();
	}
	catch (const std::exception &e)
	{
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	std::cout << "Hello, Vulkan!" << std::endl;

	return EXIT_SUCCESS;
}