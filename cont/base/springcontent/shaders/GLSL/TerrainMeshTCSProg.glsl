#version 150 compatibility

#extension GL_ARB_tessellation_shader : require

layout(vertices = 4) out;

uniform ivec2 texSquare;
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
	//clip0.xy *= vec2(screenDims);
	//clip1.xy *= vec2(screenDims);

	// Return the tessellation factor based on the screen size
	// given by the distance of the two edge control points in screen space
	// and a reference (min.) tessellation size for the edge set by the application
	//return clamp(distance(clip0, clip1) / ubo.tessellatedEdgeSize * ubo.tessellationFactor, 1.0, 64.0);
	float tessFactor = clamp(distance(clip0.xy, clip1.xy) / maxTessValue / tessellatedEdgeSize * tessellationFactor, 0.0, 1.0);
	return mix(1.0, maxTessValue, tessFactor);
}


// these below cause gaps
// https://stackoverflow.com/questions/27652769/better-control-over-tessellation-in-opengl
float ScreenSpaceTessFactor2(vec4 p0, vec4 p1, vec4 p2, vec4 p3) {
	vec4 center = 0.25 * (p0 + p1 + p2 + p3);
	float radius = max(
		max(distance(center.xyz, p0.xyz), distance(center.xyz, p1.xyz)),
		max(distance(center.xyz, p2.xyz), distance(center.xyz, p3.xyz))	);


	// Transform object-space X extremes into clip-space
	vec4 min0 = MVP * (center - vec4 (radius, 0.0f, 0.0f, 0.0f));
	vec4 max0 = MVP * (center + vec4 (radius, 0.0f, 0.0f, 0.0f));

	// Transform object-space Y extremes into clip-space
	vec4 min1 = MVP * (center - vec4 (0.0f, radius, 0.0f, 0.0f));
	vec4 max1 = MVP * (center + vec4 (0.0f, radius, 0.0f, 0.0f));

	// Transform object-space Z extremes into clip-space
	vec4 min2 = MVP * (center - vec4 (0.0f, 0.0f, radius, 0.0f));
	vec4 max2 = MVP * (center + vec4 (0.0f, 0.0f, radius, 0.0f));

	// Transform from clip-space to NDC
	min0 /= min0.w; max0 /= max0.w;
	min1 /= min1.w; max1 /= max1.w;
	min2 /= min2.w; max2 /= max2.w;

	// Calculate the distance (ignore depth) covered by all three pairs of extremes
	float dist0 = distance(min0.xy, max0.xy);
	float dist1 = distance(min1.xy, max1.xy);
	float dist2 = distance(min2.xy, max2.xy);

	float maxDist = max(dist0, max(dist1, dist2));

	float tessFactor = clamp(0.5 * maxDist, 0.0, 1.0);
	return mix(1.0, maxTessValue, tessFactor);
}

float ScreenSpaceTessFactor3(vec4 p0, vec4 p1, vec4 p2) {
	vec4 p0c = MVP * p0; p0c /= p0c.w; p0c.xy *= vec2(screenDims);
	vec4 p1c = MVP * p1; p1c /= p1c.w; p1c.xy *= vec2(screenDims);
	vec4 p2c = MVP * p2; p2c /= p2c.w; p2c.xy *= vec2(screenDims);

	//float trArea = 0.5 * length(cross( p1c.xyz - p0c.xyz, p0c.xyz - p2c.xyz ));
	float trArea = -(0.5 * determinant(mat2(p0c.xy - p1c.xy, p2c.xy - p1c.xy)));
	if (trArea < 0) return 0.0;

	float tessFactor = clamp(trArea / maxTessValue / tessellatedEdgeSize / tessellatedEdgeSize * tessellationFactor, 0.0, 1.0);
}

float TriangleFacing(vec4 p0, vec4 p1, vec4 p2) {
	vec4 p0c = MVP * p0; p0c /= p0c.w;
	vec4 p1c = MVP * p1; p1c /= p1c.w;
	vec4 p2c = MVP * p2; p2c /= p2c.w;

	float trArea = 0.5 * determinant(mat2(p0c.xy - p1c.xy, p2c.xy - p1c.xy));

	return mix(1.0, 0.0, float(trArea > 0.0));
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

#define METHOD 1

void main(void) {

	if (!FrustumCheck()) {
		gl_TessLevelOuter[0] = 0.0;
		gl_TessLevelOuter[1] = 0.0;
		gl_TessLevelOuter[2] = 0.0;
		gl_TessLevelOuter[3] = 0.0;
	} else {
		#if (METHOD == 1)
			float f1 = TriangleFacing(gl_in[3].gl_Position, gl_in[0].gl_Position, gl_in[1].gl_Position);
			float f2 = TriangleFacing(gl_in[1].gl_Position, gl_in[2].gl_Position, gl_in[3].gl_Position);
			float f = max(f1, f2);
			//f = 1.0;
			gl_TessLevelOuter[0] = ScreenSpaceTessFactor1(gl_in[3].gl_Position, gl_in[0].gl_Position) * f;
			gl_TessLevelOuter[1] = ScreenSpaceTessFactor1(gl_in[0].gl_Position, gl_in[1].gl_Position) * f;
			gl_TessLevelOuter[2] = ScreenSpaceTessFactor1(gl_in[1].gl_Position, gl_in[2].gl_Position) * f;
			gl_TessLevelOuter[3] = ScreenSpaceTessFactor1(gl_in[2].gl_Position, gl_in[3].gl_Position) * f;
		#elif (METHOD == 2)
			gl_TessLevelOuter[0] = ScreenSpaceTessFactor2(gl_in[3].gl_Position, gl_in[0].gl_Position, gl_in[1].gl_Position, gl_in[2].gl_Position);
			gl_TessLevelOuter[1] = gl_TessLevelOuter[0];
			gl_TessLevelOuter[2] = gl_TessLevelOuter[0];
			gl_TessLevelOuter[3] = gl_TessLevelOuter[0];

			gl_TessLevelOuter[0] = max(gl_TessLevelOuter[0], ScreenSpaceTessFactor1(gl_in[3].gl_Position, gl_in[0].gl_Position));
			gl_TessLevelOuter[1] = max(gl_TessLevelOuter[1], ScreenSpaceTessFactor1(gl_in[0].gl_Position, gl_in[1].gl_Position));
			gl_TessLevelOuter[2] = max(gl_TessLevelOuter[2], ScreenSpaceTessFactor1(gl_in[1].gl_Position, gl_in[2].gl_Position));
			gl_TessLevelOuter[3] = max(gl_TessLevelOuter[3], ScreenSpaceTessFactor1(gl_in[2].gl_Position, gl_in[3].gl_Position));
		#elif (METHOD == 3)
			gl_TessLevelOuter[0] = ScreenSpaceTessFactor3(gl_in[3].gl_Position, gl_in[0].gl_Position, gl_in[1].gl_Position);
			gl_TessLevelOuter[1] = gl_TessLevelOuter[0];
			gl_TessLevelOuter[2] = ScreenSpaceTessFactor3(gl_in[1].gl_Position, gl_in[2].gl_Position, gl_in[3].gl_Position);
			gl_TessLevelOuter[3] = gl_TessLevelOuter[2];
		#else
			gl_TessLevelOuter[0] = 1.0;
			gl_TessLevelOuter[1] = 1.0;
			gl_TessLevelOuter[2] = 1.0;
			gl_TessLevelOuter[3] = 1.0;
		#endif
	}

	gl_TessLevelInner[0] = mix(gl_TessLevelOuter[0], gl_TessLevelOuter[3], 0.5);
	gl_TessLevelInner[1] = mix(gl_TessLevelOuter[2], gl_TessLevelOuter[1], 0.5);

	dataTCS[gl_InvocationID].mapUV = dataVS[gl_InvocationID].mapUV;

	gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;

}