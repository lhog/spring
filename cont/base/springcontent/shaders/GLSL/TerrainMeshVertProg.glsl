#version 150 compatibility

uniform ivec2 texSquare;
uniform ivec2 mapDims;

const float SMF_TEXSQR_SIZE = 1024.0;

struct Data {
	vec2 mapUV;
};

out Data dataVS;

void main() {
	vec4 worldPos = vec4(gl_Vertex.xyz, 1.0);
	worldPos.xz += SMF_TEXSQR_SIZE * vec2(texSquare);
	
	dataVS.mapUV = worldPos.xz / vec2(mapDims);

	gl_Position = worldPos;
}