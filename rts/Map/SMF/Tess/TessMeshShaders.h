/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _TESS_MESH_SHADERS_H_
#define _TESS_MESH_SHADERS_H_

#include "Rendering/GL/myGL.h"
#include "Rendering/Shaders/Shader.h"
#include "Rendering/Shaders/ShaderHandler.h"

#include <string>

class CTessMeshShader {
public:
	CTessMeshShader(const int mapX, const int mapZ);
	virtual ~CTessMeshShader();
public:
	virtual void Activate() { shaderPO->Enable(); }
	virtual void Deactivate() { shaderPO->Disable(); }

	virtual void BindTextures(const GLuint hmVarTexID);
	virtual void UnbindTextures();

	virtual void SetSquareCoord(const int sx, const int sz);
	virtual void SetMaxTessValue(int maxTess);
	virtual void SetCommonUniforms();
protected:
	const int mapX;
	const int mapZ;

	Shader::IShaderObject* vsSO;
	Shader::IShaderObject* tesSO;
	Shader::IShaderObject* tcsSO;
	Shader::IShaderObject* gsSO;
	//Shader::IShaderObject* fsSO;

	Shader::IProgramObject* shaderPO;

	GLint prevTexID;
};

class CTessMeshShaderSSBO : public CTessMeshShader {
public:
	CTessMeshShaderSSBO(const int mapX, const int mapZ);
	virtual ~CTessMeshShaderSSBO();
private:
	const std::string poClass = "[TessMeshDrawer-SSBO]";
};

class CTessHMVariancePass {
public:
	CTessHMVariancePass(const int hmX, const int hmY, const int tessLevelMax, const int lsx = 32, const int lsy = 32, const int lsz = 1);
	~CTessHMVariancePass();
	void Run(const GLuint hmTexureID);
	GLuint GetTextureID() { return logTexID; };
private:
	GLint prevTexID;
	GLuint logTexID;

	int numMips;

	const int hmX;
	const int hmY;
	const int tessLevelMax;
	const int lsx, lsy, lsz;

	Shader::IShaderObject* hmvSO;
	Shader::IProgramObject* hmvPO;
	const std::string poClass = "[TessMeshDrawer-hmVar]";
};


#endif // _TESS_MESH_SHADERS_H_