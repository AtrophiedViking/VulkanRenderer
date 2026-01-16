#include "buffers.h"
//Utility
VkFormat findSupportedFormat(State *state, const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
VkFormat findDepthFormat(State* state);
bool hasStencilComponent(VkFormat format);
//Textures
void imageCreate(State* state, uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory, uint32_t mipLevels, VkSampleCountFlagBits numSamples);

void transitionImageLayout(State* state, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels);
void copyBufferToImage(State * state, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

void textureImageCreate(State* state);
void textureImageDestroy(State* state);

VkImageView imageViewCreate(State* state, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels);

void textureImageViewCreate(State* state);
void textureImageViewDestroy(State* state);

void textureSamplerCreate(State* state);
void textureSamplerDestroy(State* state);

void colorResourceCreate(State* state);
void colorResourceDestroy(State* state);

void depthResourceCreate(State* state);
void depthBufferDestroy(State* state);

void generateMipmaps(State* state, VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels);