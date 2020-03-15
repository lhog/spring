/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "TessMeshDrawer.h"
#include "Game/Camera.h"
#include "Game/CameraHandler.h"
#include "Map/ReadMap.h"
#include "Map/SMF/SMFReadMap.h"
#include "Map/SMF/SMFGroundDrawer.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/ShadowHandler.h"
#include "Sim/Misc/GlobalConstants.h"
#include "System/Rectangle.h"
#include "System/Threading/ThreadPool.h"
#include "System/TimeProfiler.h"
#include "System/Config/ConfigHandler.h"
#include "System/Log/ILog.h"

#include <cmath>

#define DEBUG_QUADS_RENDERING

#ifdef DEBUG_QUADS_RENDERING
	#define GL_RENDERING_PRIMITIVE GL_QUADS
#else
	#define GL_RENDERING_PRIMITIVE GL_PATCHES
#endif


#define LOG_SECTION_TESS "TessMeshDrawer"
LOG_REGISTER_SECTION_GLOBAL(LOG_SECTION_TESS)

// use the specific section for all LOG*() calls in this source file
#ifdef LOG_SECTION_CURRENT
	#undef LOG_SECTION_CURRENT
#endif
#define LOG_SECTION_CURRENT LOG_SECTION_TESS

CTessMeshDrawer::CTessMeshDrawer(CSMFGroundDrawer* gd)
	: CEventClient("[CTessMeshDrawer]", 271989+1, false)
	, smfGroundDrawer(gd)
{
	eventHandler.AddClient(this);

	//can't do instancing anyway because diffuse texture is broken down into chunks
	//instancingSupported = glewIsSupported("GL_ARB_draw_instanced");

	numPatchesX = mapDims.mapx / PATCH_SIZE;
	numPatchesY = mapDims.mapy / PATCH_SIZE;

	assert(numPatchesX >= 1);
	assert(numPatchesY >= 1);

	squareVertexBuffer.Bind(GL_ARRAY_BUFFER);
	squareVertexBuffer.New(PATCH_VERT_NUM * sizeof(float3), GL_DYNAMIC_DRAW); //vec3 for vertex, no normals(?)

	auto patchVert = reinterpret_cast<float3*>(squareVertexBuffer.MapBuffer(GL_MAP_WRITE_BIT)); //don't bother with persistent buffer

	if (patchVert) {
		uint32_t patchVertIdx = 0;
		for (int i = 0; i < PATCH_RC_QUAD_NUM; ++i)
			for (int j = 0; j < PATCH_RC_QUAD_NUM; ++j) {
				// CCW
				patchVert[patchVertIdx++] = float3((i + 0) * SQUARE_SIZE * TESS_LEVEL, VBO_DEFAULT_HEIGHT, (j + 0) * SQUARE_SIZE * TESS_LEVEL); //TL
				patchVert[patchVertIdx++] = float3((i + 0) * SQUARE_SIZE * TESS_LEVEL, VBO_DEFAULT_HEIGHT, (j + 1) * SQUARE_SIZE * TESS_LEVEL); //BL
				patchVert[patchVertIdx++] = float3((i + 1) * SQUARE_SIZE * TESS_LEVEL, VBO_DEFAULT_HEIGHT, (j + 1) * SQUARE_SIZE * TESS_LEVEL); //BR
				patchVert[patchVertIdx++] = float3((i + 1) * SQUARE_SIZE * TESS_LEVEL, VBO_DEFAULT_HEIGHT, (j + 0) * SQUARE_SIZE * TESS_LEVEL); //TR
			}
	}

	assert(patchVertIdx == patchVertIdx);

	squareVertexBuffer.UnmapBuffer();
	squareVertexBuffer.Unbind();

	//UnsyncedHeightMapUpdate(SRectangle{ 0, 0, mapDims.mapx, mapDims.mapy });
}

CTessMeshDrawer::~CTessMeshDrawer()
{
	eventHandler.RemoveClient(this);
}


void CTessMeshDrawer::DrawMesh(const DrawPass::e& drawPass)
{
	for (uint32_t py = 0; py < numPatchesY; ++py) {
		for (uint32_t px = 0; px < numPatchesX; ++px) {

			if (drawPass != DrawPass::Shadow)
				smfGroundDrawer->SetupBigSquare(px, py);

			glEnableClientState(GL_VERTEX_ARRAY);
			squareVertexBuffer.Bind(GL_ARRAY_BUFFER);
			assert(squareVertexBuffer.GetPtr() == nullptr);
			glVertexPointer(3, GL_FLOAT, sizeof(float3), squareVertexBuffer.GetPtr());
			squareVertexBuffer.Unbind();

			glDrawArrays(GL_RENDERING_PRIMITIVE, 0, PATCH_VERT_NUM);

			glDisableClientState(GL_VERTEX_ARRAY);
		}
	}
}

void CTessMeshDrawer::DrawBorderMesh(const DrawPass::e& drawPass)
{

}

void CTessMeshDrawer::DrawInMiniMap()
{

}



void CTessMeshDrawer::UnsyncedHeightMapUpdate(const SRectangle& rect)
{
	/**
	constexpr int BORDER_MARGIN = 2;
	constexpr float INV_PATCH_SIZE = 1.0f / PATCH_SIZE;

	// add margin since Patches share borders
	const int xstart = std::max(          0, (int)math::floor((rect.x1 - BORDER_MARGIN) * INV_PATCH_SIZE));
	const int xend   = std::min(numPatchesX, (int)math::ceil ((rect.x2 + BORDER_MARGIN) * INV_PATCH_SIZE));
	const int zstart = std::max(          0, (int)math::floor((rect.z1 - BORDER_MARGIN) * INV_PATCH_SIZE));
	const int zend   = std::min(numPatchesY, (int)math::ceil ((rect.z2 + BORDER_MARGIN) * INV_PATCH_SIZE));

	// update patches in both tessellations
	for (unsigned int i = MESH_NORMAL; i <= MESH_SHADOW; i++) {
		auto& patches = patchMeshGrid[i];

		for (int z = zstart; z < zend; ++z) {
			for (int x = xstart; x < xend; ++x) {
				Patch& p = patches[z * numPatchesX + x];

				// clamp the update-rectangle within the patch
				SRectangle prect(
					std::max(rect.x1 - BORDER_MARGIN - p.coors.x,          0),
					std::max(rect.z1 - BORDER_MARGIN - p.coors.y,          0),
					std::min(rect.x2 + BORDER_MARGIN - p.coors.x, PATCH_SIZE),
					std::min(rect.z2 + BORDER_MARGIN - p.coors.y, PATCH_SIZE)
				);

				p.UpdateHeightMap(prect);
			}
		}
	}
	**/
}

