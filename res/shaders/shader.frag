#version 450

// ─────────────────────────────────────────────
// Push Constants
// ─────────────────────────────────────────────
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

// ─────────────────────────────────────────────
// UBO (binding = 0)
// ─────────────────────────────────────────────
layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;

    vec4 lightPositions[4];
    vec4 lightColors[4];
    vec4 camPos;

    float exposure;
    float gamma;
    float prefilteredCubeMipLevels;
    float scaleIBLAmbient;
} ubo;

// ─────────────────────────────────────────────
// Textures (set = 0, bindings 1–5)
// ─────────────────────────────────────────────
layout(set = 1, binding = 0) uniform sampler2D baseColorTex;
layout(set = 1, binding = 1) uniform sampler2D metallicRoughnessTex;
layout(set = 1, binding = 2) uniform sampler2D occlusionTex;
layout(set = 1, binding = 3) uniform sampler2D emissiveTex;
layout(set = 1, binding = 4) uniform sampler2D normalTex;

// ─────────────────────────────────────────────
// Inputs from vertex shader
// ─────────────────────────────────────────────
layout(location = 0) in vec3 fragColorVS;
layout(location = 1) in vec2 fragTexCoordVS;

// ─────────────────────────────────────────────
// Outputs
// ─────────────────────────────────────────────
layout(location = 0) out vec4 outColor;

// ─────────────────────────────────────────────
// Helper: choose UV set (your VS only provides 1)
// ─────────────────────────────────────────────
vec2 getUV(int setIndex)
{
    // Your vertex shader only outputs ONE UV set.
    // So all texture sets map to the same UV.
    return fragTexCoordVS;
}

// ─────────────────────────────────────────────
// Main
// ─────────────────────────────────────────────
void main()
{
    // Base color sampling
    vec2 uvBase = getUV(pc.baseColorTextureSet);
    vec4 baseColorTexSample = texture(baseColorTex, uvBase);

    vec4 baseColor = baseColorTexSample * pc.baseColorFactor;

    // Alpha masking
    if (pc.alphaMask > 0.5) {
        if (baseColor.a < pc.alphaMaskCutoff) {
            discard;
        }
    }

    // Metallic-roughness sampling
    vec2 uvMR = getUV(pc.physicalDescriptorTextureSet);
    vec3 mrSample = texture(metallicRoughnessTex, uvMR).rgb;

    float metallic = mrSample.b * pc.metallicFactor;
    float roughness = mrSample.g * pc.roughnessFactor;

    // Occlusion
    vec2 uvOcc = getUV(pc.occlusionTextureSet);
    float occlusion = texture(occlusionTex, uvOcc).r;

    // Emissive
    vec2 uvEm = getUV(pc.emissiveTextureSet);
    vec3 emissive = texture(emissiveTex, uvEm).rgb;

    // Simple lighting: just modulate vertex color + base color
    vec3 color = fragColorVS * baseColor.rgb * baseColor.a;

    // Add emissive
    color += emissive;

    // Apply occlusion
    color *= occlusion;

    // Gamma correction
    color = pow(color * baseColor.a, vec3(1.0 / ubo.gamma));


    vec4 sampledColor = texture(color, fragTexCoordVS);
    
    // Apply vertex color and alpha
    vec3 finalRGB = sampledColor.rgb * fragColorVS;
    outColor = vec4(finalRGB, sampledColor.a);
}
