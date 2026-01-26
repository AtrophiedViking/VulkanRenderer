#include "headers/window.h"
//Error Handlin
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

//IO CallBacks
void mouseCallback(GLFWwindow* window, double xpos, double ypos) {

	auto state = static_cast<State*>(glfwGetWindowUserPointer(window));
	// State persistence for calculating movement deltas
	// Static variables maintain state between callback invocations
	static bool firstMouse = true;          // Flag to handle initial mouse position
	static float lastX = 0.0f, lastY = 0.0f;  // Previous mouse position for delta calculation

	// Handle initial mouse position to prevent sudden camera jumps
	// First callback provides absolute position, not relative movement
	if (firstMouse) {
		lastX = (float)xpos;               // Initialize previous position
		lastY = (float)ypos;
		firstMouse = false;         // Disable special handling for subsequent calls
	}

	// Calculate mouse movement deltas since last callback
	// These deltas represent the amount and direction of mouse movement
	float xoffset = (float)xpos - lastX;                   // Horizontal movement (left-right)
	float yoffset = lastY - (float)ypos;                   // Vertical movement (inverted: screen Y increases downward, camera pitch increases upward)

	// Update state for next callback iteration
	lastX = (float)xpos;
	lastY = (float)ypos;

	// Convert mouse movement to camera rotation
	// Delta values drive continuous camera orientation changes
	state->scene.camera.processMouseMovement(xoffset, yoffset, false);
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
static void framebufferResizeCallback(GLFWwindow *window, int height, int width) {
	auto app = (State*)glfwGetWindowUserPointer(window);
    app->window.framebufferResized = true;
}

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
	glfwSetFramebufferSizeCallback(state->window.handle, framebufferResizeCallback);
	instanceCreate(state);
	surfaceCreate(state);
	deviceCreate(state);
	swapchainCreate(state);
	swapchainImageGet(state);
	imageViewsCreate(state);
	renderPassCreate(state);
	descriptorSetLayoutCreate(state);
	graphicsPipelineCreate(state);
	commandPoolCreate(state);

	colorResourceCreate(state);
	depthResourceCreate(state);
	frameBuffersCreate(state);
	glfwSetWindowUserPointer(state->window.handle, state);
	glfwSetCursorPosCallback(state->window.handle, mouseCallback);

	textureImageCreate(state, state->config.KOBOLD_TEXTURE_PATH);
	textureImageViewCreate(state);
	textureSamplerCreate(state);

	modelLoad(state, state->config.KOBOLD_MODEL_PATH);
	gameObjectsCreate(state);

	vertexBufferCreate(state);
	indexBufferCreate(state);
	uniformBuffersCreate(state);
	descriptorPoolCreate(state);
	descriptorSetsCreate(state);
	commandBufferGet(state);
	commandBufferRecord(state);
	syncObjectsCreate(state);
};
void windowDestroy(State* state) {
	swapchainCleanup(state);
	textureSamplerDestroy(state);
	textureImageViewDestroy(state);
	textureImageDestroy(state);
	uniformBuffersDestroy(state);
	descriptorPoolDestroy(state);
	descriptorSetLayoutDestroy(state);
	indexBufferDestroy(state);
	vertexBufferDestroy(state);
	syncObjectsDestroy(state);
	commandPoolDestroy(state);
	graphicsPipelineDestroy(state);
	renderPassDestroy(state);
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

	for (uint32_t i = 0; i < state->window.swapchain.imageCount; i++) {
		state->window.swapchain.imageViews[i] = imageViewCreate(state, state->window.swapchain.images[i], state->window.swapchain.format, VK_IMAGE_ASPECT_COLOR_BIT, 1);
	}
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

};
void swapchainDestroy(State* state) {
	vkDestroySwapchainKHR(state->context.device, state->window.swapchain.handle, nullptr);
};

void swapchainCleanup(State* state) {
	colorResourceDestroy(state);
	depthBufferDestroy(state),
	frameBuffersDestroy(state);
	imageViewsDestroy(state);
	swapchainDestroy(state);
};
void swapchainRecreate(State* state) {
	int width = 0, height = 0;
	glfwGetFramebufferSize(state->window.handle, &width, &height);
	while (width == 0 || height == 0) {
		glfwGetFramebufferSize(state->window.handle, &width, &height);
		glfwWaitEvents();
	}

	vkDeviceWaitIdle(state->context.device);
	swapchainCleanup(state);
	swapchainCreate(state);
	swapchainImageGet(state);
	imageViewsCreate(state);
	colorResourceCreate(state);
	depthResourceCreate(state);
	frameBuffersCreate(state);
};

//Draw
void frameDraw(State* state) {
	vkWaitForFences(state->context.device, 1, &state->renderer.inFlightFence[state->renderer.frameIndex], VK_TRUE, UINT64_MAX);
	VkResult result = vkAcquireNextImageKHR(state->context.device, state->window.swapchain.handle, UINT64_MAX, state->renderer.imageAvailableSemaphore[state->renderer.frameIndex], VK_NULL_HANDLE, &state->renderer.imageAquiredIndex);
	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		swapchainRecreate(state);
		return;
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
		throw std::runtime_error("failed to present swap chain image!");
	}
	vkResetFences(state->context.device, 1, &state->renderer.inFlightFence[state->renderer.frameIndex]);
	vkResetCommandBuffer(state->buffers.commandBuffer[state->renderer.frameIndex],/*VkCommandBufferResetFlagBits*/0);
	commandBufferRecord(state);


	VkSemaphore waitSemaphores[] = { state->renderer.imageAvailableSemaphore[state->renderer.frameIndex] };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	VkSemaphore signalSemaphores[] = { state->renderer.renderFinishedSemaphore[state->renderer.frameIndex] };
	VkSwapchainKHR swapChains[] = { state->window.swapchain.handle };

	VkSubmitInfo submitInfo{
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = waitSemaphores,
		.pWaitDstStageMask = waitStages,
		.commandBufferCount = 1,
		.pCommandBuffers = &state->buffers.commandBuffer[state->renderer.frameIndex],
		.signalSemaphoreCount = 1,
		.pSignalSemaphores = signalSemaphores,
	};

	PANIC(vkQueueSubmit(state->context.queue, 1, &submitInfo, state->renderer.inFlightFence[state->renderer.frameIndex]), "Failed To Submit Queue");
	VkPresentInfoKHR presentInfo{
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,

		.waitSemaphoreCount = 1,
		.pWaitSemaphores = signalSemaphores,
		.swapchainCount = 1,
		.pSwapchains = swapChains,
		.pImageIndices = &state->renderer.imageAquiredIndex,
		.pResults = nullptr, // Optional
	};
	result = vkQueuePresentKHR(state->context.presentQueue, &presentInfo);

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || state->window.framebufferResized) {
		state->window.framebufferResized = false;
		swapchainRecreate(state);
	}
	else if (result != VK_SUCCESS) {
		throw std::runtime_error("failed to present swap chain image!");
	}
	state->renderer.frameIndex = (state->renderer.frameIndex + 1) % state->config.swapchainBuffering;
};

void processInput(State* state) {
	// Calculate delta time
	static float lastFrame = 0.0f;
	float currentFrame = (float)glfwGetTime();
	float deltaTime = currentFrame - lastFrame;
	lastFrame = currentFrame;

	// Process keyboard input for camera movement
	if (glfwGetKey(state->window.handle, GLFW_KEY_W) == GLFW_PRESS)
		state->scene.camera.processKeyboard(CameraMovement::FORWARD, deltaTime);
	if (glfwGetKey(state->window.handle, GLFW_KEY_S) == GLFW_PRESS)
		state->scene.camera.processKeyboard(CameraMovement::BACKWARD, deltaTime);
	if (glfwGetKey(state->window.handle, GLFW_KEY_A) == GLFW_PRESS)
		state->scene.camera.processKeyboard(CameraMovement::LEFT, deltaTime);
	if (glfwGetKey(state->window.handle, GLFW_KEY_D) == GLFW_PRESS)
		state->scene.camera.processKeyboard(CameraMovement::RIGHT, deltaTime);
	if (glfwGetKey(state->window.handle, GLFW_KEY_SPACE) == GLFW_PRESS)
		state->scene.camera.processKeyboard(CameraMovement::UP, deltaTime);
	if (glfwGetKey(state->window.handle, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
		state->scene.camera.processKeyboard(CameraMovement::DOWN, deltaTime);


	//Toggle Mouse
	static bool previous = false;
	bool current = glfwGetKey(state->window.handle, GLFW_KEY_ESCAPE) == GLFW_PRESS;
	if (current && !previous) {
		state->scene.camera.lookMode = !state->scene.camera.lookMode;
		glfwSetInputMode(
			state->window.handle,
			GLFW_CURSOR,
			state->scene.camera.lookMode ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL
		);
	}
	previous = current;

};