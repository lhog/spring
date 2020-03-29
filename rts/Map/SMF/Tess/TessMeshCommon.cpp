/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <assert.h>
#include <algorithm>

#include <GL/glew.h>

#include "TessMeshCommon.h"
#include "Rendering/Shaders/Shader.h"
#include "Rendering/Shaders/ShaderHandler.h"
#include "System/Log/ILog.h"

CTessMeshCacheTF::CTessMeshCacheTF(const int numPatchesX, const int numPatchesZ):
	CTessMeshCache(numPatchesX, numPatchesZ, GL_TRANSFORM_FEEDBACK_BUFFER)
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

	LOG("CTessMeshCacheTF 2");
}

CTessMeshCacheTF::~CTessMeshCacheTF() {
	for (const auto meshTessTFO : meshTessTFOs) {
		glDeleteTransformFeedbacks(1, &meshTessTFO);
	}
}

void CTessMeshCacheTF::Update(){
	if (std::find(tessMeshDirty.begin(), tessMeshDirty.end(), true) == tessMeshDirty.end()) //nothing to do
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
	tessMeshShader->SetScreenDims();

	for (int i = 0; i < numPatchesX * numPatchesZ; ++i) {
		if (tessMeshDirty[0] || tessMeshDirty[i + 1]) {
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
		}
	}

	//glDisable(GL_RASTERIZER_DISCARD);
	glDisableClientState(GL_VERTEX_ARRAY);

	tessMeshShader->Deactivate();

	glDeleteQueries(1, &tfQuery);

	glPatchParameteri(GL_PATCH_VERTICES, 3); // restore default
}

void CTessMeshCacheTF::Reset(){
}

void CTessMeshCacheTF::DrawMesh(const int px, const int py){
	const int idx = px * numPatchesZ + py;
	glEnableClientState(GL_VERTEX_ARRAY);

	glBindBuffer(GL_ARRAY_BUFFER, meshTessVBOs[idx]);
		glVertexPointer(3, GL_FLOAT, 0, (void*)0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glDrawTransformFeedback(GL_TRIANGLES, meshTessTFOs[idx]);

	glDisableClientState(GL_VERTEX_ARRAY);
}

/////////////////////////////////////////////////////////////////////////////////

CTessMeshCacheSSBO::CTessMeshCacheSSBO(const int numPatchesX, const int numPatchesZ) :
	CTessMeshCache(numPatchesX, numPatchesZ, GL_SHADER_STORAGE_BUFFER)
{
	drawIndirect = GLEW_ARB_draw_indirect;
	//glMemoryBarrier()
	//glGenBuffers
}

CTessMeshCacheSSBO::~CTessMeshCacheSSBO(){
}

void CTessMeshCacheSSBO::Update(){
}

void CTessMeshCacheSSBO::Reset(){
}

void CTessMeshCacheSSBO::DrawMesh(const int px, const int py){
}

void CTessMeshCache::RequestTesselation() {
	//std::fill(tessMeshDirty.begin(), tessMeshDirty.end(), true);
	//LOG("RequestTesselation");
	tessMeshDirty[0] = true;
}

void CTessMeshCache::RequestTesselation(const int px, const int py) {
	LOG("RequestTesselation {%d, %d}", px, py);
	tessMeshDirty[px * numPatchesZ + py + 1] = true;
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

CTessMeshCache::CTessMeshCache(const int numPatchesX, const int numPatchesZ, const GLenum meshTessBufferType) :
	numPatchesX(numPatchesX),
	numPatchesZ(numPatchesZ)
{
	LOG("CTessMeshCache");
	//meshTemplate = std::make_unique<MeshPatches>();
	meshTemplate = std::unique_ptr<MeshPatches>(new MeshPatches());

	FillMeshTemplateBuffer();

	glGenBuffers(1, &meshTemplateVBO);

	glBindBuffer(GL_ARRAY_BUFFER, meshTemplateVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(MeshPatches), meshTemplate.get(), GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	meshTessVBOs.resize(numPatchesX * numPatchesZ);
	tessMeshDirty.resize(numPatchesX * numPatchesZ + 1); //first one is a global retesselation bool

	RequestTesselation();

	for (auto& meshTessVBO : meshTessVBOs) {
		glGenBuffers(1, &meshTessVBO);

		LOG("glBindBuffer(%d, %d)", meshTessBufferType, meshTessVBO);
		glBindBuffer(meshTessBufferType, meshTessVBO);
		glBufferData(meshTessBufferType, TeshMessConsts::TESS_TRIANGLE_NUM_MAX * sizeof(MeshTessTriangle), nullptr, GL_DYNAMIC_COPY);
	}
	glBindBuffer(meshTessBufferType, 0); //unbind once

	LOG("CTessMeshCache 2");
}

CTessMeshCache::~CTessMeshCache() {
	glDeleteBuffers(1, &meshTemplateVBO);

	for (const auto meshTessVBO : meshTessVBOs) {
		glDeleteBuffers(1, &meshTessVBO);
	}
}