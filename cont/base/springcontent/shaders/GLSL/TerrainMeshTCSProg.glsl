#version 150 compatibility

#extension GL_ARB_tessellation_shader : require

layout(vertices = 4) out;

uniform float maxTessValue;
uniform ivec2 screenDims;

struct Data {
	vec2 mapUV;
};

in Data dataVS[];
out Data dataTCS[4];

#define MVP gl_ModelViewProjectionMatrix

#define GET_FRUSTUM_LINE(j, s) vec4( \
	MVP[0][3] s MVP[0][j], \
	MVP[1][3] s MVP[1][j], \
	MVP[2][3] s MVP[2][j], \
	MVP[3][3] s MVP[3][j] )

#define linstep(x0, x1, xn) ((xn - x0) / (x1 - x0))

const float tessellatedEdgeSize = 16.0;
const float tessellationFactor = 0.5;
// Calculate the tessellation factor based on screen space
// dimensions of the edge
float ScreenSpaceTessFactor1(vec4 p0, vec4 p1) {
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
	float tessFactor = clamp(distance(clip0.xy, clip1.xy) / maxTessValue / tessellatedEdgeSize * tessellationFactor, 0.0, 1.0);
	return mix(1.0, maxTessValue, tessFactor);
}

// Checks the current's patch visibility against the frustum using a sphere check
// Sphere radius is given by the patch size
bool FrustumCheck() {
	// Fixed radius (increase if patch size is increased in example)
	const float radius = 0.0f;
	vec4 pos = gl_in[gl_InvocationID].gl_Position;

	// https://stackoverflow.com/questions/12836967/extracting-view-frustum-planes-hartmann-gribbs-method
	/// unwrap
	if (dot(pos, GET_FRUSTUM_LINE(0, +) + radius) < 0.0) return false;
	if (dot(pos, GET_FRUSTUM_LINE(0, -) + radius) < 0.0) return false;
	if (dot(pos, GET_FRUSTUM_LINE(1, +) + radius) < 0.0) return false;
	if (dot(pos, GET_FRUSTUM_LINE(1, -) + radius) < 0.0) return false;
	if (dot(pos, GET_FRUSTUM_LINE(2, +) + radius) < 0.0) return false;
	if (dot(pos, GET_FRUSTUM_LINE(2, -) + radius) < 0.0) return false;

	return true;
}

#undef MVP

void main(void) {

	gl_TessLevelOuter[0] = 0.0;
	gl_TessLevelOuter[1] = 0.0;
	gl_TessLevelOuter[2] = 0.0;
	gl_TessLevelOuter[3] = 0.0;

	if (FrustumCheck()) {
		gl_TessLevelOuter[0] = ScreenSpaceTessFactor1(gl_in[3].gl_Position, gl_in[0].gl_Position);
		gl_TessLevelOuter[1] = ScreenSpaceTessFactor1(gl_in[0].gl_Position, gl_in[1].gl_Position);
		gl_TessLevelOuter[2] = ScreenSpaceTessFactor1(gl_in[1].gl_Position, gl_in[2].gl_Position);
		gl_TessLevelOuter[3] = ScreenSpaceTessFactor1(gl_in[2].gl_Position, gl_in[3].gl_Position);
	}

	gl_TessLevelInner[0] = mix(gl_TessLevelOuter[0], gl_TessLevelOuter[3], 0.5);
	gl_TessLevelInner[1] = mix(gl_TessLevelOuter[2], gl_TessLevelOuter[1], 0.5);

	dataTCS[gl_InvocationID].mapUV = dataVS[gl_InvocationID].mapUV;

	gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;

}