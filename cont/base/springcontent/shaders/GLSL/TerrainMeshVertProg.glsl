#version 150 compatibility

uniform ivec2 texSquare;
uniform ivec2 mapDims;

uniform sampler2D heightMap;

const float SMF_TEXSQR_SIZE = 1024.0;

struct Data {
	vec2 mapUV;
};

out Data dataVS;

#define NORM2SNORM(value) (value * 2.0 - 1.0)
#define SNORM2NORM(value) (value * 0.5 + 0.5)


void main() {
	vec4 worldPos = vec4(gl_Vertex.xyz, 1.0);
	worldPos.xz += SMF_TEXSQR_SIZE * vec2(texSquare);

	dataVS.mapUV = worldPos.xz / vec2(mapDims);
	
	//rescale to not sample edge pixels
	vec2 ts = vec2(textureSize(heightMap, 0));
	dataVS.mapUV = NORM2SNORM(dataVS.mapUV);
	dataVS.mapUV *= (ts - vec2(2.0)) / ts;
	dataVS.mapUV = SNORM2NORM(dataVS.mapUV);
	
	

	worldPos.y = textureLod(heightMap, dataVS.mapUV, 0.0).x;

	gl_Position = worldPos;
}