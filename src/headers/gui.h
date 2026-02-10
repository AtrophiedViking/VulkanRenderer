#include "stateMachine.h"
#include "buffers.h"

void guiDescriptorPoolCreate(State* state);

void guiRenderPassCreate(State* state);
void guiRenderPassDestroy(State * state);

void guiFramebuffersCreate(State* state);
void guiFramebuffersDestroy(State* state);

void guiInit(State* state);
void guiDraw(State* state, VkCommandBuffer cmd);
void guiClean(State* state);