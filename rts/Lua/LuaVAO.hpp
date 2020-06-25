/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LUA_VAO_H
#define LUA_VAO_H

#include <map>
#include <string>

#include "System/Log/ILog.h"

#include "Rendering/GL/myGL.h"
#include "lib/sol2/sol.hpp"
#include "lib/fmt/format.h"

struct lua_State;
struct VAO;
struct VBO;

struct VAOAttrib {
	int divisor;
	GLint size;
	GLenum type;
	GLboolean normalized;
	GLsizei stride;
	GLsizei pointer;
	//AUX
	int typeSizeInBytes;
	std::string name;
};

class LuaVAOImpl {
public:
	static constexpr int _GL_MAX_VERTEX_ATTRIBS = 16;
public:
	LuaVAOImpl() = delete;
	LuaVAOImpl(const sol::optional<bool> freqUpdatedOpt);

	LuaVAOImpl(const LuaVAOImpl& lva) = delete; //no copy cons
	LuaVAOImpl(LuaVAOImpl&& lva) = default; //move cons

	~LuaVAOImpl();
public:
	template <typename  TIn, typename  TOut>
	static TOut TransformFunc(const TIn input);
public:
	int SetVertexAttributes(const int maxAttrCount, const sol::table& attrDefTable);
	int SetInstanceAttributes(const int maxAttrCount, const sol::table& attrDefTable);
	bool SetIndexAttributes(const int maxIndxCount, const sol::optional<GLenum> indTypeOpt);

	int UploadVertexBulk(const sol::table& bulkData, const sol::optional<int> vertexOffsetOpt);
	int UploadInstanceBulk(const sol::table& bulkData, const sol::optional<int> instanceOffsetOpt);

	int UploadVertexAttribute(const int attrIndex, const sol::table& attrData, const sol::optional<int> vertexOffsetOpt);
	int UploadInstanceAttribute(const int attrIndex, const sol::table& attrData, const sol::optional<int> instanceOffsetOpt);

	int UploadIndices(const sol::table& indData, const sol::optional<int> indOffsetOpt);

	bool DrawArrays(const GLenum mode, const sol::optional<GLsizei> vertCountOpt, const sol::optional<GLint> firstOpt, const sol::optional<int> instanceCountOpt);
	bool DrawElements(const GLenum mode, const sol::optional<GLsizei> indCountOpt, const sol::optional<int> indElemOffsetOpt, const sol::optional<int> instanceCountOpt);
private:
	bool CheckPrimType(GLenum mode);
	bool CondInitVAO();
	bool SetIndexAttributesImpl(const int maxIndxCount, const GLenum indType);
	int UploadImpl(const sol::table& luaTblData, const sol::optional<int> offsetOpt, const int divisor, const int* attrNum, const int aSizeInBytes, VBO* vbo);
	void FillAttribTables(const sol::table& attrDefTable, const int divisor, int* attribsSizeInBytes);
private:
	const static GLenum defaultVertexType = GL_FLOAT;
	const static GLenum defaultIndexType = GL_UNSIGNED_SHORT;
private:
	int numAttributes;

	int maxVertCount;
	int maxInstCount;
	int maxIndxCount;

	int vertAttribsSizeInBytes;
	int instAttribsSizeInBytes;
	int indElemSizeInBytes;

	GLenum indexType;
	bool freqUpdated;

	VAO* vao;
	VBO* vboVert;
	VBO* vboInst;
	VBO* ebo;

	std::map<int, VAOAttrib> vaoAttribs;
};

class LuaVAO {
public:
	static int GetVAO(lua_State* L);
	static bool PostPushEntries(lua_State* L);
	static bool PushEntries(lua_State* L);
};


#endif //LUA_VAO_H