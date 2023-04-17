#include "include/lights.h"
#include "include/pbr.h"

layout(location = 0) in vec2 inUv;
layout(location = 1) in vec3 inCameraPos;

layout(location = 0) out vec4 outFrag;

// light types - using specilisation constants to make
// sure these line up with the enum used on the host.
layout(constant_id = 0) const int LightTypePoint = 0;
layout(constant_id = 1) const int LightTypeSpot = 1;
layout(constant_id = 2) const int LightTypeDir = 2;
// If we hit this - then we have reached the end of the
// viable ights in the storage buffer.
layout(constant_id = 3) const int BufferEndSignal = 255;

void main()
{
    vec3 inPos = texture(PositionSampler, inUv).rgb;
    vec3 baseColour = texture(BaseColourSampler, inUv).rgb;
    float applyLightingFlag = texture(EmissiveSampler, inUv).a;

    // if lighting isn't applied to this fragment then
    // exit early.
    if (applyLightingFlag == 0.0)
    {
        outFrag = vec4(baseColour, 1.0);
        return;
    }

    vec3 V = normalize(inCameraPos.xyz - inPos);
    vec3 N = texture(NormalSampler, inUv).rgb;
    vec3 R = normalize(-reflect(V, N));

    // get pbr information from G-buffer
    float metallic = texture(PbrSampler, inUv).x;
    float roughness = texture(PbrSampler, inUv).y;
    float occlusion = texture(BaseColourSampler, inUv).a;
    vec3 emissive = texture(EmissiveSampler, inUv).rgb;

    vec3 F0 = vec3(0.04);
    vec3 specularColour = mix(F0, baseColour, metallic);

    float reflectance =
        max(max(specularColour.r, specularColour.g), specularColour.b);
    float reflectance90 =
        clamp(reflectance * 25.0, 0.0, 1.0); // 25.0-50.0 is used
    vec3 specReflectance = specularColour.rgb;
    vec3 specReflectance90 = vec3(1.0, 1.0, 1.0) * reflectance90;

    float alphaRoughness = roughness * roughness;

    // apply additional lighting contribution to specular
    vec3 colour = baseColour;

    for (int idx = 0; idx < 2000; idx++)
    {
        LightParams params = light_ssbo.params[idx];
        if (params.lightType == BufferEndSignal)
        {
            break;
        }

        if (params.lightType == LightTypePoint ||
            params.lightType == LightTypeSpot)
        {
            vec3 lightPos = params.pos.xyz - inPos;
            vec3 L = normalize(lightPos);
            float intensity = params.colour.a;

            float attenuation = calculateDistance(lightPos, params.fallOut);
            if (params.lightType == LightTypeSpot)
            {
                attenuation *= calculateAngle(
                    params.direction.xyz, L, params.scale, params.offset);
            }

            colour += specularContribution(
                L,
                V,
                N,
                baseColour,
                metallic,
                alphaRoughness,
                attenuation,
                intensity,
                params.colour.rgb,
                specReflectance,
                specReflectance90);
        }
        else if (params.lightType == LightTypeDir)
        {
            vec3 L = calculateSunArea(params.direction.xyz, params.pos.xyz, R);
            float intensity = params.colour.a;
            float attenuation = 1.0f;
            colour += specularContribution(
                L,
                V,
                N,
                baseColour,
                metallic,
                alphaRoughness,
                attenuation,
                intensity,
                params.colour.rgb,
                specReflectance,
                specReflectance90);
        }
    }

    // occlusion
    vec3 finalColour = mix(colour, colour * occlusion, 1.0);

    // emissive
    finalColour += emissive;

    outFrag = vec4(finalColour, 1.0);
}
