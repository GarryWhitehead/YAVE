[[vertex]]

void materialVertex(vec4 pos)
{
    mat4 viewMatrix = mat4(mat3(camera_ubo.view));
    gl_Position = camera_ubo.project * viewMatrix * vec4(inPos.xyz, 1.0);

    // ensure skybox is renderered on the far plane
    gl_Position.z = gl_Position.w;

    outPos = inPos;
    outPos.xy *= -1.0;
}

[[fragment]]

void materialFragment()
{
    outColour = texture(BaseColourSampler, inPos.xyz);

    // we donte zero for the emissive texture alpha channel as this tells the
    // lighting shader to skip applying lighting to the skybox
    outEmissive.a = 0.0;
}