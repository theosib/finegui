#version 450

/**
 * ImGui fragment shader for finegui
 *
 * Samples texture and multiplies by vertex color.
 */

layout(set = 0, binding = 0) uniform sampler2D texSampler;

layout(location = 0) in vec2 inUV;
layout(location = 1) in vec4 inColor;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = inColor * texture(texSampler, inUV);
}
