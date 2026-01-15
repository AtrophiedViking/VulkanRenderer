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
		std::array<VkImageView, 2> attachments = {
			state->window.swapchain.imageViews[i],
			state->textures.depthImageView
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
	VkDeviceSize bufferSize = sizeof(UniformBufferObject);

	state->buffers.uniformBuffers = (VkBuffer*)malloc(state->config.swapchainBuffering * sizeof(VkBuffer));
	state->buffers.uniformBuffersMemory = (VkDeviceMemory*)malloc(state->config.swapchainBuffering * sizeof(VkDeviceMemory));
	state->buffers.uniformBuffersMapped.resize(state->config.swapchainBuffering);
	PANIC(!state->buffers.uniformBuffers || !state->buffers.uniformBuffersMemory, "Failed To Allocate Uniform Buffer Memory");
	for (size_t i = 0; i < state->config.swapchainBuffering; i++) {
		createBuffer(state, bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, state->buffers.uniformBuffers[i], state->buffers.uniformBuffersMemory[i]);

		vkMapMemory(state->context.device, state->buffers.uniformBuffersMemory[i], 0, bufferSize, 0, &state->buffers.uniformBuffersMapped[i]);
	};
};
void uniformBuffersUpdate(State* state) {
	static auto startTime = std::chrono::high_resolution_clock::now();

	auto currentTime = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
	//ViewPort Matric
	UniformBufferObject ubo{
		.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
		.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
		.proj = glm::perspective(glm::radians(45.0f), state->window.swapchain.imageExtent.width / (float)state->window.swapchain.imageExtent.height, 0.1f, 10.0f),
	};
	ubo.proj[1][1] *= -1,
		memcpy(state->buffers.uniformBuffersMapped[state->renderer.imageAquiredIndex], &ubo, sizeof(ubo));
};
void uniformBuffersDestroy(State* state) {
	for (size_t i = 0; i < state->config.swapchainBuffering; i++) {
		vkDestroyBuffer(state->context.device, state->buffers.uniformBuffers[i], nullptr);
		vkFreeMemory(state->context.device, state->buffers.uniformBuffersMemory[i], nullptr);
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
	vkCmdBindDescriptorSets(state->buffers.commandBuffer[state->renderer.frameIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, state->renderer.pipelineLayout, 0, 1, &state->renderer.descriptorSets[state->renderer.frameIndex], 0, nullptr);
	VkRect2D scissor{
		.offset = { 0, 0 },
		.extent = state->window.swapchain.imageExtent,
	};
	vkCmdSetScissor(state->buffers.commandBuffer[state->renderer.frameIndex], 0, 1, &scissor);
	vkCmdDrawIndexed(state->buffers.commandBuffer[state->renderer.frameIndex], static_cast<uint32_t>(state->meshes.indices.size()), 1, 0, 0, 0);
	vkCmdEndRenderPass(state->buffers.commandBuffer[state->renderer.frameIndex]);
	PANIC(vkEndCommandBuffer(state->buffers.commandBuffer[state->renderer.frameIndex]), "Failed To Record Command Buffer");
};
