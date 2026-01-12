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
	auto vertShaderCode = shaderRead("./res/shaders/vert.spv");
	auto fragShaderCode = shaderRead("./res/shaders/frag.spv");
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
			.buffer = state->buffers.uniformBuffers[i],
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

