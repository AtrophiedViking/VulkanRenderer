#pragma once
#include "stateMachine.h"
//Buffers
void createBuffer(State* state, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
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
