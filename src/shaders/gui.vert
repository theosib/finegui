#version 450

/**
 * ImGui vertex shader for finegui
 *
 * Transforms 2D screen-space vertices using push constants for scale/translate.
 */

layout(push_constant) uniform PushConstants {
    vec2 scale;      // 2.0 / displaySize
    vec2 translate;  // -1.0
} pc;

layout(location = 0) in vec2 inPos;
layout(location = 1) in vec2 inUV;
layout(location = 2) in vec4 inColor;  // R8G8B8A8_UNORM unpacked by Vulkan

layout(location = 0) out vec2 outUV;
layout(location = 1) out vec4 outColor;

void main() {
    outUV = inUV;
    outColor = inColor;
    gl_Position = vec4(inPos * pc.scale + pc.translate, 0.0, 1.0);
}
