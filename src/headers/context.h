#include "stateMachine.h"

void instanceCreate(State* state);
void instanceDestroy(State* state);

bool deviceSuitabilityCheck(VkPhysicalDevice device);
void physicalDeviceSelect(State* state);
void queueFamilySelect(State* state);

void deviceCreate(State* state);
void deviceDestroy(State* state);