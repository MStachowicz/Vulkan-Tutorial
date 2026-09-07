#include <vulkan/vulkan_raii.hpp>

#include <GLFW/glfw3.h> // Must be included after vulkan/vulkan_raii.hpp

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>
#include <ranges>
#include <stdexcept>
#include <string>
#include <vector>

constexpr auto VulkanRequiredPhysicalDeviceAPIVersion                       = vk::ApiVersion13;
constexpr auto VulkanRequiredPhysicalDeviceQueueFamilies                    = vk::QueueFlagBits::eGraphics;
constexpr std::array<const char*, 1> VulkanRequiredPhysicalDeviceExtensions = {vk::KHRSwapchainExtensionName};
constexpr auto VulkanRequiredPhysicalDeviceFeatures                         = [](const vk::raii::PhysicalDevice& physicalDevice)
{
	auto features = physicalDevice.getFeatures2<vk::PhysicalDeviceFeatures2,
	    vk::PhysicalDeviceVulkan11Features,
	    vk::PhysicalDeviceVulkan13Features,
	    vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>();
	return features.get<vk::PhysicalDeviceVulkan11Features>().shaderDrawParameters &&
	       features.get<vk::PhysicalDeviceVulkan13Features>().dynamicRendering &&
	       features.get<vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>().extendedDynamicState;
};
#ifndef NDEBUG
constexpr bool enableVulkanValidation      = true; // Enables vulkan validation layer and associated callbacks and message printing.
constexpr bool enableVulkanMessageCallback = true; // Enables debug utils extension and messenger callback.
#else
constexpr bool enableVulkanValidation      = false;
constexpr bool enableVulkanMessageCallback = false;
#endif

class GLFWRuntime
{
public:
	GLFWRuntime()
	{
#ifdef __APPLE__
		glfwInitVulkanLoader(vkGetInstanceProcAddr);
#endif
		glfwInit();
	}

	~GLFWRuntime()
	{
		glfwTerminate();
	}

	GLFWRuntime(const GLFWRuntime&)            = delete;
	GLFWRuntime& operator=(const GLFWRuntime&) = delete;
	GLFWRuntime(GLFWRuntime&&)                 = delete;
	GLFWRuntime& operator=(GLFWRuntime&&)      = delete;
};

using Window = std::unique_ptr<GLFWwindow, void (*)(GLFWwindow*)>;

static Window getWindow(const GLFWRuntime& glfwRuntime)
{
	(void)glfwRuntime; // Silence unused parameter warning, consider adding getWindow to the GLFWRuntime.
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	constexpr uint32_t WIDTH  = 800;
	constexpr uint32_t HEIGHT = 600;
	Window window(
	    glfwCreateWindow(WIDTH, HEIGHT, "Vulkan tutorial", nullptr, nullptr),
	    glfwDestroyWindow);

	if (!window)
		throw std::runtime_error("Failed to create GLFW window");

	return window;
}

static VKAPI_ATTR vk::Bool32 VKAPI_CALL debugCallback(vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
    vk::DebugUtilsMessageTypeFlagsEXT type,
    const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData)
{
	std::cout << "Vulkan: [" << vk::to_string(severity) << "][" << vk::to_string(type) << "] " << pCallbackData->pMessage << std::endl;

	if (severity >= vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning)
		throw std::runtime_error("Vulkan error: " + std::string(pCallbackData->pMessage));

	return vk::False;
}

static vk::raii::Instance getVulkanInstance(const vk::raii::Context& context, const GLFWRuntime&)
{
	constexpr vk::ApplicationInfo appInfo{.pApplicationName = "Hello Triangle",
	    .applicationVersion                                 = VK_MAKE_VERSION(1, 0, 0),
	    .pEngineName                                        = "No Engine",
	    .engineVersion                                      = VK_MAKE_VERSION(1, 0, 0),
	    .apiVersion                                         = vk::ApiVersion14};

	std::vector<const char*> requiredInstanceExtensions;
	{ // Instance extensions

		uint32_t GLFWExtensionsCount = 0;
		auto glfwExtensions          = glfwGetRequiredInstanceExtensions(&GLFWExtensionsCount);
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
		for (const auto& requiredExtension : requiredInstanceExtensions)
		{
			if (std::ranges::none_of(extensionProperties, [requiredExtension](const auto& extensionProperty)
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

	std::vector<const char*> requiredInstanceLayers;
	{ // Instance layers
		if constexpr (enableVulkanValidation)
			requiredInstanceLayers.push_back("VK_LAYER_KHRONOS_validation");

		// Check if the required layers are supported by the Vulkan implementation.
		auto instanceLayerProperties = context.enumerateInstanceLayerProperties();
		std::string missingInstanceLayers;
		for (const auto& requiredInstanceLayer : requiredInstanceLayers)
		{
			bool missingLayer = std::ranges::none_of(instanceLayerProperties, [requiredInstanceLayer](const auto& layerProperty)
			    { return strcmp(layerProperty.layerName, requiredInstanceLayer) == 0; });
			if (missingLayer)
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
	    .flags                   = flags,
	    .pApplicationInfo        = &appInfo,
	    .enabledLayerCount       = static_cast<uint32_t>(requiredInstanceLayers.size()),
	    .ppEnabledLayerNames     = requiredInstanceLayers.data(),
	    .enabledExtensionCount   = static_cast<uint32_t>(requiredInstanceExtensions.size()),
	    .ppEnabledExtensionNames = requiredInstanceExtensions.data()};

	return vk::raii::Instance(context, createInfo);
}

static vk::raii::DebugUtilsMessengerEXT getMessenger(const vk::raii::Instance& instance)
{
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
		    .messageType                                                                       = messageTypeFlags,
		    .pfnUserCallback                                                                   = &debugCallback};

		return instance.createDebugUtilsMessengerEXT(debugUtilsMessengerCreateInfoEXT);
	}

	return nullptr;
}

static vk::raii::PhysicalDevice pickPhysicalDevice(const vk::raii::Instance& instance)
{
	auto isDeviceSuitable = [](const vk::raii::PhysicalDevice& physicalDevice) -> bool
	{
		if (physicalDevice.getProperties().apiVersion < VulkanRequiredPhysicalDeviceAPIVersion)
			return false;

		// Check if any of the queue families support graphics operations
		auto queueFamilies    = physicalDevice.getQueueFamilyProperties();
		bool supportsGraphics = std::ranges::any_of(queueFamilies, [](const auto& qfp)
		    { return !!(qfp.queueFlags & VulkanRequiredPhysicalDeviceQueueFamilies); });
		if (!supportsGraphics)
			return false;

		// Check if all required physicalDevice extensions are available
		auto availableDeviceExtensions     = physicalDevice.enumerateDeviceExtensionProperties();
		bool supportsAllRequiredExtensions = std::ranges::all_of(VulkanRequiredPhysicalDeviceExtensions, [&availableDeviceExtensions](const auto& requiredExtension)
		    { return std::ranges::any_of(availableDeviceExtensions, [&requiredExtension](const auto& availableExtension)
			      { return strcmp(availableExtension.extensionName, requiredExtension) == 0; }); });
		if (!supportsAllRequiredExtensions)
			return false;

		// Check if the physicalDevice supports the required features.
		if (!VulkanRequiredPhysicalDeviceFeatures(physicalDevice))
			return false;

		return true;
	};

	const auto physicalDevices = instance.enumeratePhysicalDevices();
	const auto devIter         = std::ranges::find_if(physicalDevices, [&](const auto& physicalDevice)
	    { return isDeviceSuitable(physicalDevice); });
	if (devIter == physicalDevices.end())
		throw std::runtime_error("Vulkan: Failed to find a suitable GPU!");

	return *devIter;
}

class HelloTriangleApplication
{
	GLFWRuntime glfwRuntime;
	Window window;
	vk::raii::Context context;
	vk::raii::Instance instance;
	vk::raii::DebugUtilsMessengerEXT debugMessenger;
	vk::raii::PhysicalDevice physicalDevice;

public:
	HelloTriangleApplication()
	    : glfwRuntime{}
	    , window{getWindow(glfwRuntime)}
	    , context{vk::raii::Context{}}
	    , instance{getVulkanInstance(context, glfwRuntime)}
	    , debugMessenger{getMessenger(instance)}
	    , physicalDevice{pickPhysicalDevice(instance)}
	{}

	void run()
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
};

int main()
{
	try
	{
		HelloTriangleApplication app;
		app.run();
	}
	catch (const std::exception& e)
	{
		std::cerr << "ERROR: " << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	std::cout << "Hello, Vulkan!" << std::endl;
	return EXIT_SUCCESS;
}