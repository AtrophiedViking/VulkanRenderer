#include "buffers.h"

void imageCreate(State* state, uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);

void transitionImageLayout(State* state, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
	void copyBufferToImage(State * state, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

void textureImageCreate(State* state);
void textureImageDestroy(State* state);

VkImageView imageViewCreate(State *state, VkImage image, VkFormat format);

void textureImageViewCreate(State* state);
void textureImageViewDestroy(State* state);

void textureSamplerCreate(State* state);
void textureSamplerDestroy(State* state);