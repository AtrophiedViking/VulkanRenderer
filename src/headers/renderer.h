#include "graphicsPipeline.h"
#include "scene.h"
#include "fstream"
#include "vector"
//Graphics Pipeline
void renderPassCreate(State* state);
void renderPassDestroy(State* state);

// set 0: global UBO
void createGlobalSetLayout(State* state);
// set 1: texture (for now, just baseColor at binding 0)
void createTextureSetLayout(State* state);
void descriptorSetLayoutDestroy(State* state);

void graphicsPipelineCreate(State* state);
void graphicsPipelineDestroy(State* state);

void commandPoolCreate(State* state);
void commandPoolDestroy(State* state);

void descriptorPoolCreate(State* state);
void descriptorSetsCreate(State* state);
void createMaterialDescriptorSets(State* state);
void descriptorPoolDestroy(State* state);

void syncObjectsCreate(State* state);
void syncObjectsDestroy(State* state);

