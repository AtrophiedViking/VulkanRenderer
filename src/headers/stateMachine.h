#pragma once
#define GLM_FORCE_RADIANS
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan_core.h>
#include <cstdio>
#include <cstdlib>
#include <signal.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <chrono>
#include <vector>
#include <array>

#define PANIC(ERROR, FORMAT,...){int macroErrorCode = ERROR; if(macroErrorCode){fprintf(stderr, "%s -> %s -> %i -> Error(%i):\n\t" FORMAT "\n", __FILE__, __func__, __LINE__, macroErrorCode, ##__VA_ARGS__); raise(SIGABRT);}};

struct Vertex {
	glm::vec2 pos;
	glm::vec3 color;

	static VkVertexInputBindingDescription getBindingDescription() {
		VkVertexInputBindingDescription bindingDescription{
			.binding = 0,
			.stride = sizeof(Vertex),
			.inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
		};
		return bindingDescription;
	}

	static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions() {
		std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};

		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(Vertex, pos);

		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(Vertex, color);

		return attributeDescriptions;
	}
};
struct UniformBufferObject {
	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 proj;
};
const std::vector<Vertex> vertices = {

	{{ -0.5f, -0.5f }, {0.8f, 0.3f, 0.05f}},
	{{0.5f, -0.5f}, {0.8f, 0.3f, 0.05f}},
	{{0.5f, 0.5f}, {0.8f, 0.3f, 0.05f}},
	{{-0.5f, 0.5f}, {0.8f, 0.3f, 0.05f}},
};
const std::vector<uint16_t> indices = {
	0, 1, 2, 2, 3, 0
};
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
	VkDescriptorSetLayout descriptorSetLayout;
	VkPipelineLayout pipelineLayout;
	VkRenderPass renderPass;
	VkCommandPool commandPool;
	VkFramebuffer *framebuffers;
	VkBuffer vertexBuffer;
	VkDeviceMemory vertexBufferMemory;
	VkBuffer indexBuffer;
	VkDeviceMemory indexBufferMemory;
	VkBuffer *uniformBuffers;
	VkDeviceMemory *uniformBuffersMemory;
	std::vector<void*> uniformBuffersMapped;
	VkDescriptorPool descriptorPool;
	VkDescriptorSet *descriptorSets;
	VkCommandBuffer *commandBuffer;
	uint32_t imageAquiredIndex;
	VkSemaphore *imageAvailableSemaphore;
	VkSemaphore *renderFinishedSemaphore;
	VkFence *inFlightFence;
	uint32_t frameIndex;
	uint32_t memType;
	VkMemoryRequirements memRequirements;
	VkPhysicalDeviceMemoryProperties memProperties;
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