[[vertex]]

void materialVertex(vec4 pos)
{
    mat4 viewMatrix = mat4(mat3(scene_ubo.view));
    gl_Position = scene_ubo.project * viewMatrix * vec4(inPos.xyz, 1.0);

    // ensure skybox is renderered on the far plane
    gl_Position.z = gl_Position.w;

    outPos = inPos;
    outPos.xy *= -1.0;
}

[[fragment]]

void materialFragment()
{
    vec4 skyColour;
    if (material_ubo.useColour != 0)
    {
        skyColour = material_ubo.colour;
    }
    else
    {
        skyColour = texture(BaseColourSampler, inPos.xyz);
    }

    if (material_ubo.renderSun != 0)
    {
        float intensity = scene_ubo.lightColourIntensity.a;
        vec3 dir = normalize(inPos.xyz);
        vec3 sun = scene_ubo.lightColourIntensity.rgb * (intensity * (PI * 4.0));
        float angle = dot(dir, scene_ubo.lightDirection.xyz);
        float x = (angle - scene_ubo.sun.x) * scene_ubo.sun.z;
        float grad = pow(1.0 - clamp(x, 0.0, 1.0), scene_ubo.sun.w); 
        skyColour.rgb = skyColour.rgb + grad * sun;
    }

    outColour = skyColour;

    // we donte zero for the emissive texture alpha channel as this tells the
    // lighting shader to skip applying lighting to the skybox
    outEmissive.a = 0.0;
}