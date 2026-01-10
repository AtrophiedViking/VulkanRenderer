#include "headers/window.h"
//Error Handling
void logPrint(State* state) {
	uint32_t instanceApiVersion;
	PANIC(vkEnumerateInstanceVersion(&instanceApiVersion), "Failed To Enumerate Instance Version");
	uint32_t apiVersionVarient = VK_API_VERSION_VARIANT(instanceApiVersion);
	uint32_t apiVersionMajor = VK_API_VERSION_MAJOR(instanceApiVersion);
	uint32_t apiVersionMinor = VK_API_VERSION_MINOR(instanceApiVersion);
	uint32_t apiVersionPatch = VK_API_VERSION_PATCH(instanceApiVersion);
	printf("Vulkan API %i.%i.%i.%i\n", apiVersionVarient, apiVersionMajor, apiVersionMinor, apiVersionPatch);
	printf("GLFW %s\n", glfwGetVersionString());
};
void exitCallback() {
	glfwTerminate();
};
void glfwErrorCallback(int errorCode, const char* description) {
	PANIC(errorCode, "GLFW %s", description);
};
void errorHandlingSetup(State* state) {
	glfwSetErrorCallback(glfwErrorCallback);
	atexit(exitCallback);
};

//utility
static uint32_t clamp(uint32_t value, uint32_t min, uint32_t max){
	if (value < min) {
		return min;
	}
	else if (value > max) {
		return max;
	};
	return value;
};

//Window
void initGLFW(State *state) {
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, state->config.windowResizable);
};

void surfaceCreate(State* state) {
	PANIC(glfwCreateWindowSurface(state->context.instance, state->window.handle, nullptr, &state->window.surface), "Failed To Create Surface");
};
void surfaceDestroy(State* state) {
	vkDestroySurfaceKHR(state->context.instance, state->window.surface, nullptr);
};

void windowCreate(State* state) {
	initGLFW(state);
	state->window.handle = glfwCreateWindow(state->config.windowWidth, state->config.windowHeight, state->config.windowTitle, nullptr, nullptr);
	instanceCreate(state);
	surfaceCreate(state);
	deviceCreate(state);
	swapchainCreate(state);
	renderPassCreate(state);
	graphicsPipelineCreate(state);
	frameBuffersCreate(state);
	commandPoolCreate(state);
	commandBufferGet(state);
	commandBufferRecord(state);
	syncObjectsCreate(state);
};
void windowDestroy(State* state) {
	syncObjectsDestroy(state);
	frameBuffersDestroy(state);
	commandPoolDestroy(state);
	graphicsPipelineDestroy(state);
	renderPassDestroy(state);
	swapchainDestroy(state);
	deviceDestroy(state);
	surfaceDestroy(state);
	instanceDestroy(state);
	glfwDestroyWindow(state->window.handle);
	glfwTerminate();
};

//Swapchain
VkSurfaceCapabilitiesKHR surfaceCapabilitiesGet(State* state) {
	VkSurfaceCapabilitiesKHR capabilities;
	PANIC(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(state->context.physicalDevice, state->window.surface, &capabilities), "Failed To Get Surface Cababilities");
	return capabilities;
};
VkSurfaceFormatKHR surfaceFormatSelect(State* state) {
	uint32_t formatCount;
	PANIC(vkGetPhysicalDeviceSurfaceFormatsKHR(state->context.physicalDevice, state->window.surface, &formatCount, nullptr), "Failed To Get Format Count");
	VkSurfaceFormatKHR* formats = (VkSurfaceFormatKHR*)malloc(formatCount * sizeof(VkSurfaceFormatKHR));
	PANIC(!formats, "Failed To Allocate Formats Memory");
	PANIC(vkGetPhysicalDeviceSurfaceFormatsKHR(state->context.physicalDevice, state->window.surface, &formatCount, formats), "Failed To Get Format Count");

	uint32_t formatIndex = 0;
	for (int i = 0; i < (int)formatCount; i++) {
		VkSurfaceFormatKHR format = formats[i];
		if (format.colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR && format.format == VK_FORMAT_B8G8R8A8_SRGB) {
			formatIndex = i;
			break;
		};
	};
	VkSurfaceFormatKHR format = formats[formatIndex];
	free(formats);
	return format;
};
VkPresentModeKHR presentModeSelect(State* state) {
	VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
	uint32_t presentModeCount;
	PANIC(vkGetPhysicalDeviceSurfacePresentModesKHR(state->context.physicalDevice, state->window.surface, &presentModeCount, nullptr), "Failed To Get Present Mode Count");
	VkPresentModeKHR *presentModes = (VkPresentModeKHR*)malloc(presentModeCount * sizeof(VkPresentModeKHR));
	PANIC(!presentModes, "Failed To Allocate Present Mode Memory");
	PANIC(vkGetPhysicalDeviceSurfacePresentModesKHR(state->context.physicalDevice, state->window.surface, &presentModeCount, presentModes), "Failed To Get Surface Present Modes");

	uint32_t presentModeIndex = 0;
	for (int i = 0; i < (int)presentModeCount; i++) {
		VkPresentModeKHR mode = presentModes[i];
		if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
			presentModeIndex = i;
			break;
		};
		if (presentModeIndex != UINT32_MAX) {
			presentMode = presentModes[presentModeIndex];
		};
	};
	free(presentModes);
	return presentMode;
};

void swapchainImageGet(State* state) {
	PANIC(vkGetSwapchainImagesKHR(state->context.device, state->window.swapchain.handle, &state->window.swapchain.imageCount, nullptr), "Failed to Get Swapchain Image Count");
	state->window.swapchain.images = (VkImage*)malloc(state->window.swapchain.imageCount * sizeof(VkImage));
	PANIC(!state->window.swapchain.images, "Failed To Enumerate Swapchain Image Memory");
	PANIC(vkGetSwapchainImagesKHR(state->context.device, state->window.swapchain.handle, &state->window.swapchain.imageCount, state->window.swapchain.images), "Failed To Get Swapchain Images");
};
void imageViewsCreate(State* state) {
	state->window.swapchain.imageViews = (VkImageView *)malloc(state->window.swapchain.imageCount * sizeof(VkImageView));
	PANIC(!state->window.swapchain.imageViews, "Failed To Count ImageViews");
	
	
	for (int i = 0; i < (int)state->window.swapchain.imageCount; i++) {
		VkImageSubresourceRange imageSubResourceRange{
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.levelCount = 1,
			.layerCount = 1,
		};
		VkImageViewCreateInfo imageViewInfo{
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.image = state->window.swapchain.images[i],
			.viewType = VK_IMAGE_VIEW_TYPE_2D,
			.format = state->window.swapchain.format,
			.components = state->config.swapchainComponentsMapping,
			.subresourceRange = imageSubResourceRange,
		};
		PANIC(vkCreateImageView(state->context.device, &imageViewInfo, nullptr, &state->window.swapchain.imageViews[i]), "Failed To Create Image View %i", i);
	};
};
void imageViewsDestroy(State* state) {
	for (int i = 0; i < (int)state->window.swapchain.imageCount; i++) {
		vkDestroyImageView(state->context.device, state->window.swapchain.imageViews[i], nullptr);
	};
};

void swapchainCreate(State* state) {
	VkSurfaceCapabilitiesKHR capabilities = surfaceCapabilitiesGet(state);
	VkSurfaceFormatKHR surfaceFormat = surfaceFormatSelect(state);
	VkPresentModeKHR presentMode = presentModeSelect(state);

	state->window.swapchain.format = surfaceFormat.format;
	state->window.swapchain.colorSpace = surfaceFormat.colorSpace;
	state->window.swapchain.imageExtent = capabilities.currentExtent;

	VkSwapchainCreateInfoKHR swapchainInfo{
		.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		.surface = state->window.surface,
		.minImageCount = clamp(state->config.swapchainBuffering, capabilities.minImageCount, capabilities.maxImageCount ? capabilities.maxImageCount : UINT32_MAX),
		.imageFormat = state->window.swapchain.format,
		.imageColorSpace = state->window.swapchain.colorSpace,
		.imageExtent = state->window.swapchain.imageExtent,
		.imageArrayLayers = 1,
		.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		.queueFamilyIndexCount = 1,
		.pQueueFamilyIndices = &state->context.queueFamilyIndex,
		.preTransform = capabilities.currentTransform,
		.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		.presentMode = presentMode,
		.clipped = true,
	};

	PANIC(vkCreateSwapchainKHR(state->context.device, &swapchainInfo, nullptr, &state->window.swapchain.handle),"Failed To Create Swapchain");

	swapchainImageGet(state);
	imageViewsCreate(state);
};
void swapchainDestroy(State* state) {
	imageViewsDestroy(state);
	vkDestroySwapchainKHR(state->context.device, state->window.swapchain.handle, nullptr);
};

void swapchainCleanup(State* state) {
	frameBuffersDestroy(state);
	swapchainDestroy(state);
};
void swapchainReCreate(State* state) {
	vkDeviceWaitIdle(state->context.device);
	swapchainCleanup(state);
	swapchainCreate(state);
	frameBuffersCreate(state);
};