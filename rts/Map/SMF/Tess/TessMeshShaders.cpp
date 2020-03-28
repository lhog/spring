/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "Rendering/Shaders/Shader.h"
#include "Map/HeightMapTexture.h"
#include "TessMeshShaders.h"


void ITessMeshShader::Init() {
	vsSO = shaderHandler->CreateShaderObject("GLSL/TerrainMeshVertProg.glsl", "", GL_VERTEX_SHADER);
	fsSO = shaderHandler->CreateShaderObject("GLSL/TerrainMeshFragProg.glsl", "", GL_FRAGMENT_SHADER);
	tcsSO = shaderHandler->CreateShaderObject("GLSL/TerrainMeshTCSProg.glsl", "", GL_TESS_CONTROL_SHADER);
	tesSO = shaderHandler->CreateShaderObject("GLSL/TerrainMeshTESProg.glsl", "", GL_TESS_EVALUATION_SHADER);
}

void ITessMeshShader::Finalize() {
}

void ITessMeshShader::Activate() {
	glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, heightMapTexture->GetTextureID());
	shaderPO->Enable();
}

void ITessMeshShader::SetSquareCoord(int sx, int sy) {
	shaderPO->SetUniform("squareCoords", sx, sy);
}

void ITessMeshShader::Deactivate() {
	shaderPO->Disable();
	glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, 0);
}

void CTessMeshShaderTF::Init() {
	shaderPO = shaderHandler->CreateProgramObject(poClass, "TF-GLSL", false);

	shaderPO->AttachShaderObject(vsSO);
	shaderPO->AttachShaderObject(tcsSO);
	shaderPO->AttachShaderObject(tesSO);
	shaderPO->AttachShaderObject(fsSO);


	const char* tfVarying = "vPosTF";
	glTransformFeedbackVaryings(shaderPO->GetObjID(), 1, &tfVarying, GL_INTERLEAVED_ATTRIBS);

	shaderPO->Link();

	shaderPO->Enable();
	shaderPO->SetUniform("heightMap", 0);
	shaderPO->Disable();
}

void CTessMeshShaderTF::Finalize() {
	shaderHandler->ReleaseProgramObjects(poClass);
}

//gsSO = shaderHandler->CreateShaderObject("", "", GL_GEOMETRY_SHADER);