#include "headers/buffers.h"
//Utility
uint32_t findMemoryType(State* state, VkMemoryRequirements memRequirements, VkMemoryPropertyFlags properties) {
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(state->context.physicalDevice, &memProperties);

	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
		if ((memRequirements.memoryTypeBits & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
			return i;
		}
	}

	throw std::runtime_error("failed to find suitable memory type!");
}
//Buffers
void createBuffer(State* state, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateBuffer(state->context.device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
		throw std::runtime_error("failed to create buffer!");
	}
	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(state->context.device, buffer, &memRequirements);

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = findMemoryType(state,memRequirements, properties);

	if (vkAllocateMemory(state->context.device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate buffer memory!");
	}

	vkBindBufferMemory(state->context.device, buffer, bufferMemory, 0);
}
VkCommandBuffer beginSingleTimeCommands(State* state) {
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = state->renderer.commandPool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(state->context.device, &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	return commandBuffer;
}
void endSingleTimeCommands(State *state,VkCommandBuffer commandBuffer) {
	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(state->context.queue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(state->context.queue);

	vkFreeCommandBuffers(state->context.device, state->renderer.commandPool, 1, &commandBuffer);
}
void copyBuffer(State* state, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
	VkCommandBuffer commandBuffer = beginSingleTimeCommands(state);

	VkBufferCopy copyRegion{};
	copyRegion.size = size;
	vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

	endSingleTimeCommands(state, commandBuffer);
};

void frameBuffersCreate(State* state) {
	uint32_t frameBufferCount = state->window.swapchain.imageCount;
	state->window.framebufferResized = false;
	state->buffers.framebuffers = (VkFramebuffer*)malloc(frameBufferCount * sizeof(VkFramebuffer));
	PANIC(!state->buffers.framebuffers, "Failed To Allocate Framebuffer Array Memory");
	VkExtent2D framebufferExtent = state->window.swapchain.imageExtent;

	for (int i = 0; i < (int)frameBufferCount; i++) {
		std::array<VkImageView, 3> attachments = {
			state->textures.colorImageView,
			state->textures.depthImageView,
			state->window.swapchain.imageViews[i]
		};

		VkFramebufferCreateInfo framebufferInfo{};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = state->renderer.renderPass;
		framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		framebufferInfo.pAttachments = attachments.data();
		framebufferInfo.width = state->window.swapchain.imageExtent.width;
		framebufferInfo.height = state->window.swapchain.imageExtent.height;
		framebufferInfo.layers = 1;
		PANIC(vkCreateFramebuffer(state->context.device, &framebufferInfo, nullptr, &state->buffers.framebuffers[i]), "Failed To Create Framebuffer");

	};
};
void frameBuffersDestroy(State* state) {
	uint32_t frameBufferCount = state->window.swapchain.imageCount;
	for (int i = 0; i < (int)frameBufferCount; i++) {
		vkDestroyFramebuffer(state->context.device, state->buffers.framebuffers[i], nullptr);

	};
};

void vertexBufferCreate(State* state) {
	VkDeviceSize bufferSize = sizeof(state->meshes.vertices[0]) * state->meshes.vertices.size();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	createBuffer(state, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(state->context.device, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, state->meshes.vertices.data(), (size_t)bufferSize);
	vkUnmapMemory(state->context.device, stagingBufferMemory);


	createBuffer(state, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, state->buffers.vertexBuffer, state->buffers.vertexBufferMemory);
	copyBuffer(state, stagingBuffer, state->buffers.vertexBuffer, bufferSize);

	vkDestroyBuffer(state->context.device, stagingBuffer, nullptr);
	vkFreeMemory(state->context.device, stagingBufferMemory, nullptr);
};
void vertexBufferDestroy(State* state) {
	vkDestroyBuffer(state->context.device, state->buffers.vertexBuffer, nullptr);
	vkFreeMemory(state->context.device, state->buffers.vertexBufferMemory, nullptr);
};

void indexBufferCreate(State* state) {
	VkDeviceSize bufferSize = sizeof(state->meshes.indices[0]) * state->meshes.indices.size();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	createBuffer(state, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(state->context.device, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, state->meshes.indices.data(), (size_t)bufferSize);
	vkUnmapMemory(state->context.device, stagingBufferMemory);

	createBuffer(state, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, state->buffers.indexBuffer, state->buffers.indexBufferMemory);

	copyBuffer(state, stagingBuffer, state->buffers.indexBuffer, bufferSize);

	vkDestroyBuffer(state->context.device, stagingBuffer, nullptr);
	vkFreeMemory(state->context.device, stagingBufferMemory, nullptr);
};
void indexBufferDestroy(State* state) {
	vkDestroyBuffer(state->context.device, state->buffers.indexBuffer, nullptr);
	vkFreeMemory(state->context.device, state->buffers.indexBufferMemory, nullptr);
};

void uniformBuffersCreate(State* state) {
	for (auto& gameObject : state->scene.gameObjects) {
		gameObject.uniformBuffers.clear();
		gameObject.uniformBuffersMemory.clear();
		gameObject.uniformBuffersMapped.clear();

		// Create uniform buffers for each frame in flight
		for (size_t i = 0; i < state->config.swapchainBuffering; i++) {
			VkDeviceSize bufferSize = sizeof(UniformBufferObject);
			VkBuffer buffer({});
			VkDeviceMemory bufferMem({});

			createBuffer(state, bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, buffer, bufferMem);

			gameObject.uniformBuffers.emplace_back(std::move(buffer));
			gameObject.uniformBuffersMemory.emplace_back(std::move(bufferMem));
			gameObject.uniformBuffersMapped.resize(gameObject.uniformBuffersMemory.size());
			vkMapMemory(state->context.device, gameObject.uniformBuffersMemory[i], 0, bufferSize, 0, &gameObject.uniformBuffersMapped[i]);

		}
	}
};
void uniformBuffersUpdate(State* state) {
	static auto startTime = std::chrono::high_resolution_clock::now();
	auto currentTime = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration<float>(currentTime - startTime).count();

	// Camera and projection matrices (shared by all objects)
	glm::mat4 view = state->scene.camera.getViewMatrix();
	glm::mat4 proj = state->scene.camera.getProjectionMatrix(static_cast<float>(state->window.swapchain.imageExtent.width / state->window.swapchain.imageExtent.height),
		0.1f, 20.0f);
	proj[1][1] *= -1; // Flip Y for Vulkan

	// Update uniform buffers for each object
	for (auto& gameObject : state->scene.gameObjects) {
		// Apply continuous rotation to the object
		gameObject.rotation.y += 0.01f; // Slow rotation around Y axis

		// Get the model matrix for this object
		glm::mat4 initialRotation = glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		glm::mat4 model = gameObject.getModelMatrix() * initialRotation;

		// Create and update the UBO
		UniformBufferObject ubo{
			.model = model,
			.view = view,
			.proj = proj
		};

		// Set up lights
		// Light 1: White light from above
		ubo.lightPositions[0] = glm::vec4(0.0f, 5.0f, 5.0f, 1.0f);
		ubo.lightColors[0] = glm::vec4(300.0f, 300.0f, 300.0f, 1.0f);

		// Light 2: Blue light from the left
		ubo.lightPositions[1] = glm::vec4(-5.0f, 0.0f, 0.0f, 1.0f);
		ubo.lightColors[1] = glm::vec4(0.0f, 0.0f, 300.0f, 1.0f);

		// Light 3: Red light from the right
		ubo.lightPositions[2] = glm::vec4(5.0f, 0.0f, 0.0f, 1.0f);
		ubo.lightColors[2] = glm::vec4(300.0f, 0.0f, 0.0f, 1.0f);

		// Light 4: Green light from behind
		ubo.lightPositions[3] = glm::vec4(0.0f, -5.0f, 0.0f, 1.0f);
		ubo.lightColors[3] = glm::vec4(0.0f, 300.0f, 0.0f, 1.0f);

		// Set camera position for view-dependent effects
		ubo.camPos = glm::vec4(state->scene.camera.getPosition(),1.0f);

		// Set PBR parameters
		ubo.exposure = 4.5f;
		ubo.gamma = 2.2f;
		ubo.prefilteredCubeMipLevels = 1.0f;
		ubo.scaleIBLAmbient = 1.0f;


		// Copy the UBO data to the mapped memory
		memcpy(gameObject.uniformBuffersMapped[state->renderer.frameIndex], &ubo, sizeof(ubo));
	}
};
void uniformBuffersDestroy(State* state) {
	for (auto& gameObject : state->scene.gameObjects) {
		for (size_t i = 0; i < state->config.swapchainBuffering; i++) {
			vkDestroyBuffer(state->context.device, gameObject.uniformBuffers[i], nullptr);
			vkFreeMemory(state->context.device, gameObject.uniformBuffersMemory[i], nullptr);
		};
	};
};

void commandBufferGet(State* state) {
	VkCommandBufferAllocateInfo allocInfo{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.commandPool = state->renderer.commandPool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = state->config.swapchainBuffering,
	};
	state->buffers.commandBuffer = (VkCommandBuffer*)malloc(state->config.swapchainBuffering * sizeof(VkCommandBuffer));
	PANIC(vkAllocateCommandBuffers(state->context.device, &allocInfo, state->buffers.commandBuffer), "Failed To Create Command Buffer");
};
void commandBufferRecord(State* state) {
	VkCommandBufferBeginInfo beginInfo{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
	};
	vkBeginCommandBuffer(state->buffers.commandBuffer[state->renderer.frameIndex], &beginInfo);

	std::array<VkClearValue, 2> clearValues{};
	clearValues[0].color = { state->config.backgroundColor.color };
	clearValues[1].depthStencil = { 1.0f, 0 };

	VkRenderPassBeginInfo renderPassInfo{
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.renderPass = state->renderer.renderPass,
		.framebuffer = state->buffers.framebuffers[state->renderer.imageAquiredIndex],
		.clearValueCount = static_cast<uint32_t>(clearValues.size()),
		.pClearValues = clearValues.data()
	};
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = state->window.swapchain.imageExtent;
	vkCmdBeginRenderPass(state->buffers.commandBuffer[state->renderer.frameIndex], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
	vkCmdBindPipeline(state->buffers.commandBuffer[state->renderer.frameIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, state->renderer.graphicsPipeline);

	VkBuffer vertexBuffers[] = { state->buffers.vertexBuffer };
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(state->buffers.commandBuffer[state->renderer.frameIndex], 0, 1, vertexBuffers, offsets);
	vkCmdBindIndexBuffer(state->buffers.commandBuffer[state->renderer.frameIndex], state->buffers.indexBuffer, 0, VK_INDEX_TYPE_UINT32);

	VkViewport viewport{
		.x = 0.0f,
		.y = 0.0f,
		.width = (float)state->window.swapchain.imageExtent.width,
		.height = (float)state->window.swapchain.imageExtent.height,
		.minDepth = 0.0f,
		.maxDepth = 1.0f,
	};
	vkCmdSetViewport(state->buffers.commandBuffer[state->renderer.frameIndex], 0, 1, &viewport);
	VkRect2D scissor{
		.offset = { 0, 0 },
		.extent = state->window.swapchain.imageExtent,
	};
	vkCmdSetScissor(state->buffers.commandBuffer[state->renderer.frameIndex], 0, 1, &scissor);

	PushConstantBlock pc{};
	pc.baseColorFactor = glm::vec4(1.0f);
	pc.metallicFactor = 1.0f;
	pc.roughnessFactor = 1.0f;
	pc.baseColorTextureSet = 0;
	pc.physicalDescriptorTextureSet = 0;
	pc.normalTextureSet = 0;
	pc.occlusionTextureSet = 0;
	pc.emissiveTextureSet = 0;
	pc.alphaMask = 0.0f;
	pc.alphaMaskCutoff = 0.5f;

	vkCmdPushConstants(state->buffers.commandBuffer[state->renderer.frameIndex],
		state->renderer.pipelineLayout,
		VK_SHADER_STAGE_FRAGMENT_BIT, // must match pcRange.stageFlags
		0,
		sizeof(PushConstantBlock),
		&pc
	);


	for (const auto& gameObject : state->scene.gameObjects) {
		// Bind the descriptor set for this object
		vkCmdBindDescriptorSets(state->buffers.commandBuffer[state->renderer.frameIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, state->renderer.pipelineLayout, 0, 1, &gameObject.descriptorSets[state->renderer.frameIndex], 0, nullptr);

		// Draw the object
		vkCmdDrawIndexed(state->buffers.commandBuffer[state->renderer.frameIndex], static_cast<uint32_t>(state->meshes.indices.size()), 1, 0, 0, 0);
	};

	vkCmdEndRenderPass(state->buffers.commandBuffer[state->renderer.frameIndex]);
	PANIC(vkEndCommandBuffer(state->buffers.commandBuffer[state->renderer.frameIndex]), "Failed To Record Command Buffer");
};
