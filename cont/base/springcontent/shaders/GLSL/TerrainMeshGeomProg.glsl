#version 150 compatibility

#extension GL_ARB_geometry_shader4 : require
#extension GL_ARB_shader_storage_buffer_object : require

layout ( triangles ) in;
layout ( triangle_strip, max_vertices = 3 ) out;

uniform mat4 shadowMat;

layout(std140, binding = 0) buffer DrawArraysIndirectCommand {
	int count;
	int primCount;
	int first;
	int baseInstance;
};

// https://community.khronos.org/t/size-of-elements-of-arrays-in-shader-storage-buffer-objects/69803/6
struct Pos {
	float x, y, z;
};

layout(std430, binding = 1) writeonly restrict buffer MeshTessTriangle {
	Pos pos[];
};


struct Data {
	vec2 mapUV;
};

in Data dataTES[];
out Data dataGS;

// seems meaningless, but god for test to reduce the number of saved triangles
bool CheckTriangleFacing(mat4 MVP) {
	vec4 p0c = MVP * gl_in[0].gl_Position; p0c /= p0c.w;
	vec4 p1c = MVP * gl_in[1].gl_Position; p1c /= p1c.w;
	vec4 p2c = MVP * gl_in[2].gl_Position; p2c /= p2c.w;

	vec3 trNormal = cross( normalize(p1c.xyz - p0c.xyz), normalize(p1c.xyz - p2c.xyz) );

	return (trNormal.z <= 0.0);
}

#define CULL_TRIANGLES 1

void main() {

#if (CULL_TRIANGLES == 1)
	if (!CheckTriangleFacing(gl_ModelViewProjectionMatrix) && !CheckTriangleFacing(shadowMat))
		return;
#endif


	int idx = atomicAdd(count, 3);

	for (int i = 0; i < 3; ++i) {

		gl_Position = gl_in[i].gl_Position;
		gl_PrimitiveID = idx / 3;
		pos[idx + i].x = gl_Position.x;
		pos[idx + i].y = gl_Position.y;
		pos[idx + i].z = gl_Position.z;


		dataGS.mapUV = dataTES[i].mapUV;

		gl_Position = gl_ModelViewProjectionMatrix * gl_Position;

		EmitVertex();
	}

	EndPrimitive();
}