[[vertex]]

#extension GL_EXT_multiview : enable

void materialVertex(vec4 pos)
{
    outPos = inPos;
    gl_Position = camera_ubo.project * ubo.faceViews[gl_ViewIndex] * vec4(inPos, 1.0);
}

[[fragment]]

void materialFragment()
{
    const float PI = 3.141592;
    const float PI_2 = PI * 2.0;

    vec3 v = inPos.xyz;

	// convert Cartesian direction vector to spherical coords
	const float phi = atan(v.z, v.x);
	const float theta = acos(v.y);

    // write out color to output cubemap
	outColour = texture(BaseColourSampler, vec2(phi / PI_2, theta / PI));	
}