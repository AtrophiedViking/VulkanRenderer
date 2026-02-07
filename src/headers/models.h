#include "textures.h"


void modelLoad(State* state, std::string modelPath);
void drawMesh(State* state, VkCommandBuffer cmd, const Mesh& mesh);

void drawNode(State* state, VkCommandBuffer cmd, const Node* node);