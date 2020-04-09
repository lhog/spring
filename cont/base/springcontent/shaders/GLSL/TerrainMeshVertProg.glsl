#version 150 compatibility

uniform ivec2 texSquare;
uniform ivec2 mapDims;

uniform mat4 shadowMat;

uniform sampler2D hmVarTex;

const float SMF_TEXSQR_SIZE = 1024.0;

struct Data {
	vec2 mapUV;
	bool inFrustum;
};

out Data dataVS;

#define NORM2SNORM(value) (value * 2.0 - 1.0)
#define SNORM2NORM(value) (value * 0.5 + 0.5)

// Checks the current's patch visibility against the playerCam or shadowCam frustum
// Special case when quad covers view frustum (extreme zoom in) is not covered
bool FrustumCheck(vec4 mp, mat4 MVP) {
	vec4 mvpPos = MVP * mp;
	return all(lessThanEqual(abs(mvpPos.xyz), vec3(mvpPos.w)));
}

void main() {

	vec4 worldPos = vec4(gl_Vertex.xyz, 1.0);
	worldPos.xz += SMF_TEXSQR_SIZE * vec2(texSquare);

	dataVS.mapUV = worldPos.xz / vec2(mapDims);
	
	// the transformation below is already done in the compute shader
#if 0
	//rescale to not sample edge pixels
	vec2 ts = vec2(textureSize(hmVarTex, 0));
	dataVS.mapUV = NORM2SNORM(dataVS.mapUV);
	dataVS.mapUV *= (ts - vec2(1.0)) / ts;
	dataVS.mapUV = SNORM2NORM(dataVS.mapUV);
#endif

	worldPos.y = textureLod(hmVarTex, dataVS.mapUV, 0.0).x;
	//worldPos.y = 0.0;
	
	dataVS.inFrustum = ( FrustumCheck(worldPos, gl_ModelViewProjectionMatrix) || FrustumCheck(worldPos, shadowMat) );

	gl_Position = worldPos;
	
}