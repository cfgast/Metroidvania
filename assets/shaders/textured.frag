#version 450

#include "lighting.glsl"

layout(location = 0) in vec2 vTexCoord;
layout(location = 1) in vec2 vWorldPos;

layout(push_constant) uniform PushConstants {
    mat4 uProjection;
    int  uHasNormalMap;
} pc;

layout(set = 1, binding = 0) uniform sampler2D uTexture;
layout(set = 1, binding = 1) uniform sampler2D uNormalMap;

layout(location = 0) out vec4 FragColor;

void main()
{
    vec4 texColor = texture(uTexture, vTexCoord);

    vec3 normal;
    if (pc.uHasNormalMap != 0)
    {
        normal = texture(uNormalMap, vTexCoord).rgb;
        normal = normal * 2.0 - 1.0;
        normal.y = -normal.y;  // flip Y for Y-down coordinate system
        normal = normalize(normal);
    }
    else
    {
        normal = vec3(0.0, 0.0, 1.0);
    }

    vec3 lighting = calcLighting(vWorldPos, normal);
    FragColor = vec4(texColor.rgb * lighting, texColor.a);
}
