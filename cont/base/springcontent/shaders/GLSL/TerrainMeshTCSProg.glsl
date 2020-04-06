#version 150 compatibility

#extension GL_ARB_tessellation_shader : require

layout(vertices = 4) out;

uniform int maxTessValue = 32;
uniform ivec2 screenDims;
uniform ivec2 mapDims;

uniform sampler2D heightMap;

uniform mat4 shadowMat;

struct Data {
	vec2 mapUV;
};

in Data dataVS[];
out Data dataTCS[4];

#define linstep(x0, x1, xn) ((xn - x0) / (x1 - x0))

const float tessellatedEdgeSize = 16.0;
const float tessellationFactor = 0.25;
// Calculate the tessellation factor based on screen space
// dimensions of the edge
float ScreenSpaceTessFactor(vec4 p0, vec4 p1) {
	// Calculate edge mid point
	vec4 midPoint = 0.5 * (p0 + p1);
	// Sphere radius as distance between the control points
	float radius = 0.5 * distance(p0, p1);

	// View space
	vec4 v0 = gl_ModelViewMatrix * midPoint;

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
	return 32.0;
	return clamp(distance(clip0.xy, clip1.xy) / tessellatedEdgeSize * tessellationFactor, 1.0, float(maxTessValue));
	//return 1.0;
}

// Checks the current's patch visibility against the playerCam or shadowCam frustum
// Special case when quad covers view frustum (extreme zoom in) is not covered
bool FrustumCheck(mat4 MVP) {
	bool res = false;
	for (int i = 0; i < 4; ++i) {
		vec4 mvpPos = MVP * gl_in[i].gl_Position;
		res = res || all(lessThanEqual(abs(mvpPos.xyz), vec3(mvpPos.w)));
	}
	return res;
}

#define CULL_TRIANGLES 1

void main(void) {

	gl_TessLevelOuter[0] = 0.0;
	gl_TessLevelOuter[1] = 0.0;
	gl_TessLevelOuter[2] = 0.0;
	gl_TessLevelOuter[3] = 0.0;	
	
#if (CULL_TRIANGLES == 1)
	if ( FrustumCheck(gl_ModelViewProjectionMatrix) || FrustumCheck(shadowMat) )
#endif
	{
		gl_TessLevelOuter[0] = ScreenSpaceTessFactor(gl_in[3].gl_Position, gl_in[0].gl_Position);
		gl_TessLevelOuter[1] = ScreenSpaceTessFactor(gl_in[0].gl_Position, gl_in[1].gl_Position);
		gl_TessLevelOuter[2] = ScreenSpaceTessFactor(gl_in[1].gl_Position, gl_in[2].gl_Position);
		gl_TessLevelOuter[3] = ScreenSpaceTessFactor(gl_in[2].gl_Position, gl_in[3].gl_Position);
	}

	gl_TessLevelInner[0] = mix(gl_TessLevelOuter[0], gl_TessLevelOuter[3], 0.5);
	gl_TessLevelInner[1] = mix(gl_TessLevelOuter[2], gl_TessLevelOuter[1], 0.5);

	dataTCS[gl_InvocationID].mapUV = dataVS[gl_InvocationID].mapUV;

	gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;

}