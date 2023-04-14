layout(location = 0) in vec3 inPos;
#if defined(HAS_UV_ATTR_INPUT)
layout(location = 1) in vec2 inUv;
#endif
#if defined(HAS_NORMAL_ATTR_INPUT)
layout(location = 2) in vec3 inNormal;
#endif
#if defined(HAS_COLOUR_ATTR_INPUT)
layout(location = 3) in vec4 inColour;
#endif
#if defined(HAS_SKIN)
layout(location = 4) in vec4 inWeights;
layout(location = 5) in vec4 inBoneId;
#endif

#if defined(HAS_UV_ATTR_INPUT)
layout(location = 0) out vec2 outUv;
#endif
#if defined(HAS_NORMAL_ATTR_INPUT)
layout(location = 1) out vec3 outNormal;
#endif
#if defined(HAS_COLOUR_ATTR_INPUT)
layout(location = 2) out vec4 outColour;
#endif
layout(location = 3) out vec3 outPos;

#define MAX_BONES 250

void main()
{
#if defined(HAS_SKIN)
    mat4 boneTransform = skin_ubo.bones[int(inBoneId.x)] * inWeights.x;
    boneTransform += skin_ubo.bones[int(inBoneId.y)] * inWeights.y;
    boneTransform += skin_ubo.bones[int(inBoneId.z)] * inWeights.z;
    boneTransform += skin_ubo.bones[int(inBoneId.w)] * inWeights.w;

    mat4 normalTransform = mesh_ubo.modelMatrix * boneTransform;
#else
    mat4 normalTransform = mesh_ubo.modelMatrix;
    vec4 pos = normalTransform * vec4(inPos, 1.0);
#endif

#if defined(HAS_NORMAL_ATTR_INPUT)
    // inverse-transpose for non-uniform scaling - expensive computations here -
    // maybe remove this?
    outNormal = normalize(transpose(inverse(mat3(normalTransform))) * inNormal);
#endif
#if defined(HAS_UV_ATTR_INPUT)
    outUv = inUv;
#endif
#if defined(HAS_COLOUR_ATTR_INPUT)
    outColour = inColour;
#endif

    materialVertex(pos);
}
