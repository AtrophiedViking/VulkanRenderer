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
VkCommandBuffer beginSingleTimeCommands(State* state, VkCommandPool commandPool) {
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = commandPool,
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
	VkCommandBuffer commandBuffer = beginSingleTimeCommands(state, state->renderer.commandPool);

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
			state->texture.colorImageView,
			state->texture.depthImageView,
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

void vertexBufferCreateForMesh(State* state, const std::vector<Vertex>& vertices, VkBuffer& vertexBuffer, VkDeviceMemory& vertexMemory) {

	if (vertices.empty()) return;

	VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	createBuffer(state, bufferSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(state->context.device, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, vertices.data(), (size_t)bufferSize);
	vkUnmapMemory(state->context.device, stagingBufferMemory);

	createBuffer(state, bufferSize,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		vertexBuffer, vertexMemory);

	copyBuffer(state, stagingBuffer, vertexBuffer, bufferSize);

	vkDestroyBuffer(state->context.device, stagingBuffer, nullptr);
	vkFreeMemory(state->context.device, stagingBufferMemory, nullptr);
}

void vertexBufferDestroy(State* state) {
	vkDestroyBuffer(state->context.device, state->buffers.vertexBuffer, nullptr);
	vkFreeMemory(state->context.device, state->buffers.vertexBufferMemory, nullptr);
};

void indexBufferCreateForMesh(State* state, const std::vector<uint32_t>& indices, VkBuffer& indexBuffer, VkDeviceMemory& indexMemory) {

	if (indices.empty()) return;

	VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	createBuffer(state, bufferSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(state->context.device, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, indices.data(), (size_t)bufferSize);
	vkUnmapMemory(state->context.device, stagingBufferMemory);

	createBuffer(state, bufferSize,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		indexBuffer, indexMemory);

	copyBuffer(state, stagingBuffer, indexBuffer, bufferSize);

	vkDestroyBuffer(state->context.device, stagingBuffer, nullptr);
	vkFreeMemory(state->context.device, stagingBufferMemory, nullptr);
}

void indexBufferDestroy(State* state) {
	vkDestroyBuffer(state->context.device, state->buffers.indexBuffer, nullptr);
	vkFreeMemory(state->context.device, state->buffers.indexBufferMemory, nullptr);
};

void uniformBuffersCreate(State* state) {
	// One global UBO per frame in flight
	state->renderer.uniformBuffers.resize(state->config.swapchainBuffering);
	state->renderer.uniformBuffersMemory.resize(state->config.swapchainBuffering);
	state->renderer.uniformBuffersMapped.resize(state->config.swapchainBuffering);

	VkDeviceSize bufferSize = sizeof(UniformBufferObject);

	for (size_t i = 0; i < state->config.swapchainBuffering; i++) {
		VkBuffer buffer{};
		VkDeviceMemory bufferMem{};

		createBuffer(
			state,
			bufferSize,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			buffer,
			bufferMem
		);

		state->renderer.uniformBuffers[i] = buffer;
		state->renderer.uniformBuffersMemory[i] = bufferMem;

		vkMapMemory(
			state->context.device,
			state->renderer.uniformBuffersMemory[i],
			0,
			bufferSize,
			0,
			&state->renderer.uniformBuffersMapped[i]
		);
	}
}

void uniformBuffersUpdate(State* state) {
	static auto startTime = std::chrono::high_resolution_clock::now();
	auto currentTime = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration<float>(currentTime - startTime).count();

	// Camera matrices
	glm::mat4 view = state->scene.camera.getViewMatrix();

	float aspect = static_cast<float>(state->window.swapchain.imageExtent.width) /
		static_cast<float>(state->window.swapchain.imageExtent.height);

	glm::mat4 proj = state->scene.camera.getProjectionMatrix(aspect, 0.1f, 20.0f);
	proj[1][1] *= -1.0f; // Vulkan Y flip

	// Global UBO (model is identity; node transforms come from push constants)
	UniformBufferObject ubo{};
	ubo.model = glm::mat4(1.0f);
	ubo.view = view;
	ubo.proj = proj;

	// Lights
	ubo.lightPositions[0] = glm::vec4(0.0f, 5.0f, 5.0f, 1.0f);
	ubo.lightColors[0] = glm::vec4(300.0f, 300.0f, 300.0f, 1.0f);

	ubo.lightPositions[1] = glm::vec4(-5.0f, 0.0f, 0.0f, 1.0f);
	ubo.lightColors[1] = glm::vec4(0.0f, 0.0f, 300.0f, 1.0f);

	ubo.lightPositions[2] = glm::vec4(5.0f, 0.0f, 0.0f, 1.0f);
	ubo.lightColors[2] = glm::vec4(300.0f, 0.0f, 0.0f, 1.0f);

	ubo.lightPositions[3] = glm::vec4(0.0f, -5.0f, 0.0f, 1.0f);
	ubo.lightColors[3] = glm::vec4(0.0f, 300.0f, 0.0f, 1.0f);

	// Camera position
	ubo.camPos = glm::vec4(state->scene.camera.getPosition(), 1.0f);

	// PBR params
	ubo.exposure = 4.5f;
	ubo.gamma = 2.2f;
	ubo.prefilteredCubeMipLevels = 1.0f;
	ubo.scaleIBLAmbient = 1.0f;

	// Write into the mapped global UBO buffer for this frame
	void* data = state->renderer.uniformBuffersMapped[state->renderer.frameIndex];
	memcpy(data, &ubo, sizeof(ubo));
}

void uniformBuffersDestroy(State* state) {
		for (size_t i = 0; i < state->config.swapchainBuffering; i++) {
			vkDestroyBuffer(state->context.device, state->renderer.uniformBuffers[i], nullptr);
			vkFreeMemory(state->context.device, state->renderer.uniformBuffersMemory[i], nullptr);	
			state->renderer.uniformBuffersMapped[i] = nullptr;
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
	VkCommandBuffer cmd = state->buffers.commandBuffer[state->renderer.frameIndex];

	VkCommandBufferBeginInfo beginInfo{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
	};
	vkBeginCommandBuffer(cmd, &beginInfo);

	std::array<VkClearValue, 2> clearValues{};
	clearValues[0].color = { state->config.backgroundColor.color };
	clearValues[1].depthStencil = { 1.0f, 0 };

	VkRenderPassBeginInfo renderPassInfo{
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.renderPass = state->renderer.renderPass,
		.framebuffer = state->buffers.framebuffers[state->renderer.imageAquiredIndex],
		.clearValueCount = static_cast<uint32_t>(clearValues.size()),
		.pClearValues = clearValues.data(),
	};
	renderPassInfo.renderArea = {
		.offset = { 0, 0 },
		.extent = state->window.swapchain.imageExtent
	};

	vkCmdBeginRenderPass(cmd, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, state->renderer.graphicsPipeline);

	// Bind global UBO (set = 0), NOT a texture
	vkCmdBindDescriptorSets(
		cmd,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		state->renderer.pipelineLayout,
		0, // firstSet = 0 → set 0
		1,
		&state->renderer.descriptorSet,
		0,
		nullptr
	);

	VkViewport viewport{
		.x = 0.0f,
		.y = 0.0f,
		.width = (float)state->window.swapchain.imageExtent.width,
		.height = (float)state->window.swapchain.imageExtent.height,
		.minDepth = 0.0f,
		.maxDepth = 1.0f,
	};
	vkCmdSetViewport(cmd, 0, 1, &viewport);

	VkRect2D scissor{
		.offset = { 0, 0 },
		.extent = state->window.swapchain.imageExtent,
	};
	vkCmdSetScissor(cmd, 0, 1, &scissor);

	PushConstantBlock pushConstants{};
	pushConstants.baseColorFactor = { 1.0f, 1.0f, 1.0f, 1.0f };
	pushConstants.metallicFactor = 1.0f;
	pushConstants.roughnessFactor = 0.5f;
	pushConstants.baseColorTextureSet = 0;
	pushConstants.physicalDescriptorTextureSet = 1;
	pushConstants.normalTextureSet = 2;
	pushConstants.occlusionTextureSet = 3;
	pushConstants.emissiveTextureSet = 4;
	pushConstants.alphaMask = 0.0f;
	pushConstants.alphaMaskCutoff = 0.5f;
	
	vkCmdPushConstants(
		cmd,
		state->renderer.pipelineLayout,
		VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
		0,
		sizeof(PushConstantBlock),
		&pushConstants
	);

	// Textures (set 1) are bound inside drawMesh per material
	// 1. Gather draw items
	std::vector<DrawItem> items;
	gatherDrawItems(
		state->scene.rootNode,
		state->scene.camera.getPosition(),
		state->scene.materials,
		items
	);

	// 2. Split opaque and transparent
	std::vector<DrawItem> opaqueItems;
	std::vector<DrawItem> transparentItems;

	for (const DrawItem& item : items) {
		if (item.transparent)
			transparentItems.push_back(item);
		else
			opaqueItems.push_back(item);
	}

	// 3. Draw opaque (no sorting needed)
	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, state->renderer.graphicsPipeline);

	for (const DrawItem& item : opaqueItems) {
		drawMesh(state, cmd, *item.mesh);
	}

	// 4. Sort transparent back-to-front
	std::sort(transparentItems.begin(), transparentItems.end(),
		[](const DrawItem& a, const DrawItem& b) {
			return a.distanceToCamera > b.distanceToCamera; // far → near
		});

	// 5. Draw transparent with transparency pipeline
	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, state->renderer.transparencyPipeline);

	for (const DrawItem& item : transparentItems) {
		drawMesh(state, cmd, *item.mesh);
	}
	vkCmdEndRenderPass(cmd);


	guiDraw(state, cmd);

	PANIC(vkEndCommandBuffer(cmd), "Failed To Record Command Buffer");
}
