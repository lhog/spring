#version 150 compatibility
#extension GL_ARB_tessellation_shader : require

layout(vertices = 4) out;

uniform int maxTessValue;
//const int maxTessValue = 64;
uniform ivec2 screenDims;
uniform ivec2 mapDims;

uniform sampler2D hmVarTex;

uniform mat4 shadowMat;

struct Data {
	vec2 mapUV;
	bool inFrustum;
};

in Data dataVS[];
out Data dataTCS[4];

#define linstep(x0, x1, xn) ((xn - x0) / (x1 - x0))

const float tessellatedEdgeSize = 16.0;
const float tessellationFactor = 0.25;
// Calculate the tessellation factor based on screen space
// dimensions of the edge
float ScreenSpaceTessFactor(int i0, int i1) {

	vec4 p0 = gl_in[i0].gl_Position;
	vec4 p1 = gl_in[i1].gl_Position;
	// Calculate edge mid point
	vec4 midPoint = mix(p0, p1, 0.5);
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

	return clamp(distance(clip0.xy, clip1.xy) / tessellatedEdgeSize * tessellationFactor, 1.0, float(maxTessValue));
}

float ScreenSpaceFactor(int i0, int i1) {
	vec4 p0 = gl_in[i0].gl_Position;
	vec4 p1 = gl_in[i1].gl_Position;
	
	#if 1
		vec4 midPoint = mix(p0, p1, 0.5);
		float radius = 0.5 * distance(p0, p1);	
		vec4 v0 = gl_ModelViewMatrix * midPoint;
		vec4 clip0 = (gl_ProjectionMatrix * (v0 - vec4(radius, vec3(0.0))));
		vec4 clip1 = (gl_ProjectionMatrix * (v0 + vec4(radius, vec3(0.0))));
	#else
		vec4 clip0 = gl_ModelViewProjectionMatrix * p0;
		vec4 clip1 = gl_ModelViewProjectionMatrix * p1; 
	#endif
	
	clip0 /= clip0.w;
	clip1 /= clip1.w;

	return 0.5 + 0.5 * smoothstep(0.0, 0.06, 0.5 * distance(clip0.xy, clip1.xy));
}


//#define UV_OK(uv) all(bvec4( greaterThanEqual(uv, vec2(0.0)), lessThanEqual(uv, vec2(1.0)) ))
#define UV_OK(uv) true
#define GET_VARIANCE(uv, ml) (textureLod(hmVarTex, uv, ml).x / 16.0)
#define VARIANCE_TO_TESSLEVEL(v) mix(1.0, float(maxTessValue), clamp(v, 0.0, 1.0))
//      0
// (0)-----(3)
//  |       |
// 1|  mid  |3
//  |       |
// (1)-----(2)
//      2

void SetOuterTesselationLevels() {
	float mipLevel = float(log2(maxTessValue));

	vec3 uvDiff = vec3(dataVS[2].mapUV - dataVS[0].mapUV, 0.0);
	vec2 uvMid  = mix(dataVS[2].mapUV, dataVS[0].mapUV, 0.5);
	//vec2 uvMid  = dataVS[0].mapUV;

	float thisVariance = GET_VARIANCE(uvMid, mipLevel);
	float edgeVariance;

	vec2 uvNeigh;

	// top edge
	uvNeigh = uvMid - uvDiff.zy;
	edgeVariance = thisVariance;
	if (UV_OK(uvNeigh))
		edgeVariance = max(thisVariance, GET_VARIANCE(uvNeigh, mipLevel));

	gl_TessLevelOuter[0] = VARIANCE_TO_TESSLEVEL(edgeVariance) * ScreenSpaceFactor(0, 3);

	// bottom edge
	uvNeigh = uvMid + uvDiff.zy;
	edgeVariance = thisVariance;
	if (UV_OK(uvNeigh))
		edgeVariance = max(thisVariance, GET_VARIANCE(uvNeigh, mipLevel));

	gl_TessLevelOuter[2] = VARIANCE_TO_TESSLEVEL(edgeVariance) * ScreenSpaceFactor(1, 2);

	// left edge
	uvNeigh = uvMid - uvDiff.xz;
	edgeVariance = thisVariance;
	if (UV_OK(uvNeigh))
		edgeVariance = max(thisVariance, GET_VARIANCE(uvNeigh, mipLevel));

	gl_TessLevelOuter[1] = VARIANCE_TO_TESSLEVEL(edgeVariance) * ScreenSpaceFactor(0, 1);

	// right edge
	uvNeigh = uvMid + uvDiff.xz;
	edgeVariance = thisVariance;
	if (UV_OK(uvNeigh))
		edgeVariance = max(thisVariance, GET_VARIANCE(uvNeigh, mipLevel));

	gl_TessLevelOuter[3] = VARIANCE_TO_TESSLEVEL(edgeVariance) * ScreenSpaceFactor(2, 3);
}

// Checks the current's patch visibility against the playerCam or shadowCam frustum
// Special case when quad covers view frustum (extreme zoom in) is not covered
bool FrustumCheckBool() {
	bool res = false;
	for (int i = 0; i < 4; ++i) {
		res = res || dataVS[i].inFrustum;
	}
	return res;
}

#define CULL_TRIANGLES 1

void main(void) {

	if (gl_InvocationID == 0) {
		gl_TessLevelOuter[0] = 0.0;
		gl_TessLevelOuter[1] = 0.0;
		gl_TessLevelOuter[2] = 0.0;
		gl_TessLevelOuter[3] = 0.0;

	#if (CULL_TRIANGLES == 1)
		if ( FrustumCheckBool() )
	#endif
		{
			SetOuterTesselationLevels();
			#if 0
				float fixFactor = maxTessValue;
				gl_TessLevelOuter[0] = fixFactor;
				gl_TessLevelOuter[1] = fixFactor;
				gl_TessLevelOuter[2] = fixFactor;
				gl_TessLevelOuter[3] = fixFactor;
			#endif
		}
	}

	gl_TessLevelInner[0] = mix(gl_TessLevelOuter[1], gl_TessLevelOuter[3], 0.5);
	gl_TessLevelInner[1] = mix(gl_TessLevelOuter[0], gl_TessLevelOuter[2], 0.5);

	dataTCS[gl_InvocationID].mapUV = dataVS[gl_InvocationID].mapUV;
	gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;

	//if (gl_InvocationID == 0)
	//gl_out[gl_InvocationID].gl_Position.y += 100.0;
	//gl_out[gl_InvocationID].gl_Position.y += 10.0 * textureLod(hmVarTex, dataVS[gl_InvocationID].mapUV, 255.0).x;

}