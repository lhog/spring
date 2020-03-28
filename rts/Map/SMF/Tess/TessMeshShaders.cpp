/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "Rendering/Shaders/Shader.h"
#include "Map/HeightMapTexture.h"
#include "TessMeshShaders.h"

#include "System/Log/ILog.h"


CTessMeshShader::CTessMeshShader(const int mapX, const int mapZ) :
	mapX(mapX),
	mapZ(mapZ)
{
	vsSO = shaderHandler->CreateShaderObject("GLSL/TerrainMeshVertProg.glsl", "", GL_VERTEX_SHADER);
	tcsSO = shaderHandler->CreateShaderObject("GLSL/TerrainMeshTCSProg.glsl", "", GL_TESS_CONTROL_SHADER);
	tesSO = shaderHandler->CreateShaderObject("GLSL/TerrainMeshTESProg.glsl", "", GL_TESS_EVALUATION_SHADER);
	fsSO = shaderHandler->CreateShaderObject("GLSL/TerrainMeshFragProg.glsl", "", GL_FRAGMENT_SHADER); //debug only
}

CTessMeshShader::~CTessMeshShader() {
	//seems like engine doesn't do it
	glDeleteShader(vsSO->GetObjID());
	glDeleteShader(tcsSO->GetObjID());
	glDeleteShader(tesSO->GetObjID());
	glDeleteShader(fsSO->GetObjID());
}

void CTessMeshShader::Activate() {
	glActiveTexture(GL_TEXTURE0);
	glGetIntegerv(GL_TEXTURE_BINDING_2D, &prevTexID);
	glBindTexture(GL_TEXTURE_2D, heightMapTexture->GetTextureID());
	shaderPO->Enable();
}

void CTessMeshShader::SetSquareCoord(const int sx, const int sz) {
	shaderPO->SetUniform("texSquare", sx, sz);
}

void CTessMeshShader::Deactivate() {
	shaderPO->Disable();
	glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, prevTexID);
}
//gsSO = shaderHandler->CreateShaderObject("GLSL/TerrainMeshGSProg.glsl", "", GL_TESS_EVALUATION_SHADER);

CTessMeshShaderTF::CTessMeshShaderTF(const int mapX, const int mapZ):
	CTessMeshShader(mapX, mapZ)
{
	shaderPO = shaderHandler->CreateProgramObject(poClass, "TF-GLSL", false);

	shaderPO->AttachShaderObject(vsSO);
	shaderPO->AttachShaderObject(tcsSO);
	shaderPO->AttachShaderObject(tesSO);
	shaderPO->AttachShaderObject(fsSO);

	//const char* tfVarying = "vPosTF";
	//glTransformFeedbackVaryings(shaderPO->GetObjID(), 1, &tfVarying, GL_INTERLEAVED_ATTRIBS);

	shaderPO->Link();

	shaderPO->Enable();
	shaderPO->SetUniform("mapDims", mapX, mapZ);
	shaderPO->SetUniform("heightMap", 0);
	shaderPO->Disable();
}

CTessMeshShaderTF::~CTessMeshShaderTF() {
	shaderHandler->ReleaseProgramObjects(poClass);
}
