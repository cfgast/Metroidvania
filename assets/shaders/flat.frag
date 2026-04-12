#version 450

#include "lighting.glsl"

layout(location = 0) in vec2 vWorldPos;

layout(push_constant) uniform PushConstants {
    mat4 uProjection;
    mat4 uModel;
    vec4 uColor;
} pc;

layout(location = 0) out vec4 FragColor;

void main()
{
    vec3 normal = vec3(0.0, 0.0, 1.0);
    vec3 lighting = calcLighting(vWorldPos, normal);
    FragColor = vec4(pc.uColor.rgb * lighting, pc.uColor.a);
}
