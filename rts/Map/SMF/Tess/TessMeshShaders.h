/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _TESS_MESH_SHADERS_H_
#define _TESS_MESH_SHADERS_H_

#include "Rendering/GL/myGL.h"
#include "Rendering/Shaders/ShaderHandler.h"

#include <string>

const char* tcsSrc = R"####(
	#version 150 compatibility

	#extension GL_ARB_tessellation_shader : require

	layout(vertices = 3) out;

	uniform float tessLevel;

	void main(void)
	{
		gl_TessLevelOuter[0] = float(tessLevel);
		gl_TessLevelOuter[1] = gl_TessLevelOuter[0];
		gl_TessLevelOuter[2] = gl_TessLevelOuter[0];

		gl_TessLevelInner[0] = gl_TessLevelOuter[0];

		gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
	}
)####";

const char* tesSrc = R"####(
	#version 150 compatibility

	#extension GL_ARB_tessellation_shader : require

	layout( triangles, equal_spacing, ccw ) in;

	uniform int tessLevel;
	uniform float time;

	out vec4 vPosTF;


    void main()
    {
		gl_Position = gl_TessCoord.x * gl_in[0].gl_Position +
					  gl_TessCoord.y * gl_in[1].gl_Position +
					  gl_TessCoord.z * gl_in[2].gl_Position;
		gl_Position.x += 0.1 * sin(3.0 * time + gl_Position.y);
		vPosTF = gl_Position;
    }
)####";


const char* gsSrc = R"####(
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
)####";



///////////////////////////////////////////////////////////////////////

class ITessMeshShader {
public:
	virtual void Init();
	virtual void Finalize();
	//virtual Shader::IProgramObject* GetProgramObject() { return shaderPO; }
	virtual void Activate();
	virtual void SetSquareCoord(int sx, int sy);
	virtual void Deactivate();
protected:
	Shader::IShaderObject* vsSO;
	Shader::IShaderObject* fsSO;
	Shader::IShaderObject* tesSO;
	Shader::IShaderObject* tcsSO;

	Shader::IProgramObject* shaderPO;
};

class CTessMeshShaderTF : public ITessMeshShader {
public:
	// Inherited via ITessMeshShader
	virtual void Init() override;
	virtual void Finalize() override;
private:
	const std::string poClass = "[TessMeshDrawer]";
};


#endif // _TESS_MESH_SHADERS_H_