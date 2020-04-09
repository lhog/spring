/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "Rendering/GlobalRendering.h"
#include "Rendering/ShadowHandler.h"

#include "TessMeshShaders.h"

#include "System/Log/ILog.h"

#include <GL/glew.h>
#include <algorithm>
#include <cmath>

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
	#pragma message ( "TODO: recheck" __FILE__  __LINE__ )
	glDeleteShader(vsSO->GetObjID());
	glDeleteShader(tcsSO->GetObjID());
	glDeleteShader(tesSO->GetObjID());
	//glDeleteShader(fsSO->GetObjID());
}

void CTessMeshShader::BindTextures(const GLuint hmVarTexID) {
	glActiveTexture(GL_TEXTURE0);
	glGetIntegerv(GL_TEXTURE_BINDING_2D, &prevTexID);
	glBindTexture(GL_TEXTURE_2D, hmVarTexID);
}

void CTessMeshShader::UnbindTextures() {
	glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, prevTexID);
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

void CTessMeshShader::SetCommonUniforms() {
	bool alreadyBound = shaderPO->IsBound();

	if (!alreadyBound) shaderPO->Enable();

		shaderPO->SetUniform("screenDims", globalRendering->viewSizeX, globalRendering->viewSizeY);
		shaderPO->SetUniformMatrix4x4("shadowMat", false, shadowHandler.GetShadowMatrixRaw());

	if (!alreadyBound) shaderPO->Disable();
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
		shaderPO->SetUniform("hmVarTex", 0);
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
		shaderPO->SetUniform("hmVarTex", 0);
	shaderPO->Disable();
}

CTessMeshShaderSSBO::~CTessMeshShaderSSBO() {
	shaderHandler->ReleaseProgramObjects(poClass);
	glDeleteShader(gsSO->GetObjID());
}

/////////////////////////////////////////////////////////////////////////////////

CTessHMVariancePass::CTessHMVariancePass(const int hmX, const int hmY, const int tessLevelMax, const int lsx, const int lsy, const int lsz):
	hmX(hmX), hmY(hmY),
	tessLevelMax(tessLevelMax),
	lsx(lsx), lsy(lsy), lsz(lsz)
{
	assert(lsx * lsy * lsz <= 1024); //minimum GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS guaranteed by spec
	/////////////
	numMips = static_cast<int>(std::log2(tessLevelMax)) + 1;
	glGenTextures(1, &logTexID);

	GLint prevTexID;
	glActiveTexture(GL_TEXTURE0);
	glGetIntegerv(GL_TEXTURE_BINDING_2D, &prevTexID);
	glBindTexture(GL_TEXTURE_2D, logTexID);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, numMips - 1);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	if (GLEW_ARB_texture_storage && false)
		glTexStorage2D(GL_TEXTURE_2D, numMips, GL_R32F, hmX, hmY);
	else {
		for (int mipLvl = 0; mipLvl < numMips; ++mipLvl)
			glTexImage2D(GL_TEXTURE_2D, mipLvl, GL_R32F, std::max(hmX >> mipLvl, 1), std::max(hmY >> mipLvl, 1), 0, GL_RED, GL_FLOAT, nullptr);
	}

	glBindTexture(GL_TEXTURE_2D, prevTexID);

	/////////////
	hmvPO = shaderHandler->CreateProgramObject(poClass, "hmVar-GLSL", false);
	hmvSO = shaderHandler->CreateShaderObject("GLSL/HeightMapVarianceCSProg.glsl", "", GL_COMPUTE_SHADER);
	hmvPO->AttachShaderObject(hmvSO);
	hmvPO->SetFlag("LSX", lsx);
	hmvPO->SetFlag("LSY", lsy);
	hmvPO->SetFlag("LSZ", lsz);

	hmvPO->Link();
	hmvPO->Enable();
		hmvPO->SetUniform("hmTex", 0);
	hmvPO->Disable();
}

CTessHMVariancePass::~CTessHMVariancePass() {
	glDeleteTextures(1, &logTexID);

	shaderHandler->ReleaseProgramObjects(poClass);
	#pragma message ( "TODO: recheck" __FILE__  __LINE__ )
	glDeleteShader(hmvSO->GetObjID());
}

void CTessHMVariancePass::Run(const GLuint hmTexureID) {
	GLint prevTexID;

	glActiveTexture(GL_TEXTURE0);
	glGetIntegerv(GL_TEXTURE_BINDING_2D, &prevTexID);

	//keep UHM texture bound throughout whole hmvPO pass to make OpenGL Debug happier
	glBindTexture(GL_TEXTURE_2D, hmTexureID);
	hmvPO->Enable();

	/////
	// Implentation note. The below is slightly incorrect:
	// mipLvl == -1 is supposed to create centered even sized heightmap, instead of engine's hm{X,Y}+1 stupidity
	// mipLvl ==  0, should have same size as centered even sized heightmap, but due to mipmap requirements it's twice less
	// Since we run hiZ() pass later on and high frequency details are lost anyway, I hope twice less size won't break anything
	////

	for (int mipLvl = -1; mipLvl < numMips - 1; ++mipLvl) {
		hmvPO->SetUniform("mipLvl", mipLvl);

		if (mipLvl >= 0) {
			glBindImageTexture(0, logTexID, mipLvl, false, 0, GL_READ_ONLY, GL_R32F);
		}

		glBindImageTexture(1, logTexID, mipLvl + 1, false, 0, GL_WRITE_ONLY, GL_R32F);

		auto inImgWidth  = std::floor(static_cast<float>(hmX) / static_cast<float>(std::max(mipLvl + 1, 1))); //floor() per spec
		auto inImgHeight = std::floor(static_cast<float>(hmY) / static_cast<float>(std::max(mipLvl + 1, 1)));

		auto dispatchX = static_cast<GLuint>(ceil( inImgWidth / static_cast<float>(lsx)));
		auto dispatchY = static_cast<GLuint>(ceil(inImgHeight / static_cast<float>(lsy)));

		// LOG("glDispatchCompute %d %d %f %f %d %d %d %d", hmX, hmY, inImgWidth, inImgHeight, lsx, lsy, dispatchX, dispatchY);

		glDispatchCompute(dispatchX, dispatchY, 1);
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	};

	hmvPO->Disable();

	glBindTexture(GL_TEXTURE_2D, prevTexID);
}