#version 150 compatibility

#extension GL_ARB_geometry_shader4 : require
#extension GL_ARB_shader_storage_buffer_object : require

layout ( triangles ) in;
layout ( triangle_strip, max_vertices = 3 ) out;

#ifdef SSBO
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
#else
	out vec3 vPosTF;
#endif

struct Data {
	vec2 mapUV;
};

in Data dataTES[];
out Data dataGS;

// seems meaningless
/*
bool CheckTriangleFacing(vec4 p0, vec4 p1, vec4 p2) {

	vec4 p0c = gl_ModelViewProjectionMatrix * p0; p0c /= p0c.w;
	vec4 p1c = gl_ModelViewProjectionMatrix * p1; p1c /= p1c.w;
	vec4 p2c = gl_ModelViewProjectionMatrix * p2; p2c /= p2c.w;

	vec3 trNormal = cross( normalize(p1c.xyz - p0c.xyz), normalize(p1c.xyz - p2c.xyz) );

	return (trNormal.z < 0.0);
}
*/

void main() {
	
	//if (!CheckTriangleFacing(gl_in[0].gl_Position, gl_in[1].gl_Position, gl_in[2].gl_Position))
		//return;

#ifdef SSBO
	int idx = atomicAdd(count, 3);
#endif

	for (int i = 0; i < 3; ++i) {

		gl_Position = gl_in[i].gl_Position;
	#ifdef SSBO
		gl_PrimitiveID = idx / 3;
		pos[idx + i].x = gl_Position.x;
		pos[idx + i].y = gl_Position.y;
		pos[idx + i].z = gl_Position.z;
	#else
		vPosTF = gl_Position.xyz;
		gl_PrimitiveID = gl_PrimitiveIDIn;
	#endif

		dataGS.mapUV = dataTES[i].mapUV;

		gl_Position = gl_ModelViewProjectionMatrix * gl_Position;

		EmitVertex();
	}

	EndPrimitive();
}