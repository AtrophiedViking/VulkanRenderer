#include "headers/graphicsPipeline.h"

void tranparencyDescriptorPoolCreate (State* state) {
	VkDescriptorPoolSize poolSize{};
	poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSize.descriptorCount = 100; // Adjust as needed
	VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = 1;
	poolInfo.pPoolSizes = &poolSize;
	poolInfo.maxSets = 100; // Adjust as needed
	if (vkCreateDescriptorPool(state->context.device, &poolInfo,
		nullptr, &state->renderer.transparencyDescriptorPool) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create transparency descriptor pool");
	}
};