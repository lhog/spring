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
	: CEventClient("[CTessMeshDrawer]", 2718965 + 1, false), //after heightmap update
	smfGroundDrawer(gd)
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
/*
	if ((CTessMeshCacheTF::Supported()))
		tessMeshCache = std::unique_ptr<CTessMeshCacheTF>(new CTessMeshCacheTF(numPatchesX, numPatchesZ));
*/
	if ((CTessMeshCacheSSBO::Supported()))
		tessMeshCache = std::unique_ptr<CTessMeshCacheSSBO>(new CTessMeshCacheSSBO(numPatchesX, numPatchesZ));

	tessMeshCache->SetRunQueries(true);
}

CTessMeshDrawer::~CTessMeshDrawer()
{
	eventHandler.RemoveClient(this);
}

void CTessMeshDrawer::DrawMesh(const DrawPass::e& drawPass)
{
	for (int z = 0; z < numPatchesZ; ++z)
	for (int x = 0; x < numPatchesX; ++x) {
		if (drawPass != DrawPass::Shadow)
			smfGroundDrawer->SetupBigSquare(x, z);
		tessMeshCache->DrawMesh(x, z);
	}
}

void CTessMeshDrawer::DrawInMiniMap() {
	//TODO: place debug
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
	/*
	LOG("CTessMeshDrawer::UnsyncedHeightMapUpdate DF=%d Rect{%d, %d, %d, %d} x={%d, %d} z={%d, %d}", globalRendering->drawFrame, rect.x1, rect.x2, rect.y1, rect.y2,\
		static_cast<int>(std::floor(rect.x1 / TeshMessConsts::UHM_TO_MESH)), static_cast<int>(std::ceil(rect.x2 / TeshMessConsts::UHM_TO_MESH)), \
		static_cast<int>(std::floor(rect.z1 / TeshMessConsts::UHM_TO_MESH)), static_cast<int>(std::ceil(rect.z2 / TeshMessConsts::UHM_TO_MESH))  \
	);
	*/

	for (int x = static_cast<int>(std::floor(rect.x1 / TeshMessConsts::UHM_TO_MESH)); x <= static_cast<int>(std::ceil(rect.x2 / TeshMessConsts::UHM_TO_MESH)); ++x)
	for (int z = static_cast<int>(std::floor(rect.z1 / TeshMessConsts::UHM_TO_MESH)); z <= static_cast<int>(std::ceil(rect.z2 / TeshMessConsts::UHM_TO_MESH)); ++z) {
		tessMeshCache->RequestTesselation(x, z);
	}
}

void CTessMeshDrawer::SunChanged() {
	tessMeshCache->RequestTesselation();
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
/*
	LOG("CTessMeshDrawer::Update lastCamDir={%f,%f,%f} camDir={%f,%f,%f} dot1=%f dot2=%f", \
		lastCamDir.x, lastCamDir.y, lastCamDir.z, \
		camDir.x, camDir.y, camDir.z, \
		lastCamDir.dot(camDir), lastCamDir.x * camDir.x + lastCamDir.y * camDir.y + lastCamDir.z * camDir.z \
	);
*/

	if (lastCamPos.distance(camPos) > CTessMeshDrawer::camDistDiff ||
		math::fabs(lastCamDir.dot(camDir)) < CTessMeshDrawer::camDirDiff) {
/*
		LOG("CTessMeshDrawer::Update lastCamPos={%f,%f,%f} camPos={%f,%f,%f} lastCamDir={%f,%f,%f} camDir={%f,%f,%f} dist=%f dot=%f", \
			lastCamPos.x, lastCamPos.y, lastCamPos.z, \
			camPos.x, camPos.y, camPos.z, \
			lastCamDir.x, lastCamDir.y, lastCamDir.z, \
			camDir.x, camDir.y, camDir.z, \
			lastCamPos.distance(camPos), math::fabs(lastCamDir.dot(camDir)) \
		);

		LOG("RequestTesselation() %f || %f || drawFrame = %d", lastCamPos.distance(camPos), math::fabs(lastCamDir.dot(camDir)), globalRendering->drawFrame);
*/
		lastCamPos = camPos;
		lastCamDir = camDir;
		tessMeshCache->RequestTesselation();
	}

	tessMeshCache->Update();
}

