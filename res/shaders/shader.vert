#version 450

layout(push_constant) uniform PushConstants {
    vec4 baseColorFactor;
    float metallicFactor;
    float roughnessFactor;
    int baseColorTextureSet;
    int physicalDescriptorTextureSet;
    int normalTextureSet;
    int occlusionTextureSet;
    int emissiveTextureSet;
    float alphaMask;
    float alphaMaskCutoff;
} pc;

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
layout(location = 3) in vec3 inNormal;
layout(location = 4) in vec4 inTangent;


void main() {
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0);
    fragColor = inColor;
    fragTexCoord = inTexCoord;
}