/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <assert.h>
#include <algorithm>

#include <GL/glew.h>

#include "TessMeshCommon.h"
#include "Rendering/Shaders/Shader.h"
#include "Rendering/Shaders/ShaderHandler.h"

CTessMeshCacheTF::CTessMeshCacheTF(){
}

CTessMeshCacheTF::~CTessMeshCacheTF(){
}

void CTessMeshCacheTF::Init(const int nPX, const int nPY, const GLenum meshTessBufferType) {
	CTessMeshCache::Init(nPX, nPY, meshTessBufferType);
	meshTessTFOs.resize(numPatchesX * numPatchesY);

	for (auto i = 0; i < numPatchesX * numPatchesY; ++i) {
		glGenTransformFeedbacks(1, &meshTessTFOs[i]);

		glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, meshTessTFOs[i]);

			glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, meshTessVBOs[i]);

		glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, 0);
	}

	tessMeshShader = CTessMeshShaderTF();
}

void CTessMeshCacheTF::Finalize(){
	for (const auto meshTessTFO : meshTessTFOs) {
		glDeleteTransformFeedbacks(1, &meshTessTFO);
	}
	CTessMeshCache::Finalize();
}

void CTessMeshCacheTF::Update(){
	if (std::find(tessMeshDirty.begin(), tessMeshDirty.end(), true) == tessMeshDirty.end()) //nothing to do
		return;

	tessMeshShader.Activate();

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnable(GL_RASTERIZER_DISCARD);

	glBindBuffer(GL_ARRAY_BUFFER, meshTemplateVBO);
	glVertexPointer(3, GL_FLOAT, 0, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	for (int i = 0; i < numPatchesX * numPatchesY; ++i) {
		int x = i / numPatchesY;
		int y = i % numPatchesY;
		//glUniform1f(loc2, time);
		tessMeshShader.SetSquareCoord(x, y);

		glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, meshTessTFOs[i]);
			glBeginTransformFeedback(GL_TRIANGLES);

				glDrawArrays(GL_PATCHES, 0, TeshMessConsts::PATCH_VERT_NUM);

			glEndTransformFeedback();
		glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, 0);
	}

	glDisable(GL_RASTERIZER_DISCARD);
	glDisableClientState(GL_VERTEX_ARRAY);

	tessMeshShader.Deactivate();
}

void CTessMeshCacheTF::Reset(){
}

void CTessMeshCacheTF::DrawMesh(const int px, const int py){
	const int idx = px * numPatchesY + py;
	glEnableClientState(GL_VERTEX_ARRAY);

	glBindBuffer(GL_ARRAY_BUFFER, meshTessVBOs[idx]);
		glVertexPointer(3, GL_FLOAT, 0, (void*)0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glDrawTransformFeedback(GL_TRIANGLES, meshTessTFOs[idx]);

	glDisableClientState(GL_VERTEX_ARRAY);
}

/////////////////////////////////////////////////////////////////////////////////

CTessMeshCacheSSBO::CTessMeshCacheSSBO(){
	drawIndirect = GLEW_ARB_draw_indirect;
	//glMemoryBarrier()
	//glGenBuffers
}

CTessMeshCacheSSBO::~CTessMeshCacheSSBO(){
}

void CTessMeshCacheSSBO::Init(const int nPX, const int nPY, const GLenum meshTessBufferType) {
}

void CTessMeshCacheSSBO::Finalize(){
}

void CTessMeshCacheSSBO::Update(){
}

void CTessMeshCacheSSBO::Reset(){
}

void CTessMeshCacheSSBO::DrawMesh(const int px, const int py){
}

void CTessMeshCache::RequestTesselation() {
	std::fill(tessMeshDirty.begin(), tessMeshDirty.end(), true);
}

void CTessMeshCache::RequestTesselation(const int px, const int py) {
	tessMeshDirty[px * numPatchesY + py] = true;
}

void CTessMeshCache::FillMeshTemplateBuffer() {
	uint32_t patchVertIdx = 0;
	for (int i = 0; i < TeshMessConsts::PATCH_RC_QUAD_NUM; ++i)
		for (int j = 0; j < TeshMessConsts::PATCH_RC_QUAD_NUM; ++j) {
			// CCW
			meshTemplate->vertices[patchVertIdx++] = float3((i + 0) * SQUARE_SIZE * TeshMessConsts::TESS_LEVEL, 0.0f, (j + 0) * SQUARE_SIZE * TeshMessConsts::TESS_LEVEL); //TL
			meshTemplate->vertices[patchVertIdx++] = float3((i + 0) * SQUARE_SIZE * TeshMessConsts::TESS_LEVEL, 0.0f, (j + 1) * SQUARE_SIZE * TeshMessConsts::TESS_LEVEL); //BL
			meshTemplate->vertices[patchVertIdx++] = float3((i + 1) * SQUARE_SIZE * TeshMessConsts::TESS_LEVEL, 0.0f, (j + 1) * SQUARE_SIZE * TeshMessConsts::TESS_LEVEL); //BR
			meshTemplate->vertices[patchVertIdx++] = float3((i + 1) * SQUARE_SIZE * TeshMessConsts::TESS_LEVEL, 0.0f, (j + 0) * SQUARE_SIZE * TeshMessConsts::TESS_LEVEL); //TR
		}
	assert(patchVertIdx == TeshMessConsts::PATCH_VERT_NUM);
}

void CTessMeshCache::Init(const int nPX, const int nPY, const GLenum meshTessBufferType) {

	this->numPatchesX = nPX;
	this->numPatchesY = nPY;

	meshTemplate = std::make_unique<MeshPatches>();

	FillMeshTemplateBuffer();

	glGenBuffers(1, &meshTemplateVBO);

	glBindBuffer(GL_ARRAY_BUFFER, meshTemplateVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(MeshPatches), meshTemplate.get(), GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	meshTessVBOs.resize(numPatchesX * numPatchesY);
	tessMeshDirty.resize(numPatchesX * numPatchesY);

	RequestTesselation();

	for (auto& meshTessVBO: meshTessVBOs) {
		glGenBuffers(1, &meshTessVBO);

		glBindBuffer(meshTessBufferType, meshTessVBO);
		glBufferData(meshTessBufferType, TeshMessConsts::TESS_TRIANGLE_NUM_MAX * sizeof(MeshTessTriangle), nullptr, GL_DYNAMIC_COPY);
	}
	glBindBuffer(meshTessBufferType, 0); //unbind once
}

void CTessMeshCache::Finalize() {
	glDeleteBuffers(1, &meshTemplateVBO);

	for (const auto meshTessVBO : meshTessVBOs) {
		glDeleteBuffers(1, &meshTessVBO);
	}
}