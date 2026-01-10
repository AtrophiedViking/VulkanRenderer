#include "stateMachine.h"
#include "fstream"
#include "vector"
//Graphics Pipeline
void renderPassCreate(State* state);
void renderPassDestroy(State* state);

void descriptorSetLayoutCreate(State* state);
void descriptorSetLayoutDestroy(State* state);

void graphicsPipelineCreate(State* state);
void graphicsPipelineDestroy(State* state);

void frameBuffersCreate(State* state);
void frameBuffersDestroy(State* state);

void commandPoolCreate(State* state);
void commandPoolDestroy(State* state);

void createBuffer(State* state, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
void copyBuffer(State* state, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

void vertexBufferCreate(State* state);
void vertexBufferDestroy(State* state);

void indexBufferCreate(State* state);
void indexBufferDestroy(State* state);

void uniformBuffersCreate(State* state);
void uniformBuffersUpdate(State* state);
void uniformBuffersDestroy(State* state);

void descriptorPoolCreate(State* state);
void descriptorSetsCreate(State* state);
void descriptorPoolDestroy(State* state);

void commandBufferGet(State* state);
void commandBufferRecord(State* state);

void syncObjectsCreate(State* state);
void syncObjectsDestroy(State* state);

