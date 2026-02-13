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
		.samples = state->config.msaaSamples,
		.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
	};

	VkAttachmentReference colorAttachmentRef{
		.attachment = 0,
		.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
	};

	VkAttachmentDescription depthAttachment{};
	depthAttachment.format = findDepthFormat(state);
	depthAttachment.samples = state->config.msaaSamples;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	VkAttachmentReference depthAttachmentRef{};
	depthAttachmentRef.attachment = 1;
	depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentDescription colorAttachmentResolve{};
	colorAttachmentResolve.format = state->window.swapchain.format;
	colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	
	VkAttachmentReference colorAttachmentResolveRef{};
	colorAttachmentResolveRef.attachment = 2;
	colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass{
		.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
		.colorAttachmentCount = 1,
		.pColorAttachments = &colorAttachmentRef,
		.pResolveAttachments = &colorAttachmentResolveRef,
		.pDepthStencilAttachment = &depthAttachmentRef,

	};
	VkSubpassDependency dependency{
		.srcSubpass = VK_SUBPASS_EXTERNAL,
		.dstSubpass = 0,
		.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
		.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
		.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
		.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
	};
	
	std::array<VkAttachmentDescription, 3> attachments = { colorAttachment, depthAttachment, colorAttachmentResolve };
	VkRenderPassCreateInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	renderPassInfo.pAttachments = attachments.data();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;
	PANIC(vkCreateRenderPass(state->context.device, &renderPassInfo, nullptr, &state->renderer.renderPass), "Failed To Create Render Pass")
};
void renderPassDestroy(State* state) {
	vkDestroyRenderPass(state->context.device, state->renderer.renderPass, nullptr);
};

// set 0: global UBO
void createGlobalSetLayout(State* state) {
	VkDescriptorSetLayoutBinding uboBinding{
		.binding = 0,
		.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
		.pImmutableSamplers = nullptr
	};

	VkDescriptorSetLayoutCreateInfo info{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.bindingCount = 1,
		.pBindings = &uboBinding
	};

	PANIC(
		vkCreateDescriptorSetLayout(state->context.device, &info, nullptr, &state->renderer.descriptorSetLayout),
		"Failed to create global UBO set layout"
	);
}

// set 1: texture (for now, just baseColor at binding 0)
void createTextureSetLayout(State* state) {
	std::array<VkDescriptorSetLayoutBinding, 6> bindings{};

	// binding 0 — baseColor texture
	bindings[0] = {
		.binding = 0,
		.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
	};

	// binding 1 — metallicRoughness texture
	bindings[1] = {
		.binding = 1,
		.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
	};

	// binding 2 — occlusion texture
	bindings[2] = {
		.binding = 2,
		.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
	};

	// binding 3 — emissive texture
	bindings[3] = {
		.binding = 3,
		.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
	};

	// binding 4 — normal texture
	bindings[4] = {
		.binding = 4,
		.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
	};

	// binding 5 — NEW: material UBO (TextureTransforms)
	bindings[5] = {
		.binding = 5,
		.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
	};

	VkDescriptorSetLayoutCreateInfo layoutInfo{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.bindingCount = static_cast<uint32_t>(bindings.size()),
		.pBindings = bindings.data()
	};

	PANIC(vkCreateDescriptorSetLayout(state->context.device, &layoutInfo, nullptr, &state->renderer.textureSetLayout),"Failed to create texture set layout");
}



void descriptorSetLayoutDestroy(State* state) {
	vkDestroyDescriptorSetLayout(state->context.device, state->renderer.descriptorSetLayout, nullptr);
	vkDestroyDescriptorSetLayout(state->context.device, state->renderer.textureSetLayout, nullptr);
};

void graphicsPipelineCreate(State* state) {
	//ShaderModules
	auto vertShaderCode = shaderRead("./res/shaders/vert.spv");
	auto fragShaderCode = shaderRead("./res/shaders/frag.spv");
	VkShaderModuleCreateInfo vertShaderModuleInfo{
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.codeSize = vertShaderCode.size(),
		.pCode = reinterpret_cast<const uint32_t*>(vertShaderCode.data()),
	};
	PANIC(vkCreateShaderModule(state->context.device, &vertShaderModuleInfo, nullptr, &state->renderer.vertShaderModule), "Failed To Create Vertex Shader Module");
	VkShaderModuleCreateInfo fragShaderModuleInfo{
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.codeSize = fragShaderCode.size(),
		.pCode = reinterpret_cast<const uint32_t*>(fragShaderCode.data()),
	};
	PANIC(vkCreateShaderModule(state->context.device, &fragShaderModuleInfo, nullptr, &state->renderer.fragShaderModule), "Failed To Create Fragment Shader Module");

	//ShaderStages
	VkPipelineShaderStageCreateInfo vertShaderStageInfo{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.stage = VK_SHADER_STAGE_VERTEX_BIT,
		.module = state->renderer.vertShaderModule,
		.pName = "main"
	};
	VkPipelineShaderStageCreateInfo fragShaderStageInfo{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
		.module = state->renderer.fragShaderModule,
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
	.depthBiasEnable = VK_TRUE,
	.lineWidth = 1.0f,
	};
	//MultiSampling
	VkPipelineMultisampleStateCreateInfo multisampling{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
		.rasterizationSamples = state->config.msaaSamples,
		.sampleShadingEnable = VK_TRUE,
		.minSampleShading = 0.1f, // Optional
		.pSampleMask = nullptr, // Optional
		.alphaToCoverageEnable = VK_FALSE, // Optional
		.alphaToOneEnable = VK_FALSE, // Optional
	};
	//ColorBlending
	VkPipelineColorBlendAttachmentState colorBlendAttachment{
		.blendEnable = VK_TRUE,
		.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
		.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
		.colorBlendOp = VK_BLEND_OP_ADD,
		.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
		.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
		.alphaBlendOp = VK_BLEND_OP_ADD,
		.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
	};
	VkPipelineColorBlendStateCreateInfo colorBlending{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
		.logicOpEnable = VK_FALSE,
		.attachmentCount = 1,
		.pAttachments = &colorBlendAttachment,
	};

	VkPushConstantRange pushConstantRange{
		.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT,
		.offset = 0,
		.size = sizeof(PushConstantBlock),
	};

	VkDescriptorSetLayout setLayouts[] = {
	state->renderer.descriptorSetLayout,   // set = 0
	state->renderer.textureSetLayout   // set = 1
	};
	//PipelineLayout
	VkPipelineLayoutCreateInfo pipelineLayoutInfo{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.setLayoutCount = 2,
		.pSetLayouts = setLayouts,
		.pushConstantRangeCount = 1,
		.pPushConstantRanges = &pushConstantRange
	};
	PANIC(vkCreatePipelineLayout(state->context.device, &pipelineLayoutInfo, nullptr, &state->renderer.pipelineLayout), "Failed To Create Pipeline Layout");
	
	VkPipelineDepthStencilStateCreateInfo depthStencil{};
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable = VK_TRUE;
	depthStencil.depthWriteEnable = VK_TRUE;
	depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
	depthStencil.depthBoundsTestEnable = VK_FALSE;
	depthStencil.stencilTestEnable = VK_FALSE;

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
		.pDepthStencilState = &depthStencil,
		.pColorBlendState = &colorBlending,
		.pDynamicState = &dynamicState,
		.layout = state->renderer.pipelineLayout,
		.renderPass = state->renderer.renderPass,
		.subpass = 0,
		.basePipelineHandle = VK_NULL_HANDLE, // Optional
	};
	PANIC(vkCreateGraphicsPipelines(state->context.device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &state->renderer.graphicsPipeline),"Failed To Create GraphicsPipeline");
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

void descriptorPoolCreate(State* state)
{
	uint32_t frames = state->config.swapchainBuffering;
	uint32_t materialCount = state->scene.materials.size();

	uint32_t imageDescriptorCount = materialCount * 5 * state->renderer.descriptorPoolMultiplier;
	uint32_t uboDescriptorCount = frames * state->renderer.descriptorPoolMultiplier;

	std::array<VkDescriptorPoolSize, 2> poolSizes{
		VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, uboDescriptorCount },
		VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, imageDescriptorCount }
	};

	uint32_t totalSets = (frames + materialCount) * state->renderer.descriptorPoolMultiplier;

	VkDescriptorPoolCreateInfo poolInfo{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.flags = 0,
		.maxSets = totalSets,
		.poolSizeCount = (uint32_t)poolSizes.size(),
		.pPoolSizes = poolSizes.data(),
	};

	PANIC(
		vkCreateDescriptorPool(state->context.device, &poolInfo, nullptr, &state->renderer.descriptorPool),
		"Failed to create descriptor pool!"
	);
}
VkResult allocateDescriptorSetsWithResize(
	State* state,
	const VkDescriptorSetAllocateInfo* allocInfo,
	VkDescriptorSet* sets)
{
	VkResult result = vkAllocateDescriptorSets(state->context.device, allocInfo, sets);

	if (result == VK_ERROR_OUT_OF_POOL_MEMORY || result == VK_ERROR_FRAGMENTED_POOL) {
		// Resize pool
		descriptorPoolDestroy(state);
		descriptorPoolCreate(state); // Recreate with larger size

		// Retry allocation
		result = vkAllocateDescriptorSets(state->context.device, allocInfo, sets);
	}

	return result;
}
void descriptorSetsCreate(State* state)
{
	uint32_t frames = state->config.swapchainBuffering;

	state->renderer.descriptorSets.resize(frames);

	std::vector<VkDescriptorSetLayout> layouts(frames, state->renderer.descriptorSetLayout);

	VkDescriptorSetAllocateInfo allocInfo{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.descriptorPool = state->renderer.descriptorPool,
		.descriptorSetCount = frames,
		.pSetLayouts = layouts.data()
	};

	PANIC(
		vkAllocateDescriptorSets(
			state->context.device,
			&allocInfo,
			state->renderer.descriptorSets.data()
		),
		"Failed to allocate global UBO descriptor sets!"
	);

	for (uint32_t i = 0; i < frames; i++)
	{
		VkDescriptorBufferInfo bufferInfo{
			.buffer = state->renderer.uniformBuffers[i],
			.offset = 0,
			.range = sizeof(UniformBufferObject)
		};

		VkWriteDescriptorSet writeUBO{
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = state->renderer.descriptorSets[i],
			.dstBinding = 0,
			.descriptorCount = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.pBufferInfo = &bufferInfo
		};

		vkUpdateDescriptorSets(
			state->context.device,
			1,
			&writeUBO,
			0,
			nullptr
		);
	}
}

TexTransformGPU toGPU(const TextureTransform& t)
{
    TexTransformGPU r{};
    r.offset_scale   = glm::vec4(t.offset.x, t.offset.y,
                                 t.scale.x,  t.scale.y);
    r.rot_center_tex = glm::vec4(t.rotation,
                                 t.center.x,
                                 t.center.y,
                                 float(t.texCoord));
    return r;
}


void createBufferForMaterial(
	State* state,
	VkDeviceSize size,
	VkBufferUsageFlags usage,
	VkBuffer* outBuffer,
	VkDeviceMemory* outMemory)
{
	// This is identical to how you create your UBO buffer
	createBuffer(
		state,
		size,
		usage | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		*outBuffer,
		*outMemory
	);
}
void createMaterialDescriptorSets(State* state)
{
	uint32_t materialCount = state->scene.materials.size();
	if (materialCount == 0) return;

	for (Material& mat : state->scene.materials)
	{
		VkDescriptorSetAllocateInfo allocInfo{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
			.descriptorPool = state->renderer.descriptorPool,
			.descriptorSetCount = 1,
			.pSetLayouts = &state->renderer.textureSetLayout
		};

		VkResult result = allocateDescriptorSetsWithResize(state, &allocInfo, &mat.descriptorSet);

		PANIC(result, "Failed to allocate material descriptor set");


		auto resolveTex = [&](int index) -> const Texture&
			{
				if (index >= 0 && index < state->scene.textures.size())
					return state->scene.textures[index];
				return state->scene.textures[state->scene.defaultTextureIndex];
			};

		const Texture& baseTex = resolveTex(mat.baseColorTextureIndex);
		const Texture& mrTex = resolveTex(mat.metallicRoughnessTextureIndex);
		const Texture& occTex = resolveTex(mat.occlusionTextureIndex);
		const Texture& emisTex = resolveTex(mat.emissiveTextureIndex);
		const Texture& normTex = resolveTex(mat.normalTextureIndex);
		
		// Create/fill MaterialGPU
		MaterialGPU gpu{};
		gpu.baseColorTT = toGPU(mat.baseColorTransform);
		gpu.mrTT = toGPU(mat.metallicRoughnessTransform);
		gpu.normalTT = toGPU(mat.normalTransform);
		gpu.occlusionTT = toGPU(mat.occlusionTransform);
		gpu.emissiveTT = toGPU(mat.emissiveTransform);

		// Create a small uniform buffer for this material (you already have helpers for UBOs)
		createBufferForMaterial(state, sizeof(MaterialGPU),
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			&mat.materialBuffer, &mat.materialMemory);

		void* data = nullptr;
		vkMapMemory(state->context.device, mat.materialMemory, 0, sizeof(MaterialGPU), 0, &data);
		memcpy(data, &gpu, sizeof(MaterialGPU));
		vkUnmapMemory(state->context.device, mat.materialMemory);

		VkDescriptorBufferInfo materialBufInfo{
			.buffer = mat.materialBuffer,
			.offset = 0,
			.range = sizeof(MaterialGPU)
		};


		std::array<VkDescriptorImageInfo, 5> infos{
			VkDescriptorImageInfo{ baseTex.textureSampler, baseTex.textureImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL }, // binding 0
			VkDescriptorImageInfo{ mrTex.textureSampler,   mrTex.textureImageView,   VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL }, // binding 1
			VkDescriptorImageInfo{ occTex.textureSampler,  occTex.textureImageView,  VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL }, // binding 2
			VkDescriptorImageInfo{ emisTex.textureSampler, emisTex.textureImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL }, // binding 3
			VkDescriptorImageInfo{ normTex.textureSampler, normTex.textureImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL }  // binding 4
		};

		std::array<VkWriteDescriptorSet, 6> writes{};
		for (uint32_t i = 0; i < 5; ++i) {
			writes[i] = {
				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.dstSet = mat.descriptorSet,
				.dstBinding = i,
				.descriptorCount = 1,
				.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				.pImageInfo = &infos[i]
			};
		}

		// binding 5: material UBO
		writes[5] = {
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = mat.descriptorSet,
			.dstBinding = 5,
			.descriptorCount = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.pBufferInfo = &materialBufInfo
		};

		vkUpdateDescriptorSets(state->context.device,
			static_cast<uint32_t>(writes.size()), writes.data(),
			0, nullptr);
	}

}

void descriptorPoolDestroy(State* state)
{
	// 1. Destroy per-material UBOs
	for (Material& mat : state->scene.materials)
	{
		if (mat.materialBuffer != VK_NULL_HANDLE)
		{
			vkDestroyBuffer(state->context.device, mat.materialBuffer, nullptr);
			mat.materialBuffer = VK_NULL_HANDLE;
		}

		if (mat.materialMemory != VK_NULL_HANDLE)
		{
			vkFreeMemory(state->context.device, mat.materialMemory, nullptr);
			mat.materialMemory = VK_NULL_HANDLE;
		}
	}

	// 2. Destroy descriptor pool
	if (state->renderer.descriptorPool != VK_NULL_HANDLE)
	{
		vkDestroyDescriptorPool(state->context.device,
			state->renderer.descriptorPool,
			nullptr);
		state->renderer.descriptorPool = VK_NULL_HANDLE;
	}
}

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

