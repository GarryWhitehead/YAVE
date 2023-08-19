[[vertex]]

void materialVertex(vec4 pos)
{
    gl_Position = vec4(inPos, 1.0);
	
	outPos = inPos;
	outUv = inUv;
}

[[tesse-control]]

float screenSpaceTessFactor(vec4 p0, vec4 p1)
{
	vec4 midPoint = 0.5 * (p0 + p1);
	float radius = distance(p0, p1) / 2.0;

	// View space
	vec4 v0 = scene_ubo.view * midPoint;

	// Project into clip space
	vec4 clip0 = (scene_ubo.project * (v0 - vec4(radius, vec3(0.0))));
	vec4 clip1 = (scene_ubo.project * (v0 + vec4(radius, vec3(0.0))));

	// Get normalized device coordinates
	clip0 /= clip0.w;
	clip1 /= clip1.w;

	// Convert to viewport coordinates
	clip0.xy *= material_ubo.screenSize;
	clip1.xy *= material_ubo.screenSize;
	
	return clamp(distance(clip0, clip1) / material_ubo.tessEdgeSize * material_ubo.tessFactor, 1.0, 64.0);
}

void materialTessControl()
{
    if(gl_InvocationID == 0) 
    {
        /*vec4 vp00 = scene_ubo.view * gl_in[0].gl_Position;
        vec4 vp01 = scene_ubo.view * gl_in[1].gl_Position;
        vec4 vp10 = scene_ubo.view * gl_in[2].gl_Position;
        vec4 vp11 = scene_ubo.view * gl_in[3].gl_Position;
        
        float d00 = clamp((abs(vp00.z) - material_ubo.minDistance) / (material_ubo.maxDistance - material_ubo.minDistance), 0.0, 1.0);
        float d01 = clamp((abs(vp01.z) - material_ubo.minDistance) / (material_ubo.maxDistance - material_ubo.minDistance), 0.0, 1.0);
        float d10 = clamp((abs(vp10.z) - material_ubo.minDistance) / (material_ubo.maxDistance - material_ubo.minDistance), 0.0, 1.0);
        float d11 = clamp((abs(vp11.z) - material_ubo.minDistance) / (material_ubo.maxDistance - material_ubo.minDistance), 0.0, 1.0);

        float tessFactor0 = mix(material_ubo.maxTessLevel, material_ubo.minTessLevel, min(d10, d00));
        float tessFactor1 = mix(material_ubo.maxTessLevel, material_ubo.minTessLevel, min(d00, d01));
        float tessFactor2 = mix(material_ubo.maxTessLevel, material_ubo.minTessLevel, min(d01, d11));
        float tessFactor3 = mix(material_ubo.maxTessLevel, material_ubo.minTessLevel, min(d11, d10));

        gl_TessLevelOuter[0] = tessFactor0;
        gl_TessLevelOuter[1] = tessFactor1;
        gl_TessLevelOuter[2] = tessFactor2;
        gl_TessLevelOuter[3] = tessFactor3;

        gl_TessLevelInner[0] = mix(gl_TessLevelOuter[0], gl_TessLevelOuter[3], 0.5);
        gl_TessLevelInner[1] = mix(gl_TessLevelOuter[2], gl_TessLevelOuter[1], 0.5);*/

        gl_TessLevelOuter[0] = screenSpaceTessFactor(gl_in[3].gl_Position, gl_in[0].gl_Position);
        gl_TessLevelOuter[1] = screenSpaceTessFactor(gl_in[0].gl_Position, gl_in[1].gl_Position);
        gl_TessLevelOuter[2] = screenSpaceTessFactor(gl_in[1].gl_Position, gl_in[2].gl_Position);
        gl_TessLevelOuter[3] = screenSpaceTessFactor(gl_in[2].gl_Position, gl_in[3].gl_Position);
        gl_TessLevelInner[0] = mix(gl_TessLevelOuter[0], gl_TessLevelOuter[3], 0.5);
        gl_TessLevelInner[1] = mix(gl_TessLevelOuter[2], gl_TessLevelOuter[1], 0.5);
	}
}

[[tesse-eval]]

void materialTessEval(TessEvalInput tessInput)
{	
	// displace the y coord depending on height derived from map
    vec2 displacement = textureLod(DisplacementMap, tessInput.uv, 0.0).yz;
	tessInput.pos.y = textureLod(DisplacementMap, tessInput.uv, 0.0).r * material_ubo.dispFactor;
    tessInput.pos.x += displacement.x;
    tessInput.pos.z += displacement.y;

	// convert everything to world space
	gl_Position = scene_ubo.project * scene_ubo.view * tessInput.pos;
	
	// position (world space)
	outPos = vec3(scene_ubo.model * tessInput.pos);

    outUv = tessInput.uv;
}

[[fragment]]

void materialFragment()
{
    vec2 gradient = texture(GradientMap, inUv).xy;
    float jacobian = texture(GradientMap, inUv).z;
    vec2 N = texture(NormalMap, inUv).xy;
    
    vec2 noiseGradient = 0.30 * N;
    float turbulence = max(2.0 - jacobian + dot(abs(noiseGradient), vec2(1.2)), 0.0);

    float colourMod = 1.0 + 3.0 * smoothstep(1.2, 1.8, turbulence);
	
    outNormal = normalize(vec4(-gradient.x, 1.0, -gradient.y, 0.0));

    // default water colour
	vec3 waterCol = vec3(0.07, 0.15, 0.2);		
    outColour = vec4(waterCol.rgb * colourMod, 1.0);	

	outPos = vec4(inPos, 1.0);

    float metallic = 0.0; // 1.0 - 0.05 * turbulence;
    float roughness = 0.03;// * turbulence;
    outPbr = vec2(metallic, roughness);
}

