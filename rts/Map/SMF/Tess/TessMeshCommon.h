/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _TESS_MESH_COMMON_H_
#define _TESS_MESH_COMMON_H_

#include <memory>
#include <vector>

#include "Map/ReadMap.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/GL/myGL.h"
#include "Map/SMF/SMFGroundDrawer.h"
#include "System/float3.h"

#include "TessMeshShaders.h"

struct TeshMessConsts {
	static constexpr int32_t PATCH_SIZE = 128; // must match SMFReadMap::bigSquareSize
	static constexpr int32_t TESS_LEVEL = 32; // should be a power of two, less or equal than GL_MAX_TESS_GEN_LEVEL (64)
	static constexpr int32_t PATCH_RC_QUAD_NUM = PATCH_SIZE / TESS_LEVEL; // number of quads in one row/column of the patch
	static constexpr int32_t PATCH_VERT_NUM = 4 * PATCH_RC_QUAD_NUM * PATCH_RC_QUAD_NUM;  // number of vertices in whole patch
	static constexpr int32_t TESS_TRIANGLE_NUM_MAX = (PATCH_RC_QUAD_NUM * PATCH_RC_QUAD_NUM) * (TESS_LEVEL * TESS_LEVEL) * 2;

	static constexpr float   UHM_TO_MESH = static_cast<float>(PATCH_SIZE); // Divider to divide UHM rect coord by to get Patch{x,y}
};

struct DrawArraysIndirectCommand {
	uint32_t  count;
	uint32_t  primCount;
	uint32_t  first;
	uint32_t  baseInstance;
};

struct MeshPatches {
	float3 vertices[TeshMessConsts::PATCH_VERT_NUM];
};

struct MeshTessTriangle {
	float3 vertices[3];
};

class CTessMeshCache
{
public:
	CTessMeshCache(const int numPatchesX, const int numPatchesZ, const GLenum meshTessBufferType);
	virtual ~CTessMeshCache();
public:
	static bool Supported() {
		return
			globalRendering->haveGLSL &&
			readMap->GetGroundDrawer()->UseAdvShading() &&

			GLEW_ARB_geometry_shader4 &&
			GLEW_ARB_vertex_buffer_object &&

			GLEW_ARB_texture_float && //needed for Heightmap
			GLEW_ARB_texture_non_power_of_two &&  //needed for Heightmap

			GLEW_ARB_tessellation_shader;
	};
public:
	virtual void Update() = 0;
	virtual void RequestTesselation();
	virtual void RequestTesselation(const int px, const int pz);
	virtual void SetRunQueries(bool b) { runQueries = b; }
	virtual void Reset() = 0;
	virtual void DrawMesh(const int px, const int pz) = 0;
private:
	void FillMeshTemplateBuffer();
protected:
	int numPatchesX;
	int numPatchesZ;

	bool runQueries;

	GLuint tessMeshQuery;

	std::vector<bool> tessMeshDirty;

	std::unique_ptr<CTessMeshShader> tessMeshShader;

	std::unique_ptr<MeshPatches> meshTemplate;

	std::vector<GLuint> meshTessVBOs;

	GLuint meshTemplateVBO;
};

class CTessMeshCacheTF : public CTessMeshCache
{
public:
	//, const GLenum meshTessBufferType = GL_TRANSFORM_FEEDBACK_BUFFER
	CTessMeshCacheTF(const int numPatchesX, const int numPatchesZ);
	virtual ~CTessMeshCacheTF();
public:
	static bool Supported() {
		return CTessMeshCache::Supported() &&
			GLEW_ARB_transform_feedback2;
	};
public:
	// Inherited via CTessMeshCache
	virtual void Update() override;
	virtual void Reset() override;
	virtual void DrawMesh(const int px, const int pz) override;
private:
	std::vector<GLuint> meshTessTFOs;
};

class CTessMeshCacheSSBO : public CTessMeshCache
{
public:
	static bool Supported() {
		return CTessMeshCache::Supported() &&
			GLEW_ARB_framebuffer_object &&
			GLEW_EXT_framebuffer_blit &&
			GLEW_ARB_texture_rg &&
			GLEW_ARB_shader_storage_buffer_object &&
			GLEW_ARB_compute_shader &&
			GLEW_ARB_shader_image_load_store; // for glMemoryBarrier()
	};
public:
	//, const GLenum meshTessBufferType = GL_SHADER_STORAGE_BUFFER
	CTessMeshCacheSSBO(const int numPatchesX, const int numPatchesZ);
	virtual ~CTessMeshCacheSSBO();
public:
	// Inherited via CTessMeshCache
	virtual void Update() override;
	virtual void Reset() override;
	virtual void DrawMesh(const int px, const int pz) override;
private:
	DrawArraysIndirectCommand daicZero;
	std::vector<GLuint> meshTessDAIBs;

	int numMips;

	GLuint logTex; //Laplacian of Gaussian, high-Z texture
	std::vector<GLuint> logImages; //Laplacian of Gaussian Images;

	bool drawIndirect;
};


#endif // _TESS_MESH_COMMON_H_