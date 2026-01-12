#include "textures.h"
#include "buffers.h"
#include "fstream"
#include "vector"
//Graphics Pipeline
void renderPassCreate(State* state);
void renderPassDestroy(State* state);

void descriptorSetLayoutCreate(State* state);
void descriptorSetLayoutDestroy(State* state);

void graphicsPipelineCreate(State* state);
void graphicsPipelineDestroy(State* state);

void commandPoolCreate(State* state);
void commandPoolDestroy(State* state);

void descriptorPoolCreate(State* state);
void descriptorSetsCreate(State* state);
void descriptorPoolDestroy(State* state);

void syncObjectsCreate(State* state);
void syncObjectsDestroy(State* state);

