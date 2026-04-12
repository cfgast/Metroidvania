#version 450

layout(location = 0) in vec2 vTexCoord;

layout(set = 1, binding = 0) uniform sampler2D uScreen;

layout(location = 0) out vec4 FragColor;

void main()
{
    FragColor = texture(uScreen, vTexCoord);
}
