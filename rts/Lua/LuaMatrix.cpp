#include "LuaMatrix.hpp"

#include <algorithm>

#include "LuaUtils.h"

#include "System/type2.h"

#include "Sim/Features/Feature.h"
#include "Sim/Units/Unit.h"

#include "Sim/Objects/SolidObject.h"
#include "Sim/Features/FeatureHandler.h"
#include "Sim/Units/UnitHandler.h"

#include "Rendering/Models/3DModel.h"
#include "Rendering/GlobalRendering.h"

#include "Game/Camera.h"
#include "Game/CameraHandler.h"

#undef near
#undef far

LuaMatrixEventsListeners LuaMatrixImpl::lmel = LuaMatrixEventsListeners{};

LuaMatrixImpl::LuaMatrixImpl(const float m0, const float m1, const float m2, const float m3, const float m4, const float m5, const float m6, const float m7, const float m8, const float m9, const float m10, const float m11, const float m12, const float m13, const float m14, const float m15)
{
#define MAT_EQ_M(i) mat[i] = m##i
	MAT_EQ_M(0); MAT_EQ_M(1); MAT_EQ_M(2); MAT_EQ_M(3);
	MAT_EQ_M(4); MAT_EQ_M(5); MAT_EQ_M(6); MAT_EQ_M(7);
	MAT_EQ_M(8); MAT_EQ_M(9); MAT_EQ_M(10); MAT_EQ_M(11);
	MAT_EQ_M(12); MAT_EQ_M(13); MAT_EQ_M(14); MAT_EQ_M(15);
#undef MAT_EQ_M
}

void LuaMatrixImpl::Zero()
{
	//std::fill(&mat[0], &mat[0] + sizeof(CMatrix44f) / sizeof(float), 0.0f);
#define MAT_EQ_0(i) mat[i] = 0.0f
	MAT_EQ_0(0); MAT_EQ_0(1); MAT_EQ_0(2); MAT_EQ_0(3);
	MAT_EQ_0(4); MAT_EQ_0(5); MAT_EQ_0(6); MAT_EQ_0(7);
	MAT_EQ_0(8); MAT_EQ_0(9); MAT_EQ_0(10); MAT_EQ_0(11);
	MAT_EQ_0(12); MAT_EQ_0(13); MAT_EQ_0(14); MAT_EQ_0(15);
#undef MAT_EQ_0
}

void LuaMatrixImpl::Translate(const float x, const float y, const float z)
{
	mat.Translate({ x, y, z });
}

void LuaMatrixImpl::Scale(const float x, const float y, const float z)
{
	mat.Scale({ x, y, z });
}

void LuaMatrixImpl::RotateRad(const float rad, const float x, const float y, const float z)
{
	mat.Rotate(-rad , { x, y, z });
}

///////////////////////////////////////////////////////////

inline bool LuaMatrixImpl::IsUnitVisible(const CUnit* unit, sol::this_state L)
{
	const int readAllyTeam = CLuaHandle::GetHandleReadAllyTeam(L);

	if ((readAllyTeam < 0) && (readAllyTeam == CEventClient::NoAccessTeam))
		return false;

	if ((unit->losStatus[readAllyTeam] & LOS_INLOS) == 0)
		return false;

	return true;
}

inline bool LuaMatrixImpl::IsFeatureVisible(const CFeature* feature, sol::this_state L)
{
	if (CLuaHandle::GetHandleFullRead(L))
		return true;

	const int readAllyTeam = CLuaHandle::GetHandleReadAllyTeam(L);

	if (readAllyTeam < 0)
		return (readAllyTeam == CEventClient::AllAccessTeam);

	return (feature->IsInLosForAllyTeam(readAllyTeam));
}

inline CUnit* LuaMatrixImpl::ParseUnit(int unitID, sol::this_state L)
{
	CUnit* unit = unitHandler.GetUnit(unitID);

	if (unit == nullptr)
		return nullptr;

	if (!IsUnitVisible(unit, L))
		return nullptr;

	return unit;
}

inline CFeature* LuaMatrixImpl::ParseFeature(int featureID, sol::this_state L)
{
	CFeature* feature = featureHandler.GetFeature(featureID);

	if (feature == nullptr)
		return nullptr;

	if (!IsFeatureVisible(feature, L))
		return nullptr;

	return feature;
}

inline const LocalModelPiece* LuaMatrixImpl::ParseObjectConstLocalModelPiece(const CSolidObject* obj, const unsigned int pieceNum)
{
	if (!obj->localModel.HasPiece(pieceNum))
		return nullptr;

	return (obj->localModel.GetPiece(pieceNum));
}

///////////////////////////////////////////////////////////

template<typename TObj>
inline const TObj* LuaMatrixImpl::GetObjectImpl(const unsigned int objID, sol::this_state L)
{
	const TObj* obj = nullptr;
	if constexpr (std::is_same<TObj, CUnit>::value) {
		obj = ParseUnit(objID, L);
	}
	else if constexpr (std::is_same<TObj, CUnit>::value) {
		obj = ParseFeature(objID, L);
	}

	return obj;
}

template<typename TObj>
inline bool LuaMatrixImpl::GetObjectMatImpl(const unsigned int objID, CMatrix44f& outMat, sol::this_state L)
{
	const TObj* obj = GetObjectImpl<TObj>(objID, L);

	if (!obj)
		return false;

	outMat = obj->GetTransformMatrix();

	return true;
}

template<typename TObj>
inline bool LuaMatrixImpl::LoadObjectMatImpl(const unsigned int objID, sol::this_state L)
{
	return GetObjectMatImpl<TObj>(objID, mat, L);
}

template<typename TObj>
inline bool LuaMatrixImpl::MultObjectMatImpl(const unsigned int objID, sol::this_state L)
{
	CMatrix44f mat2;
	if (!GetObjectMatImpl<TObj>(objID, mat2, L))
		return false;

	mat *= mat2;
	return true;
}

template<typename TObj>
inline bool LuaMatrixImpl::GetObjectPieceMatImpl(const unsigned int objID, const unsigned int pieceNum, CMatrix44f& outMat, sol::this_state L)
{
	const TObj* obj = GetObjectImpl<TObj>(objID, L);

	if (!obj)
		return false;

	const LocalModelPiece* lmp = ParseObjectConstLocalModelPiece(obj, pieceNum);

	if (!lmp)
		return false;

	outMat = lmp->GetModelSpaceMatrix();
	return true;
}

template<typename TObj>
inline bool LuaMatrixImpl::LoadObjectPieceMatImpl(const unsigned int objID, const unsigned int pieceNum, sol::this_state L)
{
	return GetObjectPieceMatImpl<TObj>(objID, pieceNum, mat, L);
}

template<typename TObj>
inline bool LuaMatrixImpl::MultObjectPieceMatImpl(const unsigned int objID, const unsigned int pieceNum, sol::this_state L)
{
	CMatrix44f mat2;
	if (!GetObjectPieceMatImpl<TObj>(objID, pieceNum, mat2, L))
		return false;

	mat *= mat2;
	return true;
}

inline void LuaMatrixImpl::CondSetupScreenMatrices() {

	if (!lmel.GetViewResized()) //already up-to-date
		return;

	// .x := screen width (meters), .y := eye-to-screen (meters)
	static float2 screenParameters = { 0.36f, 0.60f };

	const int remScreenSize = globalRendering->screenSizeY - globalRendering->winSizeY; // remaining desktop size (ssy >= wsy)
	const int bottomWinCoor = remScreenSize - globalRendering->winPosY; // *bottom*-left origin

	const float vpx = globalRendering->viewPosX + globalRendering->winPosX;
	const float vpy = globalRendering->viewPosY + bottomWinCoor;
	const float vsx = globalRendering->viewSizeX; // same as winSizeX except in dual-screen mode
	const float vsy = globalRendering->viewSizeY; // same as winSizeY
	const float ssx = globalRendering->screenSizeX;
	const float ssy = globalRendering->screenSizeY;
	const float hssx = 0.5f * ssx;
	const float hssy = 0.5f * ssy;

	const float zplane = screenParameters.y * (ssx / screenParameters.x);
	const float znear = zplane * 0.5f;
	const float zfar = zplane * 2.0f;
	const float zfact = znear / zplane;

	const float left = (vpx - hssx) * zfact;
	const float bottom = (vpy - hssy) * zfact;
	const float right = ((vpx + vsx) - hssx) * zfact;
	const float top = ((vpy + vsy) - hssy) * zfact;

	// translate s.t. (0,0,0) is on the zplane, on the window's bottom-left corner
	screenViewMatrix = CMatrix44f{ float3{left / zfact, bottom / zfact, -zplane} };
	screenProjMatrix = CMatrix44f::ClipControl(globalRendering->supportClipSpaceControl) * CMatrix44f::PerspProj(left, right, bottom, top, znear, zfar);
}

bool LuaMatrixImpl::LoadUnit(const unsigned int unitID, sol::this_state L)
{
	return LoadObjectMatImpl<CUnit>(unitID, L);
}

bool LuaMatrixImpl::MultUnit(const unsigned int unitID, sol::this_state L)
{
	return MultObjectMatImpl<CUnit>(unitID, L);
}

bool LuaMatrixImpl::LoadUnitPiece(const unsigned int unitID, const unsigned int pieceNum, sol::this_state L)
{
	return LoadObjectPieceMatImpl<CUnit>(unitID, pieceNum, L);
}

bool LuaMatrixImpl::MultUnitPiece(const unsigned int unitID, const unsigned int pieceNum, sol::this_state L)
{
	return MultObjectPieceMatImpl<CUnit>(unitID, pieceNum, L);
}

bool LuaMatrixImpl::LoadFeature(const unsigned int featureID, sol::this_state L)
{
	return LoadObjectMatImpl<CFeature>(featureID, L);
}

bool LuaMatrixImpl::MultFeature(const unsigned int featureID, sol::this_state L)
{
	return MultObjectMatImpl<CFeature>(featureID, L);
}

bool LuaMatrixImpl::LoadFeaturePiece(const unsigned int featureID, const unsigned int pieceNum, sol::this_state L)
{
	return LoadObjectPieceMatImpl<CFeature>(featureID, pieceNum, L);
}

bool LuaMatrixImpl::MultFeaturePiece(const unsigned int featureID, const unsigned int pieceNum, sol::this_state L)
{
	return MultObjectPieceMatImpl<CFeature>(featureID, pieceNum, L);
}

void LuaMatrixImpl::ScreenViewMatrix()
{
	CondSetupScreenMatrices();
	mat = screenViewMatrix;
}

void LuaMatrixImpl::ScreenProjMatrix()
{
	CondSetupScreenMatrices();
	mat = screenProjMatrix;
}

void LuaMatrixImpl::LoadOrtho(const float left, const float right, const float bottom, const float top, const float near, const float far)
{
	mat = CMatrix44f::ClipOrthoProj(left, right, bottom, top, near, far, globalRendering->supportClipSpaceControl * 1.0f);
}

void LuaMatrixImpl::MultOrtho(const float left, const float right, const float bottom, const float top, const float near, const float far)
{
	mat *= CMatrix44f::ClipOrthoProj(left, right, bottom, top, near, far, globalRendering->supportClipSpaceControl * 1.0f);
}

void LuaMatrixImpl::LoadFrustum(const float left, const float right, const float bottom, const float top, const float near, const float far)
{
	mat = CMatrix44f::ClipPerspProj(left, right, bottom, top, near, far, globalRendering->supportClipSpaceControl * 1.0f);
}

void LuaMatrixImpl::MultFrustum(const float left, const float right, const float bottom, const float top, const float near, const float far)
{
	mat *= CMatrix44f::ClipPerspProj(left, right, bottom, top, near, far, globalRendering->supportClipSpaceControl * 1.0f);
}

void LuaMatrixImpl::LoadBillboard(const std::optional<unsigned int> cameraIdOpt)
{
	constexpr unsigned int minCamType = CCamera::CAMTYPE_PLAYER;
	constexpr unsigned int maxCamType = CCamera::CAMTYPE_ACTIVE;

	const unsigned int camType = std::clamp(cameraIdOpt.value_or(maxCamType), minCamType, maxCamType);
	const auto cam = CCameraHandler::GetCamera(camType);
	mat = cam->GetBillBoardMatrix();
}

void LuaMatrixImpl::MultBillboard(const std::optional<unsigned int> cameraIdOpt)
{
	constexpr unsigned int minCamType = CCamera::CAMTYPE_PLAYER;
	constexpr unsigned int maxCamType = CCamera::CAMTYPE_ACTIVE;

	const unsigned int camType = std::clamp(cameraIdOpt.value_or(maxCamType), minCamType, maxCamType);
	const auto cam = CCameraHandler::GetCamera(camType);
	mat *= cam->GetBillBoardMatrix();
}

///////////////////////////////////////////////////////////

bool LuaMatrix::PostPushEntries(lua_State* L)
{
	sol::state_view lua(L);
	auto gl = lua.get<sol::table>("gl");

	gl.new_usertype<LuaMatrixImpl>("LuaMatrixImpl",
		sol::constructors<LuaMatrixImpl()>(),
		"Zero", &LuaMatrixImpl::Zero,
		"LoadZero", &LuaMatrixImpl::Zero,

		"Identity", &LuaMatrixImpl::Identity,
		"LoadIdentity", &LuaMatrixImpl::Identity,

		"MultMat4", &LuaMatrixImpl::MultMat4,
		"MultVec4", &LuaMatrixImpl::MultVec4,
		"MultVec3", &LuaMatrixImpl::MultVec3,

		"Mult", sol::overload(&LuaMatrixImpl::MultMat4, &LuaMatrixImpl::MultVec4, LuaMatrixImpl::MultVec3),

		"InverseAffine", & LuaMatrixImpl::InverseAffine,
		"InvertAffine", & LuaMatrixImpl::InverseAffine,
		"Inverse", & LuaMatrixImpl::Inverse,
		"Invert", & LuaMatrixImpl::Inverse,

		"Transpose", & LuaMatrixImpl::Transpose,

		"Translate", &LuaMatrixImpl::Translate,
		"Scale", &LuaMatrixImpl::Scale,

		"RotateRad", &LuaMatrixImpl::RotateRad,
		"RotateDeg", &LuaMatrixImpl::RotateDeg,

		"RotateRadX", &LuaMatrixImpl::RotateRadX,
		"RotateRadY", &LuaMatrixImpl::RotateRadY,
		"RotateRadZ", &LuaMatrixImpl::RotateRadZ,

		"RotateDegX", &LuaMatrixImpl::RotateDegX,
		"RotateDegY", &LuaMatrixImpl::RotateDegY,
		"RotateDegZ", &LuaMatrixImpl::RotateDegZ,

		"LoadUnit", &LuaMatrixImpl::LoadUnit,
		"MultUnit", &LuaMatrixImpl::MultUnit,

		"LoadUnitPiece", &LuaMatrixImpl::LoadUnitPiece,
		"MultUnitPiece", &LuaMatrixImpl::MultUnitPiece,

		"LoadFeature", &LuaMatrixImpl::LoadFeature,
		"MultFeature", &LuaMatrixImpl::MultFeature,

		"LoadFeaturePiece", &LuaMatrixImpl::LoadFeaturePiece,
		"MultFeaturePiece", &LuaMatrixImpl::MultFeaturePiece,

		"ScreenViewMatrix", & LuaMatrixImpl::ScreenViewMatrix,
		"ScreenProjMatrix", & LuaMatrixImpl::ScreenProjMatrix,

		"LoadOrtho", & LuaMatrixImpl::LoadOrtho,
		"MultOrtho", & LuaMatrixImpl::MultOrtho,

		"LoadFrustum", & LuaMatrixImpl::LoadFrustum,
		"MultFrustum", & LuaMatrixImpl::MultFrustum,

		"LoadBillboard", & LuaMatrixImpl::LoadBillboard,
		"MultBillboard", & LuaMatrixImpl::MultBillboard,

		"GetAsScalar", & LuaMatrixImpl::GetAsScalar,
		"GetAsTable", & LuaMatrixImpl::GetAsTable
	);

	gl.set("Matrix", sol::lua_nil); //because :)

	return true;
}

bool LuaMatrix::PushEntries(lua_State* L)
{
	REGISTER_LUA_CFUNC(GetMatrix);

	return true;
}

int LuaMatrix::GetMatrix(lua_State* L)
{
	return sol::stack::call_lua(L, 1, [=]() {
		return std::move(LuaMatrixImpl{});
	});
}