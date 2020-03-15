#version 130
#extension GL_ARB_tessellation_shader : enable

layout(vertices = 4) out;

in vec3 halfDir[];
in float fogFactor[];
in vec4 vertexWorldPos[];
in vec2 diffuseTexCoords[];

out vec3 halfDirTCS[];
out float fogFactorTCS[];
out vec4 vertexWorldPosTCS[];
out vec2 diffuseTexCoordsTCS[];

void main()
{
	// Pass along the vertex position unmodified
	gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;

	halfDirTCS[gl_InvocationID] = halfDir[gl_InvocationID];
	fogFactorTCS[gl_InvocationID] = fogFactor[gl_InvocationID];
	vertexWorldPosTCS[gl_InvocationID] = vertexWorldPos[gl_InvocationID];
	diffuseTexCoordsTCS[gl_InvocationID] = diffuseTexCoords[gl_InvocationID];

	gl_TessLevelOuter[0] = 1.0;
	gl_TessLevelOuter[1] = 1.0;
	gl_TessLevelOuter[2] = 1.0;
	gl_TessLevelOuter[4] = 1.0;

	gl_TessLevelInner[0] = 1.0;
	gl_TessLevelInner[1] = 1.0;
}