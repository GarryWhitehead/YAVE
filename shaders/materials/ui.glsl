[[vertex]]

void materialVertex(vec4 pos)
{
    outColour = inColour;
    gl_Position =
        vec4(inPos.xy * push_params.scale + push_params.translate, 0, 1);
}

[[fragment]]

void materialFragment()
{
    outColour = inColour * texture(BaseColourSampler, inUv);
}