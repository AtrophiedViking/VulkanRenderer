#include "textures.h"


Model* modelLoad(State* state, std::string modelPath);
void modelUnload(State* state);

void drawMesh(State* state,
    VkCommandBuffer cmd,
    const Mesh& mesh,
    const glm::mat4& nodeMatrix,
    const glm::mat4& modelTransform);

void drawNode(State* state, VkCommandBuffer cmd, const Node* node);

void gatherDrawItems(const Node* root, const glm::vec3& camPos, const std::vector<Material>& materials, std::vector<DrawItem>& out);
