#version 150 compatibility

#extension GL_ARB_geometry_shader4 : require
#extension GL_ARB_shader_storage_buffer_object : require

layout ( triangles ) in;
layout ( triangle_strip, max_vertices = 3 ) out;

layout(std140, binding = 1) buffer AuxSSBO {
	int count;
	int primCount;
	int first;
	int baseInstance;
};

layout(std140, binding = 2) writeonly buffer TrianglesSSBO {
	vec4 pos[];
};

void main() {
	int idx = atomicAdd(count, 3);

	for (int i = 0; i < 3; ++i) {

		gl_Position = gl_in[i].gl_Position;
		gl_PrimitiveID = idx / 3;
		pos[idx + i] = vec4(gl_in[i].gl_Position.xyz, 1.0);

		EmitVertex();
	}

	EndPrimitive();
}