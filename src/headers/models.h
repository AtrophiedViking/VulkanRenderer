#include "textures.h"


void modelLoad(State* state, std::string modelPath);
void modelUnload(State* state);

void createTextureDescriptorSets(State* state);

void drawMesh(State* state, VkCommandBuffer cmd, const Mesh& mesh);

void drawNode(State* state, VkCommandBuffer cmd, const Node* node);

void gatherDrawItems(const Node* root, const glm::vec3& camPos, const std::vector<Material>& materials, std::vector<DrawItem>& out);
