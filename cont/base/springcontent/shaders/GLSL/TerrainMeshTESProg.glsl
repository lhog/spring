#version 150 compatibility

#extension GL_ARB_tessellation_shader : require

layout( quads, equal_spacing, ccw ) in;

uniform sampler2D heightMap;

struct Data {
	vec2 mapUV;
};

in Data dataTCS[];
out Data dataTES;

#define LERP4VALUES(v0, v1, v2, v3) mix( mix(v0, v1, gl_TessCoord.x), mix(v3, v2, gl_TessCoord.x), gl_TessCoord.y )
#define LERP4(s, f) LERP4VALUES(s[0].f, s[1].f, s[2].f, s[3].f)

void main() {

	gl_Position	= LERP4(gl_in, gl_Position);

	dataTES.mapUV = LERP4(dataTCS, mapUV);

	gl_Position.y = textureLod(heightMap, dataTES.mapUV, 0.0).x;

	gl_Position = gl_Position;
}