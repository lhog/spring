#version 150 compatibility

#extension GL_ARB_tessellation_shader : require

layout(vertices = 4) out;

uniform ivec2 screenDims;

struct Data {
	vec2 mapUV;
};

in Data dataVS[];
out Data dataTCS[4];


const float tessellatedEdgeSize = 8.0;
const float tessellationFactor = 0.25;
// Calculate the tessellation factor based on screen space
// dimensions of the edge
float screenSpaceTessFactor(vec4 p0, vec4 p1)
{
	// Calculate edge mid point
	vec4 midPoint = 0.5 * (p0 + p1);
	// Sphere radius as distance between the control points
	float radius = 0.5 * distance(p0, p1);

	// View space
	vec4 v0 = gl_ModelViewMatrix  * midPoint;

	// Project into clip space
	vec4 clip0 = (gl_ProjectionMatrix * (v0 - vec4(radius, vec3(0.0))));
	vec4 clip1 = (gl_ProjectionMatrix * (v0 + vec4(radius, vec3(0.0))));

	// Get normalized device coordinates
	clip0 /= clip0.w;
	clip1 /= clip1.w;

	// Convert to viewport coordinates
	clip0.xy *= vec2(screenDims);
	clip1.xy *= vec2(screenDims);

	// Return the tessellation factor based on the screen size
	// given by the distance of the two edge control points in screen space
	// and a reference (min.) tessellation size for the edge set by the application
	//return clamp(distance(clip0, clip1) / ubo.tessellatedEdgeSize * ubo.tessellationFactor, 1.0, 64.0);
	return clamp(distance(clip0, clip1) / tessellatedEdgeSize * tessellationFactor, 1.0, 64.0);
}

/*
// Checks the current's patch visibility against the frustum using a sphere check
// Sphere radius is given by the patch size
bool frustumCheck()
{
	// Fixed radius (increase if patch size is increased in example)
	const float radius = 8.0f;
	vec4 pos = gl_in[gl_InvocationID].gl_Position;
	pos.y -= textureLod(samplerHeight, inUV[0], 0.0).r * ubo.displacementFactor;

	// Check sphere against frustum planes
	for (int i = 0; i < 6; i++) {
		if (dot(pos, ubo.frustumPlanes[i]) + radius < 0.0)
		{
			return false;
		}
	}
	return true;
}
*/


void main(void)
{
	gl_TessLevelOuter[0] = screenSpaceTessFactor(gl_in[3].gl_Position, gl_in[0].gl_Position);
	gl_TessLevelOuter[1] = screenSpaceTessFactor(gl_in[0].gl_Position, gl_in[1].gl_Position);
	gl_TessLevelOuter[2] = screenSpaceTessFactor(gl_in[1].gl_Position, gl_in[2].gl_Position);
	gl_TessLevelOuter[3] = screenSpaceTessFactor(gl_in[2].gl_Position, gl_in[3].gl_Position);

	gl_TessLevelInner[0] = mix(gl_TessLevelOuter[0], gl_TessLevelOuter[3], 0.5);
	gl_TessLevelInner[1] = mix(gl_TessLevelOuter[2], gl_TessLevelOuter[1], 0.5);

	dataTCS[gl_InvocationID].mapUV = dataVS[gl_InvocationID].mapUV;

	gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
}