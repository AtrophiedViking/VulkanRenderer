#include "context.h"
#include "renderer.h"


//Error Handling
void logPrint(State* state);
void exitCallback();
void glfwErrorCallback(int errorCode, const char* description);
void errorHandlingSetup(State* state);

//Window
void initGLFW(State* state);

void surfaceCreate(State* state);
void surfaceDestroy(State* state);

void windowCreate(State* state);
void windowDestroy(State* state);

//Swapchain

VkSurfaceCapabilitiesKHR surfaceCapabilitiesGet(State *state);
VkSurfaceFormatKHR surfaceFormatSelect(State* state);
VkPresentModeKHR presentModeSelect(State* state);

void swapchainImageGet(State* state);
void imageViewsCreate(State* state);
void imageViewsDestroy(State* state);

void swapchainCreate(State* state);
void swapchainDestroy(State* state);

void swapchainCleanup(State* state);
void swapchainRecreate(State* state);

//Draw

void frameDraw(State* state);

void updateFPS(State* state);
void processInput(State* state);