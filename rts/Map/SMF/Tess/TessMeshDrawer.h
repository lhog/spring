/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _TESS_MESH_DRAWER_H_
#define _TESS_MESH_DRAWER_H_

#include <vector>

#include "Map/BaseGroundDrawer.h"
#include "Map/SMF/IMeshDrawer.h"
#include "System/EventHandler.h"

#include "Rendering/GlobalRendering.h"
#include "Rendering/GL/myGL.h"

#include "TessMeshCommon.h"


class CSMFGroundDrawer;

class CTessMeshDrawer : public IMeshDrawer, public CEventClient
{
public:
	// CEventClient interface
	bool WantsEvent(const std::string& eventName) override {
		return (eventName == "UnsyncedHeightMapUpdate") || (eventName == "Update") || (eventName == "DrawInMiniMap");
	}
	bool GetFullRead() const override { return true; }
	int  GetReadAllyTeam() const override { return AllAccessTeam; }

	void UnsyncedHeightMapUpdate(const SRectangle& rect) override;
	void SunChanged() override;
	void Update() override;
public:
	CTessMeshDrawer(CSMFGroundDrawer* gd);
	~CTessMeshDrawer();

	void DrawMesh(const DrawPass::e& drawPass) override;
	void DrawInMiniMap() override;
	void DrawBorderMesh(const DrawPass::e& drawPass) override;
public:
	static bool Supported();
private:
	enum {
		MAP_BORDER_L     = 0,
		MAP_BORDER_R     = 1,
		MAP_BORDER_T     = 2,
		MAP_BORDER_B     = 3,
		MAP_BORDER_COUNT = 4,
	};
private:
	//tune me
	static constexpr float camDistDiff = 2.0f;
	static constexpr float camDirDiff = 0.9995f; // <2 degrees difference

private:
	CSMFGroundDrawer* smfGroundDrawer;

	std::unique_ptr<CTessMeshCache> tessMeshCache;

	float3 lastCamPos = ZeroVector;
	float3 lastCamDir = FwdVector;

	uint32_t numPatchesX;
	uint32_t numPatchesZ;
};

#endif // _TESS_MESH_DRAWER_H_
