/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "Rendering/GlobalRendering.h"
#include "Rendering/Shaders/Shader.h"
#include "Rendering/ShadowHandler.h"
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
	//fsSO = shaderHandler->CreateShaderObject("GLSL/TerrainMeshFragProg.glsl", "", GL_FRAGMENT_SHADER); //debug only
}

CTessMeshShader::~CTessMeshShader() {
	//seems like engine doesn't do it
	#pragma message ( "TODO: recheck", __FILE__, __LINE__ )
	glDeleteShader(vsSO->GetObjID());
	glDeleteShader(tcsSO->GetObjID());
	glDeleteShader(tesSO->GetObjID());
	//glDeleteShader(fsSO->GetObjID());
}

void CTessMeshShader::Activate() {
	glActiveTexture(GL_TEXTURE0);
	glGetIntegerv(GL_TEXTURE_BINDING_2D, &prevTexID);
	glBindTexture(GL_TEXTURE_2D, heightMapTexture->GetTextureID());
	shaderPO->Enable();
}

void CTessMeshShader::SetSquareCoord(const int sx, const int sz) {
	// shaderPO is supposed to be enabled
	shaderPO->SetUniform("texSquare", sx, sz);
}

void CTessMeshShader::SetMaxTessValue(float maxTess) {
	bool alreadyBound = shaderPO->IsBound();

	if (!alreadyBound) shaderPO->Enable();

	shaderPO->SetUniform("maxTessValue", maxTess);

	if (!alreadyBound) shaderPO->Disable();
}

void CTessMeshShader::SetScreenDims() {
	bool alreadyBound = shaderPO->IsBound();

	if (!alreadyBound) shaderPO->Enable();

	shaderPO->SetUniform("screenDims", globalRendering->viewSizeX, globalRendering->viewSizeY);

	if (!alreadyBound) shaderPO->Disable();
}

void CTessMeshShader::SetShadowMatrix() {
	bool alreadyBound = shaderPO->IsBound();

	if (!alreadyBound) shaderPO->Enable();

	shaderPO->SetUniformMatrix4x4("shadowMat", false, shadowHandler.GetShadowMatrixRaw());

	if (!alreadyBound) shaderPO->Disable();
}

void CTessMeshShader::Deactivate() {
	shaderPO->Disable();
	glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, prevTexID);
}

/////////////////////////////////////////////////////////////////////////////////

CTessMeshShaderTF::CTessMeshShaderTF(const int mapX, const int mapZ):
	CTessMeshShader(mapX, mapZ)
{
	gsSO = shaderHandler->CreateShaderObject("GLSL/TerrainMeshGeomProg.glsl", "", GL_GEOMETRY_SHADER);

	shaderPO = shaderHandler->CreateProgramObject(poClass, "TF-GLSL", false);

	shaderPO->AttachShaderObject(vsSO);
	shaderPO->AttachShaderObject(tcsSO);
	shaderPO->AttachShaderObject(tesSO);
	shaderPO->AttachShaderObject(gsSO);
	//shaderPO->AttachShaderObject(fsSO);

	const char* tfVarying = "vPosTF";
	glTransformFeedbackVaryings(shaderPO->GetObjID(), 1, &tfVarying, GL_INTERLEAVED_ATTRIBS);

	//shaderPO->SetFlag("SSBO", 0);

	shaderPO->Link();

	shaderPO->Enable();
	shaderPO->SetUniform("mapDims", mapX, mapZ);
	shaderPO->SetUniform("heightMap", 0);
	shaderPO->Disable();
}

CTessMeshShaderTF::~CTessMeshShaderTF() {
	shaderHandler->ReleaseProgramObjects(poClass);
	glDeleteShader(gsSO->GetObjID());
}

/////////////////////////////////////////////////////////////////////////////////

CTessMeshShaderSSBO::CTessMeshShaderSSBO(const int mapX, const int mapZ) :
	CTessMeshShader(mapX, mapZ)
{
	gsSO = shaderHandler->CreateShaderObject("GLSL/TerrainMeshGeomProg.glsl", gsDef, GL_GEOMETRY_SHADER);

	shaderPO = shaderHandler->CreateProgramObject(poClass, "SSBO-GLSL", false);

	shaderPO->AttachShaderObject(vsSO);
	shaderPO->AttachShaderObject(tcsSO);
	shaderPO->AttachShaderObject(tesSO);
	shaderPO->AttachShaderObject(gsSO);
	//shaderPO->AttachShaderObject(fsSO);

	//shaderPO->SetFlag("SSBO", 1);

	shaderPO->Link();

	shaderPO->Enable();
	shaderPO->SetUniform("mapDims", mapX, mapZ);
	shaderPO->SetUniform("heightMap", 0);
	shaderPO->Disable();
}

CTessMeshShaderSSBO::~CTessMeshShaderSSBO() {
	shaderHandler->ReleaseProgramObjects(poClass);
	glDeleteShader(gsSO->GetObjID());
}

CTessHMVarianceShader::CTessHMVarianceShader(const GLuint logTexID, const int lsx, const int lsy, const int lsz):
	logTexID(logTexID),
	lsx(lsx), lsy(lsy), lsz(lsz)
{
	hmvSO = shaderHandler->CreateShaderObject("GLSL/HMVarianceCSProg.glsl", "", GL_COMPUTE_SHADER);
	hmvPO->AttachShaderObject(hmvSO);
	hmvPO->SetFlag("LSX", lsx);
	hmvPO->SetFlag("LSY", lsy);
	hmvPO->SetFlag("LSZ", lsz);

	hmvPO->Link();
}

CTessHMVarianceShader::~CTessHMVarianceShader() {
	shaderHandler->ReleaseProgramObjects(poClass);
	#pragma message ( "TODO: recheck", __FILE__, __LINE__ )
	glDeleteShader(hmvSO->GetObjID());
}

void CTessHMVarianceShader::Activate() {
	glActiveTexture(GL_TEXTURE0);
	glGetIntegerv(GL_TEXTURE_BINDING_2D, &prevTexID);
	glBindTexture(GL_TEXTURE_2D, logTexID);
	hmvPO->Enable();
}

void CTessHMVarianceShader::BindTextureLevel(const int lvl) {
	curMipLevel = lvl;

	if (curMipLevel == -1) {
		glActiveTexture(GL_TEXTURE0);
		glGetIntegerv(GL_TEXTURE_BINDING_2D, &prevTexID);
		glBindTexture(GL_TEXTURE_2D, heightMapTexture->GetTextureID()); //source
	} else {
		glBindImageTexture(0, logTexID, curMipLevel, GL_FALSE, 0, GL_READ_ONLY, GL_R32F); //source
	}

	glBindImageTexture(1, logTexID, curMipLevel + 1, GL_FALSE, 0, GL_WRITE_ONLY, GL_R32F); //destination

	bool alreadyBound = hmvPO->IsBound();
	if (!alreadyBound) hmvPO->Enable();

	hmvPO->SetUniform("lvl", curMipLevel);

	if (!alreadyBound) hmvPO->Disable();
}

void CTessHMVarianceShader::UnbindTextureLevel() {
	if (curMipLevel == -1) {
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, prevTexID); //source
	} else {
		glBindImageTexture(0, 0, curMipLevel, GL_FALSE, 0, GL_READ_ONLY, GL_R32F); //source
	}

	glBindImageTexture(1, 0, curMipLevel + 1, GL_FALSE, 0, GL_WRITE_ONLY, GL_R32F); //destination
}

void CTessHMVarianceShader::Deactivate() {
	hmvPO->Disable();
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, prevTexID);
}
