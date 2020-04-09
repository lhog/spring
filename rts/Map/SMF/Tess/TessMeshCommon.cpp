/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <assert.h>
#include <algorithm>

#include <GL/glew.h>

#include "TessMeshCommon.h"
#include "Rendering/Shaders/Shader.h"
#include "Rendering/Shaders/ShaderHandler.h"
#include "Map/HeightMapTexture.h"
#include "System/Log/ILog.h"

CTessMeshCacheTF::CTessMeshCacheTF(const int hmX, const int hmZ):
	CTessMeshCache(hmX, hmZ, GL_TRANSFORM_FEEDBACK_BUFFER)
{
	LOG("CTessMeshCacheTF");
	meshTessTFOs.resize(numPatchesX * numPatchesZ);

	for (auto i = 0; i < numPatchesX * numPatchesZ; ++i) {
		glGenTransformFeedbacks(1, &meshTessTFOs[i]);

		glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, meshTessTFOs[i]);

		glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, meshTessVBOs[i]);

		glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, 0);
	}
	tessMeshShader = std::unique_ptr<CTessMeshShaderTF>(new CTessMeshShaderTF(SQUARE_SIZE * mapDims.mapx, SQUARE_SIZE * mapDims.mapy));
	tessMeshShader->SetMaxTessValue(TeshMessConsts::TESS_LEVEL);

	LOG("CTessMeshCacheTF 2");
}

CTessMeshCacheTF::~CTessMeshCacheTF() {
	for (const auto meshTessTFO : meshTessTFOs) {
		glDeleteTransformFeedbacks(1, &meshTessTFO);
	}
}

void CTessMeshCacheTF::Update(){
	if (!cameraMoved && std::find(hmChanged.begin(), hmChanged.end(), true) == hmChanged.end()) //nothing to do
		return;

	GLuint tfQuery;
	glGenQueries(1, &tfQuery);

	glEnableClientState(GL_VERTEX_ARRAY);
	//glEnable(GL_RASTERIZER_DISCARD);

	glBindBuffer(GL_ARRAY_BUFFER, meshTemplateVBO);
	glVertexPointer(3, GL_FLOAT, 0, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glPatchParameteri(GL_PATCH_VERTICES, 4); //quads per patch

	tessMeshShader->Activate();
	tessMeshShader->SetCommonUniforms();

	for (int i = 0; i < numPatchesX * numPatchesZ; ++i) {
		if (hmChanged[0] || hmChanged[i + 1]) {
			int x = i / numPatchesZ;
			int z = i % numPatchesZ;

			//LOG("SetSquareCoord(%d, %d)", x, z);
			tessMeshShader->SetSquareCoord(x, z);

			//GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN
			//glBeginQuery(GL_PRIMITIVES_GENERATED, tfQuery);

			//glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, meshTessVBOs[i]);
			//glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, meshTessTFOs[i]);
			//glBeginTransformFeedback(GL_TRIANGLES);

			glDrawArrays(GL_PATCHES, 0, TeshMessConsts::PATCH_VERT_NUM);

			//glEndTransformFeedback();
			//glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, 0);
			//glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, 0);

			//glEndQuery(GL_PRIMITIVES_GENERATED);

			//GLuint numPrimitivesWritten = 0u;
			//glGetQueryObjectuiv(tfQuery, GL_QUERY_RESULT, &numPrimitivesWritten); //in triangles
			//LOG("PatchID = %d numPrimitivesWritten = %d", i, numPrimitivesWritten);

			hmChanged[i + 1] = false;
		}
	}

	hmChanged[0] = false;

	//glDisable(GL_RASTERIZER_DISCARD);
	glDisableClientState(GL_VERTEX_ARRAY);

	tessMeshShader->Deactivate();

	glDeleteQueries(1, &tfQuery);

	glPatchParameteri(GL_PATCH_VERTICES, 3); // restore default
}

void CTessMeshCacheTF::Reset(){
}

void CTessMeshCacheTF::DrawMesh(const int px, const int pz){
	const int idx = px * numPatchesZ + pz;
	glEnableClientState(GL_VERTEX_ARRAY);

	glBindBuffer(GL_ARRAY_BUFFER, meshTessVBOs[idx]);
		glVertexPointer(3, GL_FLOAT, 0, (void*)0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glDrawTransformFeedback(GL_TRIANGLES, meshTessTFOs[idx]);

	glDisableClientState(GL_VERTEX_ARRAY);
}

/////////////////////////////////////////////////////////////////////////////////

CTessMeshCacheSSBO::CTessMeshCacheSSBO(const int hmX, const int hmZ) :
	CTessMeshCache(hmX, hmZ, GL_SHADER_STORAGE_BUFFER)
{
	drawIndirect = GLEW_ARB_draw_indirect;

	meshTessDAIBs.resize(numPatchesX * numPatchesZ);

	for (auto i = 0; i < numPatchesX * numPatchesZ; ++i) {
		glGenBuffers(1, &meshTessDAIBs[i]);
		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, meshTessDAIBs[i]);
		glBufferData(GL_DRAW_INDIRECT_BUFFER, sizeof(DrawArraysIndirectCommand), nullptr, GL_STREAM_COPY);
		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
	}

	memset(&daicZero, 0, sizeof(DrawArraysIndirectCommand));
	daicZero.primCount = 1u;

	tessMeshShader = std::unique_ptr<CTessMeshShaderSSBO>(new CTessMeshShaderSSBO(SQUARE_SIZE * mapDims.mapx, SQUARE_SIZE * mapDims.mapy));
	tessMeshShader->SetMaxTessValue(TeshMessConsts::TESS_LEVEL);

	tessHMVarPass = std::unique_ptr<CTessHMVariancePass>(new CTessHMVariancePass(hmX, hmZ, TeshMessConsts::TESS_LEVEL));
}

CTessMeshCacheSSBO::~CTessMeshCacheSSBO(){
	for (auto i = 0; i < numPatchesX * numPatchesZ; ++i) {
		glDeleteBuffers(1, &meshTessDAIBs[i]);
	}
}

void CTessMeshCacheSSBO::Update(){
	if (!cameraMoved && std::find(hmChanged.begin(), hmChanged.end(), true) == hmChanged.end()) //nothing to do
		return;

	/////////////////////////////////////////
	tessHMVarPass->Run(heightMapTexture->GetTextureID());
	/////////////////////////////////////////

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnable(GL_RASTERIZER_DISCARD);
	glDisable(GL_CULL_FACE);

	glBindBuffer(GL_ARRAY_BUFFER, meshTemplateVBO);
	glVertexPointer(3, GL_FLOAT, 0, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glPatchParameteri(GL_PATCH_VERTICES, 4); //quads per patch

	tessMeshShader->Activate();
	tessMeshShader->BindTextures(tessHMVarPass->GetTextureID());
	//tessMeshShader->BindTextures(heightMapTexture->GetTextureID());
	tessMeshShader->SetCommonUniforms();

	GLuint npwNow = 0u;
	GLuint npwTotal = 0u;

	for (int i = 0; i < numPatchesX * numPatchesZ; ++i) {
		if (hmChanged[0] || hmChanged[i + 1]) {
			int x = i / numPatchesZ;
			int z = i % numPatchesZ;

			tessMeshShader->SetSquareCoord(x, z);

			glBindBuffer(GL_DRAW_INDIRECT_BUFFER, meshTessDAIBs[i]);
			glBufferSubData(GL_DRAW_INDIRECT_BUFFER, 0, sizeof(DrawArraysIndirectCommand), &daicZero);
			glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);

			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, meshTessDAIBs[i]);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, meshTessVBOs[i]);

			if (runQueries)
				glBeginQuery(GL_PRIMITIVES_GENERATED, tessMeshQuery);

			glDrawArrays(GL_PATCHES, 0, TeshMessConsts::PATCH_VERT_NUM);
			glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

			if (runQueries) {
				glEndQuery(GL_PRIMITIVES_GENERATED);
				glGetQueryObjectuiv(tessMeshQuery, GL_QUERY_RESULT, &npwNow); //in triangles
				npwTotal += npwNow;
				/*
				DrawArraysIndirectCommand daicTmp;
				glBindBuffer(GL_DRAW_INDIRECT_BUFFER, meshTessDAIBs[i]);
				glGetBufferSubData(GL_DRAW_INDIRECT_BUFFER, 0, sizeof(DrawArraysIndirectCommand), &daicTmp);
				glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);

				LOG("daicTmp[%d] = %d || %d", i, daicTmp.count, daicTmp.primCount);
				*/
			}

			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, 0);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, 0);

			hmChanged[i + 1] = false;
		}
	}

	if (runQueries && hmChanged[0])
		LOG("[Tesselation Shader:SSBO-Backend] Total Number of primitives written: %d", npwTotal);

	hmChanged[0] = false;

	glDisable(GL_RASTERIZER_DISCARD);
	glEnable(GL_CULL_FACE);
	glDisableClientState(GL_VERTEX_ARRAY);

	tessMeshShader->Deactivate();
	tessMeshShader->UnbindTextures();

	glPatchParameteri(GL_PATCH_VERTICES, 3); // restore default
}

void CTessMeshCacheSSBO::Reset(){
}

void CTessMeshCacheSSBO::DrawMesh(const int px, const int pz){
	const int idx = px * numPatchesZ + pz;

	glEnableClientState(GL_VERTEX_ARRAY);

	glBindBuffer(GL_ARRAY_BUFFER, meshTessVBOs[idx]);
	glVertexPointer(3, GL_FLOAT, 0, (void*)0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glBindBuffer(GL_DRAW_INDIRECT_BUFFER, meshTessDAIBs[idx]);

//	drawIndirect = false;

	if (drawIndirect) {
		glDrawArraysIndirect(GL_TRIANGLES, 0);
		glMemoryBarrier(GL_COMMAND_BARRIER_BIT);
	} else {
		DrawArraysIndirectCommand daicTmp;
		glGetBufferSubData(GL_DRAW_INDIRECT_BUFFER, 0, sizeof(DrawArraysIndirectCommand), &daicTmp);
		glDrawArrays(GL_TRIANGLES, 0, daicTmp.count);
//		LOG("Drawing[%d,%d] patch. Vertices Count = %d, Primitives Count = %d", px, pz, daicTmp.count, daicTmp.primCount);
	}

	glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);

	glDisableClientState(GL_VERTEX_ARRAY);
}

void CTessMeshCache::CameraMoved() {
}

void CTessMeshCache::TesselatePatch(const int px, const int pz) {
	hmChanged[px * numPatchesZ + pz + 1] = true;
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

CTessMeshCache::CTessMeshCache(const int hmX, const int hmZ, const GLenum meshTessBufferType) :
	hmX(hmX),
	hmZ(hmZ),
	numPatchesX(hmX / TeshMessConsts::PATCH_SIZE),
	numPatchesZ(hmZ / TeshMessConsts::PATCH_SIZE),
	runQueries(false)
{
	LOG("CTessMeshCache");
	//meshTemplate = std::make_unique<MeshPatches>();
	meshTemplate = std::unique_ptr<MeshPatches>(new MeshPatches());

	FillMeshTemplateBuffer();

	glGenBuffers(1, &meshTemplateVBO);

	glBindBuffer(GL_ARRAY_BUFFER, meshTemplateVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(MeshPatches), meshTemplate.get(), GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glGenQueries(1, &tessMeshQuery);

	meshTessVBOs.resize(numPatchesX * numPatchesZ);
	hmChanged.resize(numPatchesX * numPatchesZ, true);

	for (auto& meshTessVBO : meshTessVBOs) {
		glGenBuffers(1, &meshTessVBO);

		glBindBuffer(meshTessBufferType, meshTessVBO);
		glBufferData(meshTessBufferType, TeshMessConsts::TESS_TRIANGLE_NUM_MAX * sizeof(MeshTessTriangle), nullptr, GL_DYNAMIC_COPY);
	}
	glBindBuffer(meshTessBufferType, 0); //unbind once

	LOG("CTessMeshCache 2");
}

CTessMeshCache::~CTessMeshCache() {
	glDeleteBuffers(1, &meshTemplateVBO);

	glDeleteQueries(1, &tessMeshQuery);

	for (const auto meshTessVBO : meshTessVBOs) {
		glDeleteBuffers(1, &meshTessVBO);
	}
}