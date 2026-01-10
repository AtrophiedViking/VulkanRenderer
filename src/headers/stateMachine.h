#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan_core.h>
#include <cstdio>
#include <cstdlib>
#include <signal.h>

#define PANIC(ERROR, FORMAT,...){int macroErrorCode = ERROR; if(macroErrorCode){fprintf(stderr, "%s -> %s -> %i -> Error(%i):\n\t" FORMAT "\n", __FILE__, __func__, __LINE__, macroErrorCode, ##__VA_ARGS__); raise(SIGABRT);}};

typedef struct {
	const char* windowTitle;
	const char* engineName;
	bool windowResizable;
	uint32_t windowWidth;
	uint32_t windowHeight;
	uint32_t applicationVersion;
	uint32_t engineVersion;
	uint32_t apiVersion;
	uint32_t swapchainBuffering;
	VkAllocationCallbacks allocator;
	VkComponentMapping swapchainComponentsMapping;
	VkClearValue backgroundColor;
}Config;

typedef struct {
	uint32_t queueFamilyIndex;
	uint32_t presentFamilyIndex;

	VkInstance instance;
	VkPhysicalDevice physicalDevice;
	VkDevice device;
	VkQueue queue;
	VkQueue presentQueue;
}Context;

typedef struct {
	VkSwapchainKHR handle;

	uint32_t imageCount;
	VkImage* images;
	VkImageView *imageViews;

	VkFormat format;
	VkColorSpaceKHR colorSpace;
	VkExtent2D imageExtent;
}Swapchain;

typedef struct {
	GLFWwindow *handle;
	VkSurfaceKHR surface;
	Swapchain swapchain;
	bool framebufferResized;
}Window;

typedef struct {
	VkPipeline graphicsPipeline;
	VkPipelineLayout pipelineLayout;
	VkRenderPass renderPass;
	VkFramebuffer *framebuffers;
	VkCommandPool commandPool;
	VkCommandBuffer *commandBuffer;
	uint32_t imageAquiredIndex;
	VkSemaphore *imageAvailableSemaphore;
	VkSemaphore *renderFinishedSemaphore;
	VkFence *inFlightFence;
	uint32_t frameIndex;

}Renderer;

typedef struct {
	Config config;
	Window window;
	Context context;
	Renderer renderer;
}State;

enum SwapchainBuffering {
	SWAPCHAIN_SINGLE_BUFFERING = 1,
	SWAPCHAIN_DOUBLE_BUFFERING = 2,
	SWAPCHAIN_TRIPPLE_BUFFERING = 3,
};