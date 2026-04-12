// Shared lighting library — included by flat.frag and textured.frag
#ifndef LIGHTING_GLSL
#define LIGHTING_GLSL

#define MAX_LIGHTS 32

struct Light {
    vec2  position;
    vec3  color;
    float intensity;
    float radius;
    float z;
    vec2  direction;
    float innerCone;
    float outerCone;
    int   type;   // 0 = point, 1 = spot
};

layout(std140, set = 0, binding = 0) uniform LightingUBO {
    Light uLights[MAX_LIGHTS];
    int   uNumLights;
    vec3  uAmbientColor;
};

vec3 calcLighting(vec2 worldPos, vec3 normal)
{
    vec3 totalLight = uAmbientColor;

    for (int i = 0; i < uNumLights; ++i)
    {
        vec2 toLight2D = uLights[i].position - worldPos;
        float dist2D   = length(toLight2D);

        if (dist2D >= uLights[i].radius)
            continue;

        // 3D light direction (light is above the 2D plane at height z)
        vec3 lightDir = normalize(vec3(toLight2D, uLights[i].z));

        // Quadratic attenuation fading to zero at radius
        float atten = 1.0 - (dist2D / uLights[i].radius);
        atten = atten * atten;

        // Diffuse (Lambertian)
        float diff = max(dot(normal, lightDir), 0.0);

        // Spot light angular falloff
        float spotFactor = 1.0;
        if (uLights[i].type == 1)
        {
            vec2 spotDir = normalize(uLights[i].direction);
            float cosAngle = dot(normalize(-toLight2D), spotDir);
            spotFactor = smoothstep(uLights[i].outerCone, uLights[i].innerCone, cosAngle);
        }

        totalLight += uLights[i].color * uLights[i].intensity * atten * diff * spotFactor;
    }

    return totalLight;
}

#endif
