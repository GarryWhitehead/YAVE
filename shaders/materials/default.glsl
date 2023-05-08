[[vertex]]

void materialVertex(vec4 pos)
{
    outPos = pos.xyz / pos.w;
    gl_Position = scene_ubo.mvp * vec4(outPos, 1.0);
}

[[fragment]]

void materialFragment()
{
    outPos = vec4(inPos, 1.0);
}