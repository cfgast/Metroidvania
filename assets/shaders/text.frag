#version 450

layout(location = 0) in vec2 vTexCoord;

layout(set = 1, binding = 0) uniform sampler2D uGlyphAtlas;

layout(push_constant) uniform PushConstants {
    mat4 uProjection;
    vec4 uTextColor;
} pc;

layout(location = 0) out vec4 FragColor;

void main()
{
    float alpha = texture(uGlyphAtlas, vTexCoord).r;
    FragColor = vec4(pc.uTextColor.rgb, pc.uTextColor.a * alpha);
}
