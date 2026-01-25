#include "headers/scene.h"
void gameObjectsCreate(State* state) {
    // Object 1 - Center
    state->scene.gameObjects[0].position = { 0.0f, 0.0f, 0.0f };
    state->scene.gameObjects[0].rotation = { 0.0f, 0.0f, 0.0f };
    state->scene.gameObjects[0].scale = { 1.0f, 1.0f, 1.0f };

    // Object 2 - Left
    state->scene.gameObjects[1].position = { -2.0f, 0.0f, -1.0f };
    state->scene.gameObjects[1].rotation = { 0.0f, glm::radians(45.0f), 0.0f };
    state->scene.gameObjects[1].scale = { 0.75f, 0.75f, 0.75f };

    // Object 3 - Right
    state->scene.gameObjects[2].position = { 2.0f, 0.0f, -1.0f };
    state->scene.gameObjects[2].rotation = { 0.0f, glm::radians(-45.0f), 0.0f };
    state->scene.gameObjects[2].scale = { 0.75f, 0.75f, 0.75f };
}