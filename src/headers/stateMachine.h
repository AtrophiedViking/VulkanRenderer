#pragma once
#define GLFW_INCLUDE_VULKAN
#define STB_IMAGE_IMPLEMENTATION
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <GLFW/glfw3.h>
#include <vulkan/vulkan_core.h>
#include <cstdio>
#include <cstdlib>
#include <signal.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>
#include <unordered_map>
#include <chrono>
#include <vector>
#include <array>


#define PANIC(ERROR, FORMAT,...){int macroErrorCode = ERROR; if(macroErrorCode){fprintf(stderr, "%s -> %s -> %i -> Error(%i):\n\t" FORMAT "\n", __FILE__, __func__, __LINE__, macroErrorCode, ##__VA_ARGS__); raise(SIGABRT);}};

struct Vertex {
	glm::vec3 pos;
	glm::vec3 color;
	glm::vec2 texCoord;

	static VkVertexInputBindingDescription getBindingDescription() {
		VkVertexInputBindingDescription bindingDescription{};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(Vertex);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return bindingDescription;
	}

	static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions() {
		std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};

		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(Vertex, pos);

		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(Vertex, color);

		attributeDescriptions[2].binding = 0;
		attributeDescriptions[2].location = 2;
		attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[2].offset = offsetof(Vertex, texCoord);

		return attributeDescriptions;

	}
	bool operator==(const Vertex& other) const {
		return pos == other.pos && color == other.color && texCoord == other.texCoord;
	}
};
namespace std {
	template<> struct hash<Vertex> {
		size_t operator()(Vertex const& vertex) const {
			return ((hash<glm::vec3>()(vertex.pos) ^
				(hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^
				(hash<glm::vec2>()(vertex.texCoord) << 1);
		}
	};
}

struct UniformBufferObject {
	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 proj;
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
	const std::string TEXTURE_PATH;
	const std::string MODEL_PATH;
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

}Gui;

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
	VkCommandBuffer* commandBuffer;
	VkFramebuffer* framebuffers;
	VkBuffer vertexBuffer;
	VkDeviceMemory vertexBufferMemory;
	VkBuffer indexBuffer;
	VkDeviceMemory indexBufferMemory;
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	VkBuffer* uniformBuffers;
	VkDeviceMemory* uniformBuffersMemory;
	std::vector<void*> uniformBuffersMapped;
}Buffers;

typedef struct {
	VkImage textureImage;
	VkDeviceMemory textureImageMemory;
	VkImageView textureImageView;
	VkSampler textureSampler;
	VkImage depthImage;
	VkDeviceMemory depthImageMemory;
	VkImageView depthImageView;
}Textures;

typedef struct {
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
}Meshes;

typedef struct {
	VkPipeline graphicsPipeline;
	VkDescriptorSetLayout descriptorSetLayout;
	VkPipelineLayout pipelineLayout;
	VkRenderPass renderPass;
	VkCommandPool commandPool;
	VkDescriptorPool descriptorPool;
	VkDescriptorSet *descriptorSets;
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
	Buffers buffers;
	Textures textures;
	Meshes meshes;
}State;

enum SwapchainBuffering {
	SWAPCHAIN_SINGLE_BUFFERING = 1,
	SWAPCHAIN_DOUBLE_BUFFERING = 2,
	SWAPCHAIN_TRIPPLE_BUFFERING = 3,
};