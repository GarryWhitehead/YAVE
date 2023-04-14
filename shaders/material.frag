#if defined(HAS_UV_ATTR_INPUT)
layout(location = 0) in vec2 inUv;
#endif
#if defined(HAS_NORMAL_ATTR_INPUT)
layout(location = 1) in vec3 inNormal;
#endif
#if defined(HAS_COLOUR_ATTR_INPUT)
layout(location = 2) in vec4 inColour;
#endif
layout(location = 3) in vec3 inPos;

layout(location = 0) out vec4 outPos;
layout(location = 1) out vec4 outColour;
layout(location = 2) out vec4 outNormal;
layout(location = 3) out vec4 outEmissive;
layout(location = 4) out vec2 outPbr;

#define EPSILON 0.0000001

#if defined(METALLIC_ROUGHNESS_PIPELINE)
float convertMetallic(vec3 diffuse, vec3 specular, float maxSpecular)
{
    float perceivedDiffuse = sqrt(
        0.299 * diffuse.r * diffuse.r + 0.587 * diffuse.g * diffuse.g +
        0.114 * diffuse.b * diffuse.b);
    float perceivedSpecular = sqrt(
        0.299 * specular.r * specular.r + 0.587 * specular.g * specular.g +
        0.114 * specular.b * specular.b);

    if (perceivedSpecular < 0.04)
    {
        return 0.0;
    }

    float a = 0.04;
    float b = perceivedDiffuse * (1.0 - maxSpecular) / (1.0 - 0.04) +
        perceivedSpecular - 2.0 * 0.04;
    float c = 0.04 - perceivedSpecular;
    float D = max(b * b - 4.0 * a * c, 0.0);

    return clamp((-b + sqrt(D)) / (2.0 * a), 0.0, 1.0);
}
#endif

#if defined(HAS_NORMAL_SAMPLER)
// Taken from here:
// http://www.thetenthplanet.de/archives/1180
vec3 peturbNormal(vec2 tex_coord)
{
    // convert normal to -1, 1 coord system
    vec3 tangentNormal = texture(NormalSampler, tex_coord).xyz * 2.0 - 1.0;

    vec3 q1 = dFdx(inPos); // edge1
    vec3 q2 = dFdy(inPos); // edge2
    vec2 st1 = dFdx(tex_coord); // uv1
    vec2 st2 = dFdy(tex_coord); // uv2

    vec3 N = normalize(inNormal);
    vec3 T = normalize(q1 * st2.t - q2 * st1.t);
    vec3 B = -normalize(cross(N, T));
    mat3 TBN = mat3(T, B, N);

    return normalize(TBN * tangentNormal);
}
#endif

void main()
{
    // albedo
    vec4 baseColour = vec4(1.0);
    float alphaMask = 1.0;

#if defined(HAS_ALPHA_MASK)
    alphaMask = material_ubo.alphaMask;
#endif

    if (alphaMask == 1.0)
    {
#if defined(HAS_BASECOLOUR_SAMPLER) && defined(HAS_UV_ATTR_INPUT) &&           \
    !defined(HAS_BASECOLOUR_FACTOR)
        baseColour = texture(BaseColourSampler, inUv);
#elif defined(HAS_BASECOLOUR_SAMPLER) && defined(HAS_UV_ATTR_INPUT) &&         \
    defined(HAS_BASECOLOUR_FACTOR)
        baseColour =
            texture(BaseColourSampler, inUv) * material_ubo.baseColourFactor;
#elif defined(HAS_BASECOLOUR_FACTOR)
        baseColour = material_ubo.baseColourFactor;
#endif
#if defined(HAS_ALPHA_MASK) && defined(HAS_APLHA_MASK_CUTOFF)
        if (baseColour.a < material_ubo.alphaMaskCutOff)
        {
            discard;
        }
#endif
    }

    // default values for output attachments
    vec3 normal = vec3(0.0);
    float roughness = 1.0;
    float metallic = 1.0;

#if defined(HAS_NORMAL_SAMPLER) && defined(HAS_UV_ATTR_INPUT)
    normal = peturbNormal(inUv);
#elif defined(HAS_NORMAL_ATTR_INPUT)
    normal = normalize(inNormal);
#else
    normal = normalize(cross(dFdx(inPos), dFdy(inPos)));
#endif

#if defined(SPECULAR_GLOSSINESS_PIPELINE)
    roughness = material_ubo.roughnessFactor;
    metallic = material_ubo.metallicFactor;

#if defined(HAS_METALLICROUGHNESS_SAMPLER)
    vec4 mrSample = texture(MetallicRoughnessSampler, inUv);
    roughness = clamp(mrSample.g * roughness, 0.0, 1.0);
    metallic = mrSample.b * metallic;
#else
    roughness = clamp(roughness, 0.04, 1.0);
    metallic = clamp(metallic, 0.0, 1.0);
#endif

#elif defined(METALLIC_ROUGHNESS_PIPELINE)
    // Values from specular glossiness workflow are converted to metallic
    // roughness
    vec4 diffuse;
    vec3 specular;

#if defined(HAS_METALLICROUGHNESS_SAMPLER)
    roughness = 1.0 - texture(MetallicRoughnessSampler, inUv).a;
    specular = texture(MetallicRoughnessSampler, inUv).rgb;
#else
    roughness = 1.0;
    specular = vec3(0.0);
#endif

#if defined(HAS_DIFFUSE_SAMPLER)
    diffuse = texture(DiffuseSampler, inUv);
#elif defined(HAS_DIFFUSE_FACTOR)
    diffuse = material_ubo.diffuseFactor;
#endif

    float maxSpecular = max(max(specular.r, specular.g), specular.b);

    // Convert metallic value from specular glossiness inputs
    metallic = convertMetallic(diffuse.rgb, specular, maxSpecular);

    const float minRoughness = 0.04;
    vec3 baseColourDiffusePart = diffuse.rgb *
        ((1.0 - maxSpecular) / (1 - minRoughness) /
         max(1 - metallic, EPSILON)) *
        material_ubo.diffuseFactor.rgb;
    vec3 baseColourSpecularPart = specular -
        (vec3(minRoughness) * (1 - metallic) * (1 / max(metallic, EPSILON))) *
            material_ubo.specularFactor.rgb;
    baseColour = vec4(
        mix(baseColourDiffusePart, baseColourSpecularPart, metallic * metallic),
        diffuse.a);
#endif

    outColour = baseColour;
    outNormal = vec4(normal, 1.0);
    outPbr = vec2(metallic, roughness);

    // ao
    float ambient = 0.4;
#if defined(HAS_OCCLUSION_SAMPLER)
    ambient = texture(OcclusionSampler, inUv).x;
#endif
    outColour.a = ambient;

    // emmisive
    vec3 emissive = vec3(0.2);
#if defined(HAS_EMISSIVE_SAMPLER)
    emissive = texture(EmissiveSampler, inUv).rgb;
    emissive *= material_ubo.emissiveFactor.rgb;
#elif defined(HAS_EMISSIVE_FACTOR)
    emissive = material_ubo.emissiveFactor.rgb;
#endif
    outEmissive = vec4(emissive, 1.0);

    materialFragment();
}
