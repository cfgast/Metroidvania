#version 450

layout(location = 0) in vec2 aPos;

layout(push_constant) uniform PushConstants {
    mat4 uProjection;
    mat4 uModel;
    vec4 uColor;
} pc;

layout(location = 0) out vec2 vWorldPos;

void main()
{
    vec4 worldPos = pc.uModel * vec4(aPos, 0.0, 1.0);
    vWorldPos = worldPos.xy;
    gl_Position = pc.uProjection * worldPos;
}
