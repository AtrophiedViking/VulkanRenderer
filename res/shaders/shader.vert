#version 450

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;

    vec4 lightPositions[4];  // Position and radius
    vec4 lightColors[4];     // RGB color and intensity
    vec4 camPos;            // Camera position for view-dependent effects

    float exposure;         // Exposure for HDR rendering
    float gamma;            // Gamma correction value
    float prefilteredCubeMipLevels;  // For image-based lighting
    float scaleIBLAmbient; 
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;

void main() {
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0);
    fragColor = inColor;
    fragTexCoord = inTexCoord;
}