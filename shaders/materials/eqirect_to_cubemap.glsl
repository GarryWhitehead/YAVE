[[vertex]]

#extension GL_EXT_multiview : enable

void materialVertex(vec4 pos)
{
    outPos = inPos.xyz;
    outPos.xy *= -1.0;
    
    gl_Position = scene_ubo.project * ubo.faceViews[gl_ViewIndex] * vec4(inPos, 1.0);
    gl_Position.z = gl_Position.w;
}

[[fragment]]

void materialFragment()
{
    vec3 v = normalize(inPos.xyz);

    vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
    uv *=  vec2(0.1591, 0.3183);
    uv += 0.5;

    // write out color to output cubemap
	outColour = texture(BaseColourSampler, uv);	
}