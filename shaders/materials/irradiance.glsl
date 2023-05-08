[[vertex]]

#extension GL_EXT_multiview : enable

void materialVertex(vec4 pos)
{
    outPos = inPos.xyz;
	
	gl_Position = scene_ubo.project * ubo.faceViews[gl_ViewIndex] * vec4(inPos, 1.0);
    gl_Position.z = gl_Position.w;
}

[[fragment]]

#define PI 3.1415926535897932384626433832795

void materialFragment()
{
    vec3 N = normalize(inPos);
	vec3 up = vec3(0.0, 1.0, 0.0);
	vec3 right = normalize(cross(up, N));
	up = cross(N, right);
	
	vec3 irrColour = vec3(0.0);
	float sampleCount = 0.0;
		
	const float doublePI = PI * 2;
	const float halfPI = PI * 0.5;

	const float dPhi = 0.035;
	const float dTheta = 0.025;
	
	for(float phi = 0.0; phi < doublePI; phi += dPhi) 
    {
		for(float theta = 0.0; theta < halfPI; theta += dTheta) 
        {
			vec3 tempVec = cos(phi) * right + sin(phi) * up;
			vec3 sampleVector = cos(theta) * N + sin(theta) * tempVec;
			irrColour += texture(BaseColourSampler, sampleVector).rgb * cos(theta) * sin(theta);
			
			// spherical to cartesian
			//vec3 tanSample = vec3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
			
			// to world space
			//vec3 wPos = tanSample.x * right + tanSample.y * up + tanSample.z * N;
			
			//irrColour += texture(envSampler, wPos).rgb * cos(theta) * sin(theta);
			sampleCount++;
		}
	}

	outColour = vec4(PI * irrColour / float(sampleCount), 1.0);
}