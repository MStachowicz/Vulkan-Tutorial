#if defined(__INTELLISENSE__) || !defined(USE_CPP20_MODULES)
#include <vulkan/vulkan_raii.hpp>
#else
import vulkan_hpp;
#endif
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <vector>
#include <string>
#include <ranges>

#ifndef NDEBUG
// When integrating into an engine, it could be worth splitting this so message callbacks work for non-validation layer callbacks.
// e.g. DebugUtilsMessageTypeFlagBitsEXT::eGeneral | ePerformance
constexpr bool enableVulkanValidation = true; // Enables vulkan validation layer and associated callbacks and message printing.
constexpr bool enableVulkanMessageCallback = true; // Enables debug utils extension and messenger callback.
#else
constexpr bool enableVulkanValidation = false;
constexpr bool enableVulkanMessageCallback = false;
#endif

class HelloTriangleApplication
{
	std::unique_ptr<GLFWwindow, void (*)(GLFWwindow *)> window;
	vk::raii::Context context;
	vk::raii::Instance instance;
	vk::raii::DebugUtilsMessengerEXT debugMessenger;

public:
	HelloTriangleApplication()
		: window{nullptr, glfwDestroyWindow}, context{}, instance{nullptr}, debugMessenger{nullptr}
	{
	}

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
			glfwCreateWindow(WIDTH, HEIGHT, "Vulkan tutorial", nullptr, nullptr),
			glfwDestroyWindow);

		if (!window)
			throw std::runtime_error("Failed to create GLFW window");
	}

	static VKAPI_ATTR vk::Bool32 VKAPI_CALL debugCallback(vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
														  vk::DebugUtilsMessageTypeFlagsEXT type,
														  const vk::DebugUtilsMessengerCallbackDataEXT *pCallbackData,
														  void *pUserData)
	{
		std::cout << "Vulkan: [" << vk::to_string(severity) << "][" << vk::to_string(type) << "] " << pCallbackData->pMessage << std::endl;

		if (severity >= vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning)
			throw std::runtime_error("Vulkan error: " + std::string(pCallbackData->pMessage));

		return vk::False;
	}
	void initVulkan()
	{
		constexpr vk::ApplicationInfo appInfo{.pApplicationName = "Hello Triangle",
											  .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
											  .pEngineName = "No Engine",
											  .engineVersion = VK_MAKE_VERSION(1, 0, 0),
											  .apiVersion = vk::ApiVersion14};

		std::vector<const char *> requiredInstanceExtensions;
		{ // Instance extensions

			uint32_t GLFWExtensionsCount = 0;
			auto glfwExtensions = glfwGetRequiredInstanceExtensions(&GLFWExtensionsCount);
			requiredInstanceExtensions.insert(requiredInstanceExtensions.end(), glfwExtensions, glfwExtensions + GLFWExtensionsCount);

			if constexpr (enableVulkanMessageCallback) // Add the debug utils extension for validation layers message callbacks.
				requiredInstanceExtensions.push_back(vk::EXTDebugUtilsExtensionName);
#ifdef __APPLE__
			// Add the KHR portability enumeration extension for compatibility with MoltenVK on macOS.
			requiredInstanceExtensions.push_back(vk::KHRPortabilityEnumerationExtensionName);
#endif

			// Check if the required extensions are supported by the Vulkan implementation.
			auto extensionProperties = context.enumerateInstanceExtensionProperties();
			std::string missingInstanceExtensions;
			for (const auto &requiredExtension : requiredInstanceExtensions)
			{
				if (std::ranges::none_of(extensionProperties, [requiredExtension](auto const &extensionProperty)
										 { return strcmp(extensionProperty.extensionName, requiredExtension) == 0; }))
				{
					if (missingInstanceExtensions.empty())
						missingInstanceExtensions += requiredExtension;
					else
						missingInstanceExtensions += '\n' + std::string(requiredExtension);
				}
			}
			if (!missingInstanceExtensions.empty())
				throw std::runtime_error("ERROR: Required Vulkan instance extensions not supported: " + missingInstanceExtensions);
		}

		std::vector<const char *> requiredInstanceLayers;
		{ // Instance layers
			if constexpr (enableVulkanValidation)
				requiredInstanceLayers.push_back("VK_LAYER_KHRONOS_validation");

			// Check if the required layers are supported by the Vulkan implementation.
			auto instanceLayerProperties = context.enumerateInstanceLayerProperties();
			std::string missingInstanceLayers;
			for (const auto &requiredInstanceLayer : requiredInstanceLayers)
			{
				if (std::ranges::none_of(instanceLayerProperties, [requiredInstanceLayer](auto const &layerProperty)
										 { return strcmp(layerProperty.layerName, requiredInstanceLayer) == 0; }))
				{
					if (missingInstanceLayers.empty())
						missingInstanceLayers += requiredInstanceLayer;
					else
						missingInstanceLayers += '\n' + std::string(requiredInstanceLayer);
				}
			}
			if (!missingInstanceLayers.empty())
				throw std::runtime_error("ERROR: Required Vulkan instance validation layers not supported: " + missingInstanceLayers);
		}

		vk::InstanceCreateFlags flags = {};
#ifdef __APPLE__
		flags |= vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR;
#endif

		vk::InstanceCreateInfo createInfo{
			.flags = flags,
			.pApplicationInfo = &appInfo,
			.enabledLayerCount = static_cast<uint32_t>(requiredInstanceLayers.size()),
			.ppEnabledLayerNames = requiredInstanceLayers.data(),
			.enabledExtensionCount = static_cast<uint32_t>(requiredInstanceExtensions.size()),
			.ppEnabledExtensionNames = requiredInstanceExtensions.data()};

		instance = vk::raii::Instance(context, createInfo);

		if constexpr (enableVulkanMessageCallback)
		{
			vk::DebugUtilsMessageSeverityFlagsEXT severityFlags(vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
																vk::DebugUtilsMessageSeverityFlagBitsEXT::eError |
																vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
																vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo);

			vk::DebugUtilsMessageTypeFlagsEXT messageTypeFlags(vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
															   vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
															   vk::DebugUtilsMessageTypeFlagBitsEXT::eDeviceAddressBinding);

			if constexpr (enableVulkanValidation)
				messageTypeFlags |= vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation;

			vk::DebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfoEXT{.messageSeverity = severityFlags,
																				  .messageType = messageTypeFlags,
																				  .pfnUserCallback = &debugCallback};

			debugMessenger = instance.createDebugUtilsMessengerEXT(debugUtilsMessengerCreateInfoEXT);
		}
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
		window.reset(); // Destroy the window before glfwTerminate
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