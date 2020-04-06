/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _TESS_MESH_SHADERS_H_
#define _TESS_MESH_SHADERS_H_

#include "Rendering/GL/myGL.h"
#include "Rendering/Shaders/ShaderHandler.h"

#include <string>

class CTessMeshShader {
public:
	CTessMeshShader(const int mapX, const int mapZ);
	virtual ~CTessMeshShader();
public:
	virtual void Activate();
	virtual void SetSquareCoord(const int sx, const int sz);
	virtual void SetMaxTessValue(float maxTess);
	virtual void SetScreenDims();
	virtual void SetShadowMatrix();
	virtual void Deactivate();
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

class CTessMeshShaderTF : public CTessMeshShader {
public:
	CTessMeshShaderTF(const int mapX, const int mapZ);
	virtual ~CTessMeshShaderTF();
private:
	const std::string poClass = "[TessMeshDrawer-TF]";
};

class CTessMeshShaderSSBO : public CTessMeshShader {
public:
	CTessMeshShaderSSBO(const int mapX, const int mapZ);
	virtual ~CTessMeshShaderSSBO();
private:
	const std::string gsDef = "#define SSBO\n";
	const std::string poClass = "[TessMeshDrawer-SSBO]";
};

class CTessHMVarianceShader {
public:
	CTessHMVarianceShader(const GLuint logTexID, const int lsx = 32, const int lsy = 32, const int lsz = 1);
	~CTessHMVarianceShader();
	void Activate();
	void BindTextureLevel(const int lvl);
	void UnbindTextureLevel();
	void Deactivate();
private:
	GLint prevTexID;
	GLuint logTexID;

	int curMipLevel;

	int lsx, lsy, lsz;
	Shader::IShaderObject* hmvSO;
	Shader::IProgramObject* hmvPO;
	const std::string poClass = "[TessMeshDrawer-HMVar]";
};


#endif // _TESS_MESH_SHADERS_H_