#version 150 compatibility

#extension GL_ARB_tessellation_shader : require

layout(vertices = 4) out;

struct Data {
	vec2 mapUV;
};

in Data dataVS[];
out Data dataTCS[4];

void main(void)
{
	gl_TessLevelOuter[0] = 1.0;
	gl_TessLevelOuter[1] = 1.0;
	gl_TessLevelOuter[2] = 1.0;
	gl_TessLevelOuter[3] = 1.0;	
	
	gl_TessLevelInner[0] = mix(gl_TessLevelOuter[0], gl_TessLevelOuter[3], 0.5);
	gl_TessLevelInner[1] = mix(gl_TessLevelOuter[2], gl_TessLevelOuter[1], 0.5);
	
	dataTCS[gl_InvocationID].mapUV = dataVS[gl_InvocationID].mapUV;

	gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
}