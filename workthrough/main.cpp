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
	std::unique_ptr<GLFWwindow, void (*)(GLFWwindow *)> window;
	vk::raii::Context context;
	vk::raii::Instance instance;

public:
	HelloTriangleApplication()
		: window{nullptr, glfwDestroyWindow}, context{}, instance{nullptr}
	{}

	void run()
	{
		initWindow();
		initVulkan();
		mainLoop();
		cleanup();
	}

private:
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
		constexpr vk::ApplicationInfo appInfo{.pApplicationName = "Hello Triangle",
											  .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
											  .pEngineName = "No Engine",
											  .engineVersion = VK_MAKE_VERSION(1, 0, 0),
											  .apiVersion = vk::ApiVersion14};

		// Get the required instance extensions from GLFW.
		uint32_t GLFWExtensionsCount = 0;
		auto glfwExtensions = glfwGetRequiredInstanceExtensions(&GLFWExtensionsCount);

		// Add the KHR portability enumeration extension to the list of required extensions.
		std::vector<const char *> requiredExtensions(glfwExtensions, glfwExtensions + GLFWExtensionsCount);
#ifdef __APPLE__
		requiredExtensions.push_back(vk::KHRPortabilityEnumerationExtensionName);
#endif

		// Check if the required extensions are supported by the Vulkan implementation.
		auto extensionProperties = context.enumerateInstanceExtensionProperties();
		for (uint32_t i = 0; i < requiredExtensions.size(); ++i)
		{
			if (std::ranges::none_of(extensionProperties,
									 [requiredExtension = requiredExtensions[i]](auto const &extensionProperty)
									 { return strcmp(extensionProperty.extensionName, requiredExtension) == 0; }))
			{
				throw std::runtime_error("ERROR: Required GLFW extension not supported: " + std::string(requiredExtensions[i]));
			}
		}

		vk::InstanceCreateFlags flags = {};
#ifdef __APPLE__
		flags |= vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR;
#endif

		vk::InstanceCreateInfo createInfo{
			.pApplicationInfo = &appInfo,
			.flags = flags,
			.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size()),
			.ppEnabledExtensionNames = requiredExtensions.data()};

		instance = vk::raii::Instance(context, createInfo);
	}

	void mainLoop()
	{
		while (!glfwWindowShouldClose(window.get()))
		{
			glfwPollEvents();
			if (glfwGetKey(window.get(), GLFW_KEY_ESCAPE) == GLFW_PRESS)
			{
				glfwSetWindowShouldClose(window.get(), GLFW_TRUE);
			}
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
		std::cerr << "ERROR: " << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	std::cout << "Hello, Vulkan!" << std::endl;

	return EXIT_SUCCESS;
}