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
	virtual void Deactivate();
protected:
	const int mapX;
	const int mapZ;

	Shader::IShaderObject* vsSO;
	Shader::IShaderObject* tesSO;
	Shader::IShaderObject* tcsSO;
	Shader::IShaderObject* fsSO;

	Shader::IProgramObject* shaderPO;

	GLint prevTexID;
};

class CTessMeshShaderTF : public CTessMeshShader {
public:
	CTessMeshShaderTF(const int mapX, const int mapZ);
	virtual ~CTessMeshShaderTF();
private:
	const std::string poClass = "[TessMeshDrawer]";
};


#endif // _TESS_MESH_SHADERS_H_