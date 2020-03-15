/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _TESS_MESH_DRAWER_H_
#define _TESS_MESH_DRAWER_H_

#include <vector>

#include "Map/BaseGroundDrawer.h"
#include "Map/SMF/IMeshDrawer.h"
#include "System/EventHandler.h"



class CSMFGroundDrawer;

class CTessMeshDrawer : public IMeshDrawer, public CEventClient
{
public:
	// CEventClient interface
	bool WantsEvent(const std::string& eventName) {
		return (eventName == "UnsyncedHeightMapUpdate") || (eventName == "DrawInMiniMap");
	}
	bool GetFullRead() const { return true; }
	int  GetReadAllyTeam() const { return AllAccessTeam; }

	void UnsyncedHeightMapUpdate(const SRectangle& rect);
	void Update() {};
	void DrawInMiniMap();

public:
	CTessMeshDrawer(CSMFGroundDrawer* gd);
	~CTessMeshDrawer();

	void DrawMesh(const DrawPass::e& drawPass);
	void DrawBorderMesh(const DrawPass::e& drawPass);

public:
	static bool Supported() { return globalRendering->supportTesselation; }

private:
	enum {
		MAP_BORDER_L     = 0,
		MAP_BORDER_R     = 1,
		MAP_BORDER_T     = 2,
		MAP_BORDER_B     = 3,
		MAP_BORDER_COUNT = 4,
	};

private:
	static constexpr int32_t PATCH_SIZE = 128; // must match SMFReadMap::bigSquareSize
	static constexpr int32_t TESS_LEVEL = 64; // should be a power of two, less or equal than GL_MAX_TESS_GEN_LEVEL (64)
	static constexpr int32_t PATCH_RC_QUAD_NUM  = PATCH_SIZE / TESS_LEVEL; // number of quads in one row/column of the patch
	static constexpr int32_t PATCH_VERT_NUM     = 4 * PATCH_RC_QUAD_NUM * PATCH_RC_QUAD_NUM;  // number of vertices in whole patch

	static constexpr float VBO_DEFAULT_HEIGHT   = 100.0f; //used for debug purposes and overwritten by shader anyway

private:
	CSMFGroundDrawer* smfGroundDrawer;

	bool instancingSupported;

	uint32_t numPatchesX;
	uint32_t numPatchesY;

	VBO squareVertexBuffer;

	//std::array<VBO, MAP_BORDER_COUNT> borderVertexBuffers;
};

#endif // _ROAM_MESH_DRAWER_H_
