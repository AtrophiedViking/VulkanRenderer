#pragma once
#include "stateMachine.h"
#include "models.h"
#include "gui.h"
//Utility
uint32_t findMemoryType(State* state, VkMemoryRequirements memRequirements, VkMemoryPropertyFlags properties);
//Buffers
void createBuffer(State* state, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);

VkCommandBuffer beginSingleTimeCommands(State* state, VkCommandPool commandPool);
void endSingleTimeCommands(State* state, VkCommandBuffer commandBuffer);
void copyBuffer(State* state, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

void frameBuffersCreate(State* state);
void frameBuffersDestroy(State* state);

void vertexBufferCreateForMesh(State* state, const std::vector<Vertex>& vertices, VkBuffer& vertexBuffer, VkDeviceMemory& vertexMemory);
void vertexBufferDestroy(State* state);

void indexBufferCreateForMesh(State* state, const std::vector<uint32_t>& indices, VkBuffer& indexBuffer, VkDeviceMemory& indexMemory);
void indexBufferDestroy(State* state);

void uniformBuffersCreate(State* state);
void uniformBuffersUpdate(State* state);
void uniformBuffersDestroy(State* state);

void commandBufferGet(State* state);
void commandBufferRecord(State* state);
