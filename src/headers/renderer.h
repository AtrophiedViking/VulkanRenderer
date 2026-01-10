#include "stateMachine.h"
#include "fstream"
#include "vector"
//Graphics Pipeline
void renderPassCreate(State* state);
void renderPassDestroy(State* state);

void graphicsPipelineCreate(State* state);
void graphicsPipelineDestroy(State* state);

void frameBuffersCreate(State* state);
void frameBuffersDestroy(State* state);

void commandPoolCreate(State* state);
void commandPoolDestroy(State* state);

void commandBufferGet(State* state);
void commandBufferRecord(State* state);

void syncObjectsCreate(State* state);
void syncObjectsDestroy(State* state);

void frameDraw(State* state);