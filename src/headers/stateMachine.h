#pragma once
#define _CRT_SECURE_NO_WARNINGS
#define GLFW_INCLUDE_VULKAN
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <GLFW/glfw3.h>
#include <vulkan/vulkan_core.h>
#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <signal.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/hash.hpp>
#include <algorithm>
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

enum struct CameraMovement {
	FORWARD,
	BACKWARD,
	LEFT,
	RIGHT,
	UP,
	DOWN
};
struct Camera {
	glm::vec3 front = glm::vec3 (0.0f ,0.0f ,1.0f);   
	glm::vec3 right = glm:: vec3 (-1.0f,0.0f,0.0f);   
	glm::vec3 worldUp = glm::vec3 (0.0f, 1.0f, 0.0f);
	glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f);  // Start at world origin
	glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);        // Y-axis as world up
	float yaw = -90.0f;                                 // Look along negative Z-axis (OpenGL convention)
	float pitch = 0.0f;
	float zoom = (100.0f);

	float movementSpeed = 1;    
	float mouseSensitivity = 0.1f; 

	bool lookMode = true;

	void updateCameraVectors() {
		// Calculate the new front vector
		glm::vec3 newFront;
		newFront.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
		newFront.y = sin(glm::radians(pitch));
		newFront.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
		front = glm::normalize(newFront);

		// Recalculate the right and up vectors
		right = glm::normalize(glm::cross(front, worldUp));
		up = glm::normalize(glm::cross(right, front));
	}
	glm::mat4 getViewMatrix() const {
		return glm::lookAt(position - front, position, up);
	};
	glm::mat4 getProjectionMatrix(float aspectRatio, float nearPlane, float farPlane) const {
		return glm::perspective(glm::radians(zoom), aspectRatio, nearPlane, farPlane);
	};

	glm::vec3 getPosition() const { return position; }
	glm::vec3 getFront() const { return front; }
	float getZoom() const { return zoom; }

	void processKeyboard(CameraMovement direction, float deltaTime) {
		float velocity = movementSpeed * deltaTime;

		switch (direction) {
		case CameraMovement::FORWARD:
			position += front * velocity;
			break;
		case CameraMovement::BACKWARD:
			position -= front * velocity;
			break;
		case CameraMovement::LEFT:
			position -= right * velocity;
			break;
		case CameraMovement::RIGHT:
			position += right * velocity;
			break;
		case CameraMovement::UP:
			position += up * velocity;
			break;
		case CameraMovement::DOWN:
			position -= up * velocity;
			break;
		}
	}
	void processMouseMovement(float xOffset, float yOffset, bool constrainPitch) {
		xOffset *= mouseSensitivity;
		yOffset *= mouseSensitivity;

		yaw += xOffset;
		pitch += yOffset;

		// Constrain pitch to avoid flipping
		if (constrainPitch) {
			pitch = std::clamp(pitch, -89.0f, 89.0f);
		}

		// Update camera vectors based on updated Euler angles
		updateCameraVectors();
	}
};


struct UniformBufferObject {
	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 proj;

	glm::vec4 lightPositions[4];  // Position and radius
	glm::vec4 lightColors[4];     // RGB color and intensity
	glm::vec4 camPos;             // Camera position for view-dependent effects

	float exposure = 4.5f;         // Exposure for HDR rendering
	float gamma = 2.2f;            // Gamma correction value
	float prefilteredCubeMipLevels = 1.0f;  // For image-based lighting
	float scaleIBLAmbient = 1.0f; // Scale factor for ambient lighting
};

struct PushConstantBlock {
	glm::vec4 baseColorFactor;            // RGB base color and alpha
	float metallicFactor;                 // How metallic the surface is
	float roughnessFactor;                // How rough the surface is
	int baseColorTextureSet;              // Texture coordinate set for base color
	int physicalDescriptorTextureSet;     // Texture coordinate set for metallic-roughness
	int normalTextureSet;                 // Texture coordinate set for normal map
	int occlusionTextureSet;              // Texture coordinate set for occlusion
	int emissiveTextureSet;               // Texture coordinate set for emission
	float alphaMask;                      // Whether to use alpha masking
	float alphaMaskCutoff;                // Alpha threshold for masking
};

struct GameObject {

	float sortKey;  // Key for sorting (e.g., distance to camera)

	// Transform properties
	glm::vec3 position = { 0.0f, 0.0f, 0.0f };
	glm::vec3 rotation = { 0.0f, 0.0f, 0.0f };
	glm::vec3 scale = { 1.0f, 1.0f, 1.0f };

	// Uniform buffer for this object (one per frame in flight)
	std::vector<VkBuffer> uniformBuffers;
	std::vector<VkDeviceMemory> uniformBuffersMemory;
	std::vector<void*> uniformBuffersMapped;

	// Descriptor sets for this object (one per frame in flight)
	std::vector<VkDescriptorSet> descriptorSets;
	// Calculate model matrix based on position, rotation, and scale
	glm::mat4 getModelMatrix() const {
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, position);
		model = glm::rotate(model, rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::rotate(model, rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::rotate(model, rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
		model = glm::scale(model, scale);
		return model;
	}
};
	const int objectsMax = 3;
struct Scene {	
	std::array<GameObject, objectsMax> gameObjects;
	Camera camera;
};

struct Input {
	float lastX = 0.0f, lastY = 0.0f;
	bool firstMouse = true;
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
	uint32_t MAX_OBJECTS;
	VkAllocationCallbacks allocator;
	VkComponentMapping swapchainComponentsMapping;
	VkClearValue backgroundColor;
	VkSampleCountFlagBits msaaSamples;
	const std::string KOBOLD_TEXTURE_PATH;
	const std::string KOBOLD_MODEL_PATH;

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
	uint32_t mipLevels;

	VkImage colorImage;
	VkDeviceMemory colorImageMemory;
	VkImageView colorImageView;
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
	Scene scene;
}State;

enum SwapchainBuffering {
	SWAPCHAIN_SINGLE_BUFFERING = 1,
	SWAPCHAIN_DOUBLE_BUFFERING = 2,
	SWAPCHAIN_TRIPPLE_BUFFERING = 3,
};