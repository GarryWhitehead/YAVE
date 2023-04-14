layout(location = 0) out vec2 outUv;
layout(location = 1) out vec3 outCameraPos;

void main()
{
    outUv = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
    gl_Position = vec4(outUv * 2.0f + -1.0f, 0.0f, 1.0f);

    outCameraPos = camera_ubo.position;
}
