/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

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

#include "TessMeshDrawer.h"

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
{
	eventHandler.AddClient(this);

	numPatchesX = mapDims.mapx / TeshMessConsts::PATCH_SIZE;
	numPatchesZ = mapDims.mapy / TeshMessConsts::PATCH_SIZE;

	LOG("numPatchesX, numPatchesZ, mapx, mapz = %d %d %d %d", numPatchesX, numPatchesZ, mapDims.mapx, mapDims.mapy);

	assert(numPatchesX >= 1);
	assert(numPatchesZ >= 1);

/*
	if (CTessMeshCacheSSBO::Supported())
		tessMeshCache = std::unique_ptr<CTessMeshCacheSSBO>(new CTessMeshCacheSSBO(numPatchesX, numPatchesZ));
	else if ((CTessMeshCacheTF::Supported()))
		tessMeshCache = std::unique_ptr<CTessMeshCacheTF>(new CTessMeshCacheTF(numPatchesX, numPatchesZ));
*/
	if ((CTessMeshCacheTF::Supported()))
		tessMeshCache = std::unique_ptr<CTessMeshCacheTF>(new CTessMeshCacheTF(numPatchesX, numPatchesZ));
}

CTessMeshDrawer::~CTessMeshDrawer()
{
	eventHandler.RemoveClient(this);
}

void CTessMeshDrawer::DrawMesh(const DrawPass::e& drawPass)
{
	Update();
}

void CTessMeshDrawer::DrawBorderMesh(const DrawPass::e& drawPass)
{

}

bool CTessMeshDrawer::Supported() {
	return
		CTessMeshCacheTF::Supported() ||
		CTessMeshCacheSSBO::Supported();
}


void CTessMeshDrawer::UnsyncedHeightMapUpdate(const SRectangle& rect)
{
	//LOG("CTessMeshDrawer::UnsyncedHeightMapUpdate DF=%d Rect{%d, %d, %d, %d}", globalRendering->drawFrame, rect.x1, rect.x2, rect.y1, rect.y2);
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
	for (int x = std::floor(rect.x1 / TeshMessConsts::UHM_TO_MESH); x <= std::ceil(rect.x2 / TeshMessConsts::UHM_TO_MESH); ++x)
	for (int z = std::floor(rect.z1 / TeshMessConsts::UHM_TO_MESH); z <= std::ceil(rect.z2 / TeshMessConsts::UHM_TO_MESH); ++z) {
		tessMeshCache->RequestTesselation(x, z);
	}
}

void CTessMeshDrawer::Update()
{
	auto cam = CCameraHandler::GetCamera(CCamera::CAMTYPE_PLAYER);
	auto camPos = cam->GetPos();
	auto camDir = cam->GetDir();
/*
	LOG("CTessMeshDrawer::Update lastCamPos={%f,%f,%f} camPos={%f,%f,%f} lastCamDir={%f,%f,%f} camDir={%f,%f,%f} dist=%f dot=%f", \
		lastCamPos.x, lastCamPos.y, lastCamPos.z, \
		camPos.x, camPos.y, camPos.z, \
		lastCamDir.x, lastCamDir.y, lastCamDir.z, \
		camDir.x, camDir.y, camDir.z, \
		lastCamPos.distance(camPos), lastCamDir.dot(camDir) \
	);
*/
	if (lastCamPos.distance(camPos) > CTessMeshDrawer::camDistDiff ||
		abs(lastCamDir.dot(camDir)) < CTessMeshDrawer::camDirDiff) {

		lastCamPos = camPos;
		lastCamDir = camDir;
		tessMeshCache->RequestTesselation();
	}

	tessMeshCache->Update();
}

