#version 130
#extension GL_ARB_tessellation_shader : enable

layout(quads, equal_spacing, ccw) in;

in vec3 halfDirTCS[];
in float fogFactorTCS[];
in vec4 vertexWorldPosTCS[];
in vec2 diffuseTexCoordsTCS[];
 
out vec3 halfDir;
out float fogFactor;
out vec4 vertexWorldPos;
out vec2 diffuseTexCoords;

#define MIX_QUAD(i) mix( mix(i[0], i[1], gl_TessCoord.x), mix(i[3], i[2], gl_TessCoord.x), gl_TessCoord.y )
#define MIX_POS() mix( mix(gl_in[0].gl_Position, gl_in[1].gl_Position, gl_TessCoord.x), mix(gl_in[3].gl_Position, gl_in[2].gl_Position, gl_TessCoord.x), gl_TessCoord.y )

void main()
{
	halfDir = MIX_QUAD(halfDirTCS);
	fogFactor = MIX_QUAD(fogFactorTCS);
	vertexWorldPos = MIX_QUAD(vertexWorldPosTCS);
	diffuseTexCoords = MIX_QUAD(diffuseTexCoordsTCS);
	
	gl_Position = MIX_POS();
}
