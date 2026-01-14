#pragma once
#include "stateMachine.h"
//Utility
uint32_t findMemoryType(State* state, VkMemoryRequirements memRequirements, VkMemoryPropertyFlags properties);
//Buffers
void createBuffer(State* state, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);

VkCommandBuffer beginSingleTimeCommands(State* state);
void endSingleTimeCommands(State* state, VkCommandBuffer commandBuffer);
void copyBuffer(State* state, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

void frameBuffersCreate(State* state);
void frameBuffersDestroy(State* state);

void vertexBufferCreate(State* state);
void vertexBufferDestroy(State* state);

void indexBufferCreate(State* state);
void indexBufferDestroy(State* state);

void uniformBuffersCreate(State* state);
void uniformBuffersUpdate(State* state);
void uniformBuffersDestroy(State* state);

void commandBufferGet(State* state);
void commandBufferRecord(State* state);
