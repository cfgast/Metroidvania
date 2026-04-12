#version 450

layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aTexCoord;

layout(push_constant) uniform PushConstants {
    mat4 uProjection;
} pc;

layout(location = 0) out vec2 vTexCoord;

void main()
{
    gl_Position = pc.uProjection * vec4(aPos, 0.0, 1.0);
    vTexCoord = aTexCoord;
}
