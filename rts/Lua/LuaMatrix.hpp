/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LUA_MATRIX_H
#define LUA_MATRIX_H

#include <map>
#include <vector>
#include <string>
#include <tuple>

#include "System/Log/ILog.h"

#include "lib/sol2/sol.hpp"
#include "lib/fmt/format.h"

#include "System/Matrix44f.h"
#include "System/MathConstants.h"
#include "System/EventClient.h"
#include "System/float4.h"


#undef near
#undef far

struct lua_State;

struct CFeature;
struct CSolidObject;
struct CUnit;

struct LocalModelPiece;

typedef std::tuple< float, float, float, float, float, float, float, float, float, float, float, float, float, float, float, float > tuple16f;

class LuaMatrixEventsListeners : public CEventClient {
public:
	LuaMatrixEventsListeners()
		: CEventClient("[LuaMatrixEventsListeners]", 42, false)
		, viewResized(true) {};

	bool GetViewResized() const {
		return viewResized;
	}
public:
	bool WantsEvent(const std::string& eventName) override {
		return (eventName == "ViewResize");
	}
	void ViewResize() override {
		viewResized = true;
	}
private:
	bool viewResized;
};

class LuaMatrixImpl {
public:
	LuaMatrixImpl() = default; //matrices should default to identity
	LuaMatrixImpl(
		const float m0, const float m1, const float m2, const float m3,
		const float m4, const float m5, const float m6, const float m7,
		const float m8, const float m9, const float m10, const float m11,
		const float m12, const float m13, const float m14, const float m15);


	LuaMatrixImpl(CMatrix44f mm) { mat = mm; }

	LuaMatrixImpl(const LuaMatrixImpl& lva) = default;
	LuaMatrixImpl(LuaMatrixImpl&& lva) = default; //move cons

	~LuaMatrixImpl() = default;
public:
	void Zero();
	void Identity() { mat = CMatrix44f::Identity(); };

	void MultMat4(const LuaMatrixImpl& mat2) { mat *= mat2.GetMatRef(); }
	void MultVec4(const float x, const float y, const float z, const float w) { mat.Mul(float4{ x, y, z, w }); }
	void MultVec3(const float x, const float y, const float z) { MultVec4( x, y, z, 1.0f); }

	void InverseAffine() { mat.InvertAffineInPlace(); };
	void Inverse() { mat.InvertInPlace(); };

	void Transpose() { mat.Transpose(); };

	void Translate(const float x, const float y, const float z);
	void Scale(const float x, const float y, const float z);

	void RotateRad(const float rad, const float x, const float y, const float z);
	void RotateDeg(const float deg, const float x, const float y, const float z) { RotateRad(deg * math::DEG_TO_RAD, x, y, z); };

	void RotateRadX(const float rad) { RotateRad(rad, 1.0f, 0.0f, 0.0f); };
	void RotateRadY(const float rad) { RotateRad(rad, 0.0f, 1.0f, 0.0f); };
	void RotateRadZ(const float rad) { RotateRad(rad, 0.0f, 0.0f, 1.0f); };

	void RotateDegX(const float deg) { RotateRad(deg * math::DEG_TO_RAD, 1.0f, 0.0f, 0.0f); };
	void RotateDegY(const float deg) { RotateRad(deg * math::DEG_TO_RAD, 0.0f, 1.0f, 0.0f); };
	void RotateDegZ(const float deg) { RotateRad(deg * math::DEG_TO_RAD, 0.0f, 0.0f, 1.0f); };

	bool LoadUnit(const unsigned int unitID, sol::this_state L);
	bool MultUnit(const unsigned int unitID, sol::this_state L);

	bool LoadUnitPiece(const unsigned int unitID, const unsigned int pieceNum, sol::this_state L);
	bool MultUnitPiece(const unsigned int unitID, const unsigned int pieceNum, sol::this_state L);

	bool LoadFeature(const unsigned int featureID, sol::this_state L);
	bool MultFeature(const unsigned int featureID, sol::this_state L);

	bool LoadFeaturePiece(const unsigned int featureID, const unsigned int pieceNum, sol::this_state L);
	bool MultFeaturePiece(const unsigned int featureID, const unsigned int pieceNum, sol::this_state L);

	void ScreenViewMatrix();
	void ScreenProjMatrix();

	void LoadOrtho(const float left, const float right, const float bottom, const float top, const float near, const float far);
	void MultOrtho(const float left, const float right, const float bottom, const float top, const float near, const float far);

	void LoadFrustum(const float left, const float right, const float bottom, const float top, const float near, const float far);
	void MultFrustum(const float left, const float right, const float bottom, const float top, const float near, const float far);

	void LoadBillboard(const std::optional<unsigned int> cameraIdOpt);
	void MultBillboard(const std::optional<unsigned int> cameraIdOpt);

	tuple16f GetAsScalar()
	{
		return std::make_tuple(
			mat[0], mat[1], mat[2], mat[3],
			mat[4], mat[5], mat[6], mat[7],
			mat[8], mat[9], mat[10], mat[11],
			mat[12], mat[13], mat[14], mat[15]
		);
	}

	sol::as_table_t<std::vector<float>> GetAsTable()
	{
		return sol::as_table(std::vector<float> {
			mat[0], mat[1], mat[2], mat[3],
			mat[4], mat[5], mat[6], mat[7],
			mat[8], mat[9], mat[10], mat[11],
			mat[12], mat[13], mat[14], mat[15]
		});
	}

public:
	const CMatrix44f& GetMatRef() const { return mat; }
public:
	LuaMatrixImpl operator * (const LuaMatrixImpl& lmi) const { return LuaMatrixImpl(mat * lmi.GetMatRef()); };
	LuaMatrixImpl operator + (const LuaMatrixImpl& lmi) const { return LuaMatrixImpl(mat + lmi.GetMatRef()); };
private:
	template<typename TObj>
	inline const TObj* GetObjectImpl(const unsigned int objID, sol::this_state L);

	template<typename TObj>
	inline bool GetObjectMatImpl(const unsigned int objID, CMatrix44f& outMat, sol::this_state L);
	template <typename  TObj>
	bool LoadObjectMatImpl(const unsigned int objID, sol::this_state L);
	template <typename  TObj>
	bool MultObjectMatImpl(const unsigned int objID, sol::this_state L);

	template <typename  TObj>
	bool GetObjectPieceMatImpl(const unsigned int objID, const unsigned int pieceNum, CMatrix44f& outMat, sol::this_state L);
	template <typename  TObj>
	bool LoadObjectPieceMatImpl(const unsigned int objID, const unsigned int pieceNum, sol::this_state L);
	template <typename  TObj>
	bool MultObjectPieceMatImpl(const unsigned int objID, const unsigned int pieceNum, sol::this_state L);

	void CondSetupScreenMatrices();
private:
	inline static bool IsFeatureVisible(const CFeature* feature, sol::this_state L);
	inline static bool IsUnitVisible(const CUnit* feature, sol::this_state L);
	inline static CUnit* ParseUnit(int unitID, sol::this_state L);
	inline static CFeature* ParseFeature(int featureID, sol::this_state L);
	inline static const LocalModelPiece* ParseObjectConstLocalModelPiece(const CSolidObject* obj, const unsigned int pieceNum);
	//static methods
private:
	//static constants
private:
	static LuaMatrixEventsListeners lmel;
private:
	CMatrix44f mat;

	CMatrix44f screenViewMatrix;
	CMatrix44f screenProjMatrix;
	//private vars
};

class LuaMatrix {
public:
	static int GetMatrix(lua_State* L);
	static bool PostPushEntries(lua_State* L);
	static bool PushEntries(lua_State* L);
};


#endif //LUA_VAO_H