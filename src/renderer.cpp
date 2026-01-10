#pragma once
#include "headers/renderer.h"
//utility
static std::vector<char> shaderRead(const char* filePath) {
	std::ifstream file(filePath, std::ios::ate | std::ios::binary);
	PANIC(!file.is_open(), "Failed To Open Shader: %s", filePath);
	size_t fileSize = (size_t)file.tellg();
	std::vector<char> buffer(fileSize);
	file.seekg(0);
	file.read(buffer.data(), fileSize);
	file.close();
	return buffer;
};

static uint32_t findMemoryType(State *state, VkMemoryPropertyFlags properties) {
	vkGetPhysicalDeviceMemoryProperties(state->context.physicalDevice, &state->renderer.memProperties);

	for (uint32_t i = 0; i < state->renderer.memProperties.memoryTypeCount; i++) {
		if ((state->renderer.memRequirements.memoryTypeBits & (1 << i)) && (state->renderer.memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
			return i;
		}
	}

	throw std::runtime_error("failed to find suitable memory type!");
}

//Graphics Pipeline
void renderPassCreate(State* state) {
	VkAttachmentDescription colorAttachment{
		.format = state->window.swapchain.format,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
	};

	VkAttachmentReference colorAttachmentRef{
		.attachment = 0,
		.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
	};
	VkSubpassDescription subpass{
		.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
		.colorAttachmentCount = 1,
		.pColorAttachments = &colorAttachmentRef,
	};
	VkSubpassDependency dependency{
		.srcSubpass = VK_SUBPASS_EXTERNAL,
		.dstSubpass = 0,
		.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		.srcAccessMask = 0,
		.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
	};
	VkRenderPassCreateInfo renderPassInfo{
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		.attachmentCount = 1,
		.pAttachments = &colorAttachment,
		.subpassCount = 1,
		.pSubpasses = &subpass,
		.dependencyCount = 1,
		.pDependencies = &dependency,
	};
	PANIC(vkCreateRenderPass(state->context.device, &renderPassInfo, nullptr, &state->renderer.renderPass), "Failed To Create Render Pass")
};
void renderPassDestroy(State* state) {
	vkDestroyRenderPass(state->context.device, state->renderer.renderPass, nullptr);
};

void descriptorSetLayoutCreate(State* state) {
	VkDescriptorSetLayoutBinding uboLayoutBinding{
		.binding = 0,
		.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
		.pImmutableSamplers = nullptr, // Optional
	};
	VkDescriptorSetLayoutCreateInfo layoutInfo{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.bindingCount = 1,
		.pBindings = &uboLayoutBinding,
	};

	if (vkCreateDescriptorSetLayout(state->context.device, &layoutInfo, nullptr, &state->renderer.descriptorSetLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor set layout!");
	};
};
void descriptorSetLayoutDestroy(State* state) {
	vkDestroyDescriptorSetLayout(state->context.device, state->renderer.descriptorSetLayout, nullptr);
};
void graphicsPipelineCreate(State* state) {
	//ShaderModules
	auto vertShaderCode = shaderRead("./res/vert.spv");
	auto fragShaderCode = shaderRead("./res/frag.spv");
	VkShaderModule vertShaderModule;
	VkShaderModuleCreateInfo vertShaderModuleInfo{
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.codeSize = vertShaderCode.size(),
		.pCode = reinterpret_cast<const uint32_t*>(vertShaderCode.data()),
	};
	PANIC(vkCreateShaderModule(state->context.device, &vertShaderModuleInfo, nullptr, &vertShaderModule), "Failed To Create Vertex Shader Module");
	VkShaderModule fragShaderModule;
	VkShaderModuleCreateInfo fragShaderModuleInfo{
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.codeSize = fragShaderCode.size(),
		.pCode = reinterpret_cast<const uint32_t*>(fragShaderCode.data()),
	};
	PANIC(vkCreateShaderModule(state->context.device, &fragShaderModuleInfo, nullptr, &fragShaderModule), "Failed To Create Fragment Shader Module");

	//ShaderStages
	VkPipelineShaderStageCreateInfo vertShaderStageInfo{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.stage = VK_SHADER_STAGE_VERTEX_BIT,
		.module = vertShaderModule,
		.pName = "main"
	};
	VkPipelineShaderStageCreateInfo fragShaderStageInfo{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
		.module = fragShaderModule,
		.pName = "main"
	};
	VkPipelineShaderStageCreateInfo shaderStages[]{ vertShaderStageInfo, fragShaderStageInfo };

	//DynamicStates
	std::vector<VkDynamicState> dynamicStates = {
	VK_DYNAMIC_STATE_VIEWPORT,
	VK_DYNAMIC_STATE_SCISSOR
	};
	VkPipelineDynamicStateCreateInfo dynamicState{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
		.dynamicStateCount = (uint32_t)dynamicStates.size(),
		.pDynamicStates = dynamicStates.data(),
	};
	//VertexInputs
	auto bindingDescription = Vertex::getBindingDescription();
	auto attributeDescriptions = Vertex::getAttributeDescriptions();
	VkPipelineVertexInputStateCreateInfo vertexInputInfo{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		.vertexBindingDescriptionCount = 1,
		.pVertexBindingDescriptions = &bindingDescription,
		.vertexAttributeDescriptionCount = (uint32_t)attributeDescriptions.size(),
		.pVertexAttributeDescriptions = attributeDescriptions.data(),
	};

	//InputAssembly
	VkPipelineInputAssemblyStateCreateInfo inputAssembly{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		.primitiveRestartEnable = VK_FALSE,
	};

	//ViewPort
	VkViewport viewport{
		.x = 0.0f,
		.y = 0.0f,
		.width = (float)state->window.swapchain.imageExtent.width,
		.height = (float)state->window.swapchain.imageExtent.height,
		.minDepth = 0.0f,
		.maxDepth = 1.0f,
	};
	VkRect2D scissor{
		.offset = { 0, 0 },
		.extent = state->window.swapchain.imageExtent,
	};
	VkPipelineViewportStateCreateInfo viewportState{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
		.viewportCount = 1,
		.pViewports = &viewport,
		.scissorCount = 1,
		.pScissors = &scissor
	};
	//Rasterizor
	VkPipelineRasterizationStateCreateInfo rasterizer{
	.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
	.depthClampEnable = VK_FALSE,
	.rasterizerDiscardEnable = VK_FALSE,
	.polygonMode = VK_POLYGON_MODE_FILL,
	.cullMode = VK_CULL_MODE_BACK_BIT,
	.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
	.depthBiasEnable = VK_FALSE,
	.lineWidth = 1.0f,
	};
	//MultiSamplin
	VkPipelineMultisampleStateCreateInfo multisampling{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
		.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
		.sampleShadingEnable = VK_FALSE,
		.minSampleShading = 1.0f, // Optional
		.pSampleMask = nullptr, // Optional
		.alphaToCoverageEnable = VK_FALSE, // Optional
		.alphaToOneEnable = VK_FALSE, // Optional
	};
	//ColorBlending
	VkPipelineColorBlendAttachmentState colorBlendAttachment{
		.blendEnable = VK_FALSE,
		.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
	};
	VkPipelineColorBlendStateCreateInfo colorBlending{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
		.logicOpEnable = VK_FALSE,
		.attachmentCount = 1,
		.pAttachments = &colorBlendAttachment,
	};
	//PipelineLayout
	VkPipelineLayoutCreateInfo pipelineLayoutInfo{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.setLayoutCount = 1,
		.pSetLayouts = &state->renderer.descriptorSetLayout,
	};
	PANIC(vkCreatePipelineLayout(state->context.device, &pipelineLayoutInfo, nullptr, &state->renderer.pipelineLayout), "Failed To Create Pipeline Layout");
	
	//GraphicsPipeline
	VkGraphicsPipelineCreateInfo pipelineInfo{
		.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		.stageCount = 2,
		.pStages = shaderStages,
		.pVertexInputState = &vertexInputInfo,
		.pInputAssemblyState = &inputAssembly,
		.pViewportState = &viewportState,
		.pRasterizationState = &rasterizer,
		.pMultisampleState = &multisampling,
		.pDepthStencilState = nullptr, // Optional
		.pColorBlendState = &colorBlending,
		.pDynamicState = &dynamicState,
		.layout = state->renderer.pipelineLayout,
		.renderPass = state->renderer.renderPass,
		.subpass = 0,
		.basePipelineHandle = VK_NULL_HANDLE, // Optional
	};
	PANIC(vkCreateGraphicsPipelines(state->context.device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &state->renderer.graphicsPipeline),"Failed To Create GraphicsPipeline");
	//cleanup
	vkDestroyShaderModule(state->context.device, fragShaderModule, nullptr);
	vkDestroyShaderModule(state->context.device, vertShaderModule, nullptr);
};
void graphicsPipelineDestroy(State* state) {
	vkDestroyPipelineLayout(state->context.device, state->renderer.pipelineLayout, nullptr);
	vkDestroyPipeline(state->context.device, state->renderer.graphicsPipeline, nullptr);
};

void frameBuffersCreate(State* state) {
	uint32_t frameBufferCount = state->window.swapchain.imageCount;
	state->window.framebufferResized = false;
	state->renderer.framebuffers = (VkFramebuffer*)malloc(frameBufferCount * sizeof(VkFramebuffer));
	PANIC(!state->renderer.framebuffers, "Failed To Allocate Framebuffer Array Memory");
	VkExtent2D framebufferExtent = state->window.swapchain.imageExtent;

	for (int i = 0; i < (int)frameBufferCount; i++) {
		VkImageView attachments[] = { 
			state->window.swapchain.imageViews[i] 
		};

		VkFramebufferCreateInfo framebufferInfo{
			.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			.renderPass = state->renderer.renderPass,
			.attachmentCount = 1,
			.pAttachments = attachments,
			.width = state->window.swapchain.imageExtent.width,
			.height = state->window.swapchain.imageExtent.height,
			.layers = 1,
		};
		PANIC(vkCreateFramebuffer(state->context.device, &framebufferInfo, nullptr, &state->renderer.framebuffers[i]), "Failed To Create Framebuffer");
		
	};
};
void frameBuffersDestroy(State* state) {
	uint32_t frameBufferCount = state->window.swapchain.imageCount;
	for (int i = 0; i < (int)frameBufferCount; i++) {
		vkDestroyFramebuffer(state->context.device, state->renderer.framebuffers[i], nullptr);

	};
};

void commandPoolCreate(State* state) {
	
	VkCommandPoolCreateInfo poolInfo{
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
		.queueFamilyIndex = state->context.queueFamilyIndex,
	};
	PANIC(vkCreateCommandPool(state->context.device, &poolInfo, nullptr, &state->renderer.commandPool),"Failed To Create Command Pool");
};
void commandPoolDestroy(State* state) {
	vkDestroyCommandPool(state->context.device, state->renderer.commandPool, nullptr);
};

void createBuffer(State* state, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateBuffer(state->context.device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
		throw std::runtime_error("failed to create buffer!");
	}

	vkGetBufferMemoryRequirements(state->context.device, buffer, &state->renderer.memRequirements);

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = state->renderer.memRequirements.size;
	allocInfo.memoryTypeIndex = findMemoryType(state, properties);

	if (vkAllocateMemory(state->context.device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate buffer memory!");
	}

	vkBindBufferMemory(state->context.device, buffer, bufferMemory, 0);
}
void copyBuffer(State* state, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
	VkCommandBufferAllocateInfo allocInfo{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.commandPool = state->renderer.commandPool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = 1,
	};
	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(state->context.device, &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);
	VkBufferCopy copyRegion{};
	copyRegion.srcOffset = 0; // Optional
	copyRegion.dstOffset = 0; // Optional
	copyRegion.size = size;
	vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(state->context.queue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(state->context.queue);
	vkFreeCommandBuffers(state->context.device, state->renderer.commandPool, 1, &commandBuffer);
};

void vertexBufferCreate(State* state) {
	VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	createBuffer(state, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(state->context.device, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, vertices.data(), (size_t)bufferSize);
	vkUnmapMemory(state->context.device, stagingBufferMemory);
	
	
	createBuffer(state ,bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, state->renderer.vertexBuffer, state->renderer.vertexBufferMemory);
	copyBuffer(state, stagingBuffer, state->renderer.vertexBuffer, bufferSize);
	
	vkDestroyBuffer(state->context.device, stagingBuffer, nullptr);
	vkFreeMemory(state->context.device, stagingBufferMemory, nullptr);
};
void vertexBufferDestroy(State* state) {
	vkDestroyBuffer(state->context.device, state->renderer.vertexBuffer, nullptr);
	vkFreeMemory(state->context.device, state->renderer.vertexBufferMemory, nullptr);
};

void indexBufferCreate(State* state) {
	VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	createBuffer(state, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(state->context.device, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, indices.data(), (size_t)bufferSize);
	vkUnmapMemory(state->context.device, stagingBufferMemory);

	createBuffer(state, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, state->renderer.indexBuffer, state->renderer.indexBufferMemory);

	copyBuffer(state, stagingBuffer, state->renderer.indexBuffer, bufferSize);

	vkDestroyBuffer(state->context.device, stagingBuffer, nullptr);
	vkFreeMemory(state->context.device, stagingBufferMemory, nullptr);
};
void indexBufferDestroy(State* state) {
	vkDestroyBuffer(state->context.device, state->renderer.indexBuffer, nullptr);
	vkFreeMemory(state->context.device, state->renderer.indexBufferMemory, nullptr);
};

void uniformBuffersCreate(State* state) {
	VkDeviceSize bufferSize = sizeof(UniformBufferObject);

	state->renderer.uniformBuffers = (VkBuffer*)malloc(state->config.swapchainBuffering * sizeof(VkBuffer));
	state->renderer.uniformBuffersMemory = (VkDeviceMemory*)malloc(state->config.swapchainBuffering * sizeof(VkDeviceMemory));
	state->renderer.uniformBuffersMapped.resize(state->config.swapchainBuffering);
	PANIC(!state->renderer.uniformBuffers || !state->renderer.uniformBuffersMemory, "Failed To Allocate Uniform Buffer Memory");
	for (size_t i = 0; i < state->config.swapchainBuffering; i++) {
		createBuffer(state, bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, state->renderer.uniformBuffers[i], state->renderer.uniformBuffersMemory[i]);

		vkMapMemory(state->context.device, state->renderer.uniformBuffersMemory[i], 0, bufferSize, 0, &state->renderer.uniformBuffersMapped[i]);
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
	memcpy(state->renderer.uniformBuffersMapped[state->renderer.imageAquiredIndex], &ubo, sizeof(ubo));
};
void uniformBuffersDestroy(State* state) {
	for (size_t i = 0; i < state->config.swapchainBuffering; i++) {
		vkDestroyBuffer(state->context.device, state->renderer.uniformBuffers[i], nullptr);
		vkFreeMemory(state->context.device, state->renderer.uniformBuffersMemory[i], nullptr);
	};
};

void descriptorPoolCreate(State* state) {
	VkDescriptorPoolSize poolSize{};
	poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSize.descriptorCount = static_cast<uint32_t>(state->config.swapchainBuffering);
	
	VkDescriptorPoolCreateInfo poolInfo{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.maxSets = static_cast<uint32_t>(state->config.swapchainBuffering),
		.poolSizeCount = 1,
		.pPoolSizes = &poolSize,
	};
	vkCreateDescriptorPool(state->context.device, &poolInfo, nullptr, &state->renderer.descriptorPool);
};
void descriptorSetsCreate(State* state) {
	std::vector<VkDescriptorSetLayout> layouts(state->config.swapchainBuffering, state->renderer.descriptorSetLayout);
	VkDescriptorSetAllocateInfo allocInfo{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.descriptorPool = state->renderer.descriptorPool,
		.descriptorSetCount = static_cast<uint32_t>(state->config.swapchainBuffering),
		.pSetLayouts = layouts.data(),
	};
	state->renderer.descriptorSets = (VkDescriptorSet*)malloc(state->config.swapchainBuffering * sizeof(VkDescriptorSet));
	PANIC(!state->renderer.descriptorSets, "Failed to Allocate DescriptorSets Memory")
	PANIC(vkAllocateDescriptorSets(state->context.device, &allocInfo, state->renderer.descriptorSets), "Failed To Allocate Descriptor Set Memory");

	for (size_t i = 0; i < state->config.swapchainBuffering; i++) {
		VkDescriptorBufferInfo bufferInfo{
			.buffer = state->renderer.uniformBuffers[i],
			.offset = 0,
			.range = sizeof(UniformBufferObject),
		};
		VkWriteDescriptorSet descriptorWrite{
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = state->renderer.descriptorSets[i],
			.dstBinding = 0,
			.dstArrayElement = 0,
			.descriptorCount = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.pImageInfo = nullptr, // Optional
			.pBufferInfo = &bufferInfo,
			.pTexelBufferView = nullptr, // Optional
		};
		vkUpdateDescriptorSets(state->context.device, 1, &descriptorWrite, 0, nullptr);

	};
};
void descriptorPoolDestroy(State* state) {
	vkDestroyDescriptorPool(state->context.device, state->renderer.descriptorPool, nullptr);
};


void commandBufferGet(State* state) {
	VkCommandBufferAllocateInfo allocInfo{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.commandPool = state->renderer.commandPool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = state->config.swapchainBuffering,
	};
	state->renderer.commandBuffer = (VkCommandBuffer*)malloc(state->config.swapchainBuffering * sizeof(VkCommandBuffer));
	PANIC(vkAllocateCommandBuffers(state->context.device, &allocInfo, state->renderer.commandBuffer), "Failed To Create Command Buffer");
};
void commandBufferRecord(State* state) {
	VkCommandBufferBeginInfo beginInfo{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
	};
	vkBeginCommandBuffer(state->renderer.commandBuffer[state->renderer.frameIndex], &beginInfo);
	VkClearValue clearColor = state->config.backgroundColor;
	VkRenderPassBeginInfo renderPassInfo{
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.renderPass = state->renderer.renderPass,
		.framebuffer = state->renderer.framebuffers[state->renderer.imageAquiredIndex],
		.clearValueCount = 1,
		.pClearValues = &clearColor
	};
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = state->window.swapchain.imageExtent;
	vkCmdBeginRenderPass(state->renderer.commandBuffer[state->renderer.frameIndex], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
	vkCmdBindPipeline(state->renderer.commandBuffer[state->renderer.frameIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, state->renderer.graphicsPipeline);

	VkBuffer vertexBuffers[] = { state->renderer.vertexBuffer };
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(state->renderer.commandBuffer[state->renderer.frameIndex], 0, 1, vertexBuffers, offsets);
	vkCmdBindIndexBuffer(state->renderer.commandBuffer[state->renderer.frameIndex], state->renderer.indexBuffer, 0, VK_INDEX_TYPE_UINT16);

	VkViewport viewport{
		.x = 0.0f,
		.y = 0.0f,
		.width = (float)state->window.swapchain.imageExtent.width,
		.height = (float)state->window.swapchain.imageExtent.height,
		.minDepth = 0.0f,
		.maxDepth = 1.0f,
	};
	vkCmdSetViewport(state->renderer.commandBuffer[state->renderer.frameIndex], 0, 1, &viewport);
	vkCmdBindDescriptorSets(state->renderer.commandBuffer[state->renderer.frameIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, state->renderer.pipelineLayout, 0, 1, &state->renderer.descriptorSets[state->renderer.frameIndex], 0, nullptr);
	VkRect2D scissor{
		.offset = { 0, 0 },
		.extent = state->window.swapchain.imageExtent,
	};
	vkCmdSetScissor(state->renderer.commandBuffer[state->renderer.frameIndex], 0, 1, &scissor);
	vkCmdDrawIndexed(state->renderer.commandBuffer[state->renderer.frameIndex], static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
	vkCmdEndRenderPass(state->renderer.commandBuffer[state->renderer.frameIndex]);
	PANIC(vkEndCommandBuffer(state->renderer.commandBuffer[state->renderer.frameIndex]), "Failed To Record Command Buffer");
};
		
void syncObjectsCreate(State* state) {
	VkSemaphoreCreateInfo semaphoreInfo{
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
	};
	VkFenceCreateInfo fenceInfo{
		.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		.flags = VK_FENCE_CREATE_SIGNALED_BIT
	};
	state->renderer.imageAvailableSemaphore = (VkSemaphore*)malloc(state->config.swapchainBuffering * sizeof(VkSemaphore));
	state->renderer.renderFinishedSemaphore = (VkSemaphore*)malloc(state->config.swapchainBuffering * sizeof(VkSemaphore));
	state->renderer.inFlightFence = (VkFence*)malloc(state->config.swapchainBuffering * sizeof(VkFence));
	PANIC(!state->renderer.imageAvailableSemaphore || !state->renderer.renderFinishedSemaphore || !state->renderer.inFlightFence, "Failed To Allocate Semaphore Memory");
	for (int i = 0; i < (int)state->config.swapchainBuffering; i++) {
		PANIC((vkCreateSemaphore(state->context.device, &semaphoreInfo, nullptr, &state->renderer.imageAvailableSemaphore[i])
			|| vkCreateSemaphore(state->context.device, &semaphoreInfo, nullptr, &state->renderer.renderFinishedSemaphore[i]))
			||vkCreateFence(state->context.device, &fenceInfo, nullptr, &state->renderer.inFlightFence[i]),
			"Failed To Create Image Aquired Semaphore");
	};
};
void syncObjectsDestroy(State* state) {
	for (int i = 0; i < (int)state->window.swapchain.imageCount; i++) {
		vkDestroySemaphore(state->context.device, state->renderer.imageAvailableSemaphore[i], nullptr);
		vkDestroySemaphore(state->context.device, state->renderer.renderFinishedSemaphore[i], nullptr);
		vkDestroyFence(state->context.device, state->renderer.inFlightFence[i], nullptr);
	};
};

