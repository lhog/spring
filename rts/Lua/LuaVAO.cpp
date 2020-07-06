#include "LuaVAO.hpp"

#include <iterator>
#include <limits>
#include <algorithm>

#include "Rendering/GL/VBO.h"
#include "Rendering/GL/VAO.h"

#include "LuaUtils.h"

inline LuaVAOImpl::LuaVAOImpl(const sol::optional<bool> freqUpdatedOpt) :
	freqUpdated(freqUpdatedOpt.value_or(false)),

	vao(nullptr),
	vboVert(nullptr),
	vboInst(nullptr),
	ebo(nullptr),

	numAttributes(0),

	maxVertCount(0),
	maxInstCount(0),
	maxIndxCount(0),

	vertAttribsSizeInBytes(0),
	instAttribsSizeInBytes(0),
	indElemSizeInBytes(0)
{
	LOG("LuaVAOImpl()");
}

inline LuaVAOImpl::~LuaVAOImpl()
{
	LOG("~LuaVAOImpl()");

	if (vao) delete vao;

	if (vboVert) delete vboVert;
	if (vboInst) delete vboInst;
	if (ebo) delete ebo;

	vaoAttribs.clear();
}



template<typename TIn, typename TOut>
inline TOut LuaVAOImpl::TransformFunc(const TIn input) {
	if constexpr(std::is_same<TIn, TOut>::value)
		return input;

	const auto m = static_cast<TIn>(std::numeric_limits<TOut>::min());
	const auto M = static_cast<TIn>(std::numeric_limits<TOut>::max());

	return static_cast<TOut>(std::clamp(input, m, M));
}


template<typename T>
inline T LuaVAOImpl::MaybeFunc(const sol::table& tbl, const std::string& key, T defValue) {
	const sol::optional<T> maybeValue = tbl[key];
	return maybeValue.value_or(defValue);
}


/**
* local vertexAtrribs = {
*		[0] = {type = GL.FLOAT, ...}
* }
*/
bool LuaVAOImpl::FillAttribsTableImpl(const sol::table& attrDefTable, const int divisor)
{
	bool atLeastOne = false;
	for (const auto& kv : attrDefTable) {
		const sol::object& key = kv.first;
		const sol::object& value = kv.second;

		if (numAttributes >= LuaVAOImpl::glMaxNumOfAttributes)
			return false;

		if (!key.is<int>() || value.get_type() != sol::type::table) //key should be int, value should be table i.e. [1] = {}
			continue;

		sol::table vaDefTable = value.as<sol::table>();

		const int attrID = MaybeFunc(vaDefTable, "id", numAttributes);

		if ((attrID < 0) || (attrID > LuaVAOImpl::glMaxNumOfAttributes) || vaoAttribs.find(attrID) != vaoAttribs.end())
			continue;

		const GLenum type = MaybeFunc(vaDefTable, "type", LuaVAOImpl::defaultVertexType);
		const GLboolean normalized = MaybeFunc(vaDefTable, "normalized", false) ? GL_TRUE : GL_FALSE;
		const GLint size = std::clamp(MaybeFunc(vaDefTable, "size", 4), 1, 4);
		const std::string name = MaybeFunc(vaDefTable, "name", fmt::format("attr{}", attrID));

		int typeSizeInBytes = 0;

		switch (type) {
		case GL_BYTE:
		case GL_UNSIGNED_BYTE:
			typeSizeInBytes = sizeof(uint8_t);
			break;
		case GL_SHORT:
		case GL_UNSIGNED_SHORT:
			typeSizeInBytes = sizeof(uint16_t);
			break;
		case GL_INT:
		case GL_UNSIGNED_INT:
			typeSizeInBytes = sizeof(uint32_t);
			break;
		case GL_FLOAT:
			typeSizeInBytes = sizeof(GLfloat);
			break;
		}

		if (typeSizeInBytes == 0)
			continue;

		vaoAttribs[attrID] = {
			divisor,
			size,
			type,
			normalized,
			0, // stride , will be filled up later
			0, // pointer, will be filled up later
			//AUX
			typeSizeInBytes,
			name
		};
		atLeastOne = true;
		++numAttributes;
	}

	return atLeastOne;
}

bool LuaVAOImpl::FillAttribsNumberImpl(const int numFloatAttribs, const int divisor)
{
	if (numFloatAttribs <= 0)
		return false;

	if (numFloatAttribs % 4 != 0)
		return false;

	bool atLeastOne = false;

	constexpr GLenum type = GL_FLOAT;
	constexpr GLboolean normalized = GL_FALSE;
	constexpr GLint size = 4;
	constexpr int typeSizeInBytes = sizeof(GLfloat);

	for (int attrID = 0; attrID < std::ceil(numFloatAttribs / 4.0f); ++attrID) {

		const std::string name = fmt::format("attr{}", attrID);

		vaoAttribs[attrID] = {
			divisor,
			size,
			type,
			normalized,
			0, // stride , will be filled up later
			0, // pointer, will be filled up later
			//AUX
			typeSizeInBytes,
			name
		};
		atLeastOne = true;
		++numAttributes;
	}

	return atLeastOne;
}

void LuaVAOImpl::FillAttribsImpl(const sol::object& attrDefObject, const int divisor, int* attribsSizeInBytes)
{
	switch (attrDefObject.get_type()) {
	case sol::type::table:
		if (!FillAttribsTableImpl(attrDefObject.as<const sol::table&>(), divisor))
			return;
		break;
	case sol::type::number:
		if (!attrDefObject.is<int>() || (!FillAttribsNumberImpl(attrDefObject.as<const int>(), divisor)))
			return;
		break;
	default:
		return;
	}

	*attribsSizeInBytes = 0;
	GLsizei pointer = 0;

	//second pass is necessary because attrDefTable can be iterated in any order, but vaoAttribs order is guaranteed
	for (auto& kv : vaoAttribs) {
		if (kv.second.divisor != divisor)
			continue;

		const auto vaIndex = kv.first;
		const auto& attr = kv.second;

		const GLsizei attribSizeInBytes = attr.size * attr.typeSizeInBytes;
		*attribsSizeInBytes += attribSizeInBytes;

		vaoAttribs[vaIndex].pointer = pointer;

		pointer += attribSizeInBytes; //the pointer points to the next attrib
	}

	//third pass is need to fill in stride value, which is same as sizeof imaginary attribs structure
	for (auto& kv : vaoAttribs) {
		if (kv.second.divisor != divisor)
			continue;

		kv.second.stride = *attribsSizeInBytes;

#if 0
		LOG("Attribute name = %s, divisor = %d, size = %d, type = %d, normalized = %d, stride = %d, pointer = %d, typeSizeInBytes = %d", kv.second.name.c_str(), kv.second.divisor, kv.second.size, kv.second.type, kv.second.normalized, kv.second.stride, kv.second.pointer, kv.second.typeSizeInBytes);
#endif

	}
}

int LuaVAOImpl::SetVertexAttributes(const int maxVertCount, const sol::object& attrDefObject)
{
	if (vboVert)
		return 0;

	if (maxVertCount <= 0)
		return 0;

	this->maxVertCount = maxVertCount;

	FillAttribsImpl(attrDefObject, 0/*per vertex divisor*/, &this->vertAttribsSizeInBytes);

	if (this->vertAttribsSizeInBytes <= 0)
		return 0;

	vboVert = new VBO(GL_ARRAY_BUFFER, this->freqUpdated);
	vboVert->Bind();
	vboVert->New(this->maxVertCount * this->vertAttribsSizeInBytes, (this->freqUpdated ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW));
	vboVert->Unbind();

	return numAttributes;
}

int LuaVAOImpl::SetInstanceAttributes(const int maxInstCount, const sol::object& attrDefObject)
{
	if (vboInst)
		return 0;

	if (maxInstCount <= 0)
		return 0;

	this->maxInstCount = maxInstCount;

	FillAttribsImpl(attrDefObject, 1/*per instance divisor*/, &this->instAttribsSizeInBytes);

	if (this->instAttribsSizeInBytes <= 0)
		return 0;

	vboInst = new VBO(GL_ARRAY_BUFFER, this->freqUpdated);
	vboInst->Bind();
	vboInst->New(this->maxInstCount * this->instAttribsSizeInBytes, (this->freqUpdated ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW));
	vboInst->Unbind();

	return numAttributes;
}

bool LuaVAOImpl::SetIndexAttributes(const int maxIndxCount, const sol::optional<GLenum> indTypeOpt)
{
	const auto indexType = indTypeOpt.value_or(LuaVAOImpl::defaultIndexType);
	return SetIndexAttributesImpl(maxIndxCount, indexType);
}

bool LuaVAOImpl::CheckPrimType(GLenum mode)
{
	switch (mode) {
	case GL_POINTS:
	case GL_LINE_STRIP:
	case GL_LINE_LOOP:
	case GL_LINES:
	case GL_LINE_STRIP_ADJACENCY:
	case GL_LINES_ADJACENCY:
	case GL_TRIANGLE_STRIP:
	case GL_TRIANGLE_FAN:
	case GL_TRIANGLES:
	case GL_TRIANGLE_STRIP_ADJACENCY:
	case GL_TRIANGLES_ADJACENCY:
	case GL_PATCHES:
		break;
	default:
		LOG("%s(): Using deprecated primType (%d)", __func__, mode);
		return false;
	}

	return true;
}

bool LuaVAOImpl::CondInitVAO()
{
	if (vao)
		return true; //already init

	if (this->maxVertCount <= 0)
		return false;

	vao = new VAO();

	bool vboVertBound = false;
	bool vboInstBound = false;
	bool eboBound = false;

	////
	vao->Bind();
	////

	//LOG("VAO = %d", vao->GetId());

	if (vboVert) {
		vboVert->Bind();
		//LOG("vboVert = %d", vboVert->GetId());
		vboVertBound = true;
	}

	if (ebo) {
		ebo->Bind();
		//LOG("ebo = %d", ebo->GetId());
		eboBound = true;
	}

	#define INT2PTR(x) ((void*)static_cast<intptr_t>(x))

	for (const auto& va : vaoAttribs) {
		const auto& attr = va.second;
		if (attr.divisor == 0) {
			glEnableVertexAttribArray(va.first);
			glVertexAttribPointer(va.first, attr.size, attr.type, attr.normalized, attr.stride, INT2PTR(attr.pointer));
			glVertexAttribDivisor(va.first, 0);
			//LOG("attrID = %d, divisor = %d, size = %d, type = %d, normalized = %d, stride = %d, pointer = %d", va.first, attr.divisor, attr.size, attr.type, attr.normalized, attr.stride, attr.pointer);
		}
	}

	if (vboInst) {
		if (vboVertBound) {
			vboVert->Unbind();
			vboVertBound = false;
		}

		vboInst->Bind();
		vboInstBound = true;
		//LOG("vboInst = %d", vboInst->GetId());

		for (const auto& va : vaoAttribs) {
			const auto& attr = va.second;
			if (attr.divisor >= 1) {
				glEnableVertexAttribArray(va.first);
				glVertexAttribPointer(va.first, attr.size, attr.type, attr.normalized, attr.stride, INT2PTR(attr.pointer));
				glVertexAttribDivisor(va.first, attr.divisor);
				//LOG("attrID = %d, divisor = %d, size = %d, type = %d, normalized = %d, stride = %d, pointer = %d", va.first, attr.divisor, attr.size, attr.type, attr.normalized, attr.stride, attr.pointer);
			}
		}
	}

	#undef INT2PTR

	////
	vao->Unbind();
	////

	if (vboVert && vboVertBound)
		vboVert->Unbind();

	if (vboInst && vboInstBound)
		vboInst->Unbind();

	if (ebo && eboBound)
		ebo->Unbind();

	//restore default state
	for (const auto& va : vaoAttribs) {
		glVertexAttribDivisor(va.first, 0);
		glDisableVertexAttribArray(va.first);
	}

	return true;
}

bool LuaVAOImpl::SetIndexAttributesImpl(const int maxIndxCount, const GLenum indType)
{
	if (ebo)
		return false;

	if (maxIndxCount <= 0)
		return false;

	this->maxIndxCount = maxIndxCount;
	this->indexType = indType;

	int typeSizeInBytes = 0;
	switch (this->indexType) {
	case GL_UNSIGNED_BYTE:
		typeSizeInBytes = sizeof(uint8_t);
		break;
	case GL_UNSIGNED_SHORT:
		typeSizeInBytes = sizeof(uint16_t);
		break;
	case GL_UNSIGNED_INT:
		typeSizeInBytes = sizeof(uint32_t);
		break;
	}

	if (typeSizeInBytes == 0)
		return false;

	this->indElemSizeInBytes = typeSizeInBytes;
	ebo = new VBO(GL_ELEMENT_ARRAY_BUFFER, this->freqUpdated);
	ebo->Bind();
	ebo->New(this->maxIndxCount * this->indElemSizeInBytes, (this->freqUpdated ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW));
	ebo->Unbind();

	return true;
}

int LuaVAOImpl::UploadImpl(const sol::table& luaTblData, const sol::optional<int> offsetOpt, const int divisor, const int* attrNum, const int aSizeInBytes, VBO* vbo)
{
	if (!vbo)
		return 0;

	const int aOffset = std::max(offsetOpt.value_or(0), 0);
	const int byteOffset = aOffset * aSizeInBytes;

	const int byteSize = vbo->GetSize() - byteOffset; //non-precise, but save way

	if (byteSize <= 0)
		return 0;

	int bytesWritten = 0;

	std::vector<lua_Number> dataVec;
	dataVec.resize(luaTblData.size());

	for (auto k = 0; k < luaTblData.size(); ++k) {
		const auto& maybeNum = luaTblData.get<sol::optional<lua_Number>>(k + 1);
		dataVec.at(k) = maybeNum.value_or(static_cast<lua_Number>(0));
	}

	vbo->Bind();
	auto mappedBuf = vbo->MapBuffer(byteOffset, byteSize, GL_MAP_WRITE_BIT);

	#define TRANSFORM_COPY_ATTRIB(t, sz, iter) \
		for (int n = 0; n < sz; ++n) { \
			const auto outVal = TransformFunc<lua_Number, t>(*iter); \
			const auto outValSize = sizeof(outVal); \
			if (bytesWritten + outValSize > byteSize) { \
				vbo->UnmapBuffer(); \
				vbo->Unbind(); \
				return bytesWritten; \
			} \
			memcpy(mappedBuf, &outVal, outValSize); \
			mappedBuf += outValSize; \
			bytesWritten += outValSize; \
			++iter; \
		}

	for (auto bdvIter = dataVec.cbegin(); bdvIter != dataVec.cend(); ) {
		for (const auto& va : vaoAttribs) {
			const auto& attr = va.second;

			if (attrNum && *attrNum != va.first) //not the attribute num we are interested in
				continue;

			if (attr.divisor != divisor) //not the divisor we are interested in
				continue;

			switch (attr.type) {
			case GL_BYTE:
				TRANSFORM_COPY_ATTRIB(int8_t, attr.size, bdvIter);
				break;
			case GL_UNSIGNED_BYTE:
				TRANSFORM_COPY_ATTRIB(uint8_t, attr.size, bdvIter);
				break;
			case GL_SHORT:
				TRANSFORM_COPY_ATTRIB(int16_t, attr.size, bdvIter);
				break;
			case GL_UNSIGNED_SHORT:
				TRANSFORM_COPY_ATTRIB(uint16_t, attr.size, bdvIter);
				break;
			case GL_INT:
				TRANSFORM_COPY_ATTRIB(int32_t, attr.size, bdvIter);
				break;
			case GL_UNSIGNED_INT:
				TRANSFORM_COPY_ATTRIB(uint32_t, attr.size, bdvIter);
				break;
			case GL_FLOAT:
				TRANSFORM_COPY_ATTRIB(GLfloat, attr.size, bdvIter);
				break;
			}
		}
	}

	#undef TRANSFORM_COPY_ATTRIB

	vbo->UnmapBuffer();
	vbo->Unbind();

	return bytesWritten;
}

int LuaVAOImpl::UploadVertexBulk(const sol::table& bulkData, const sol::optional<int> vertexOffsetOpt)
{
	return UploadImpl(bulkData, vertexOffsetOpt, 0, nullptr, this->vertAttribsSizeInBytes, vboVert);
}

int LuaVAOImpl::UploadInstanceBulk(const sol::table& bulkData, const sol::optional<int> instanceOffsetOpt)
{
	return UploadImpl(bulkData, instanceOffsetOpt, 1, nullptr, this->instAttribsSizeInBytes, vboInst);
}

int LuaVAOImpl::UploadVertexAttribute(const int attrIndex, const sol::table& attrData, const sol::optional<int> vertexOffsetOpt)
{
	return UploadImpl(attrData, vertexOffsetOpt, 0, &attrIndex, this->vertAttribsSizeInBytes, vboVert);
}

int LuaVAOImpl::UploadInstanceAttribute(const int attrIndex, const sol::table& attrData, const sol::optional<int> instanceOffsetOpt)
{
	return UploadImpl(attrData, instanceOffsetOpt, 1, &attrIndex, this->instAttribsSizeInBytes, vboInst);
}

int LuaVAOImpl::UploadIndices(const sol::table& indData, const sol::optional<int> indOffsetOpt)
{
	const int indDataSize = indData.size();
	if (indDataSize <= 0)
		return 0;

	if (!ebo) { //allow skipping SetIndexAttributes() call from Lua and set attributes based on defaults and the content of first invocation of UploadInidces()
		if (!SetIndexAttributesImpl(indDataSize, LuaVAOImpl::defaultIndexType))
			return 0;
	}

	const int aOffset = std::max(indOffsetOpt.value_or(0), 0);
	const int byteOffset = aOffset * indElemSizeInBytes;

	const int byteSize = ebo->GetSize() - byteOffset; //non-precise, but save way

	if (byteSize <= 0)
		return 0;

	int bytesWritten = 0;

	std::vector<lua_Number> dataVec;
	dataVec.resize(indDataSize);

	for (auto k = 0; k < indDataSize; ++k) {
		const auto& maybeNum = indData.get<sol::optional<lua_Number>>(k + 1);
		dataVec.at(k) = maybeNum.value_or(static_cast<lua_Number>(0));
	}

	ebo->Bind();
	auto mappedBuf = ebo->MapBuffer(byteOffset, byteSize, GL_MAP_WRITE_BIT);

#define TRANSFORM_COPY_INDEX(outT, iter) \
	{ \
		const auto outVal = TransformFunc<lua_Number, outT>(*iter); \
		const auto outValSize = sizeof(outVal); \
		if (bytesWritten + outValSize > byteSize) {	\
			ebo->UnmapBuffer(); \
			ebo->Unbind(); \
			return bytesWritten; \
		} \
		memcpy(mappedBuf, &outVal, outValSize); \
		mappedBuf += outValSize; \
		bytesWritten += outValSize;\
	}

	for (auto bdvIter = dataVec.cbegin(); bdvIter != dataVec.cend(); ++bdvIter) {
		switch (indexType) {
		case GL_UNSIGNED_BYTE:
			TRANSFORM_COPY_INDEX(uint8_t, bdvIter);
			break;
		case GL_UNSIGNED_SHORT:
			TRANSFORM_COPY_INDEX(uint16_t, bdvIter);
			break;
		case GL_UNSIGNED_INT:
			TRANSFORM_COPY_INDEX(uint32_t, bdvIter);
			break;
		}
	}

#undef TRANSFORM_COPY_INDEX

	ebo->UnmapBuffer();
	ebo->Unbind();

	return bytesWritten;
}

bool LuaVAOImpl::DrawArrays(const GLenum mode, const sol::optional<GLsizei> vertCountOpt, const sol::optional<GLint> firstOpt, const sol::optional<int> instanceCountOpt)
{
	const auto vertCount = std::max(vertCountOpt.value_or(maxVertCount), 0);

	if (this->maxVertCount <= 0)
		return false;

	if (vertCount <= 0)
		return false;

	if (!CheckPrimType(mode))
		return false;

	if (!CondInitVAO())
		return false;

	const auto instanceCount = std::max(instanceCountOpt.value_or(0), 0); // 0 - forces ordinary version, while 1 - calls *Instanced()
	const auto first = std::max(firstOpt.value_or(0), 0);

	//LOG("mode = %d, first = %d, vertCount = %d, instanceCount = %d", mode, first, vertCount, instanceCount);

	vao->Bind();

	if (instanceCount == 0)
		glDrawArrays(mode, first, vertCount);
	else
		glDrawArraysInstanced(mode, first, vertCount, instanceCount);

	vao->Unbind();

	return true;
}

bool LuaVAOImpl::DrawElements(const GLenum mode, const sol::optional<GLsizei> indCountOpt, const sol::optional<int> indElemOffsetOpt, const sol::optional<int> instanceCountOpt)
{
	const auto indCount = std::max(indCountOpt.value_or(maxIndxCount), 0);

	if ((this->maxVertCount <= 0) || (this->maxIndxCount <= 0))
		return false;

	if (indCount <= 0)
		return false;

	if (!CheckPrimType(mode))
		return false;

	if (!CondInitVAO())
		return false;

	const auto indElemOffset = std::max(indElemOffsetOpt.value_or(0), 0);
	const auto indElemOffsetInBytes = indElemOffset * indElemSizeInBytes;

	const auto instanceCount = std::max(instanceCountOpt.value_or(0), 0); // 0 - forces ordinary version, while 1 - calls *Instanced()

	vao->Bind();

#define INT2PTR(x) ((void*)static_cast<intptr_t>(x))
	if (instanceCount == 0)
		glDrawElements(mode, indCount, this->indexType, INT2PTR(indElemOffsetInBytes));
	else
		glDrawElementsInstanced(mode, indCount, this->indexType, INT2PTR(indElemOffsetInBytes), instanceCount);
#undef INT2PTR

	vao->Unbind();

	return true;
}


///////////////////////////////////////////////////////////

bool LuaVAO::PostPushEntries(lua_State* L)
{
	sol::state_view lua(L);
	auto gl = lua.get<sol::table>("gl");

	gl.new_usertype<LuaVAOImpl>("VAO",
		sol::constructors<LuaVAOImpl(const sol::optional<bool>)>(),
		"SetVertexAttributes", &LuaVAOImpl::SetVertexAttributes,
		"SetInstanceAttributes", &LuaVAOImpl::SetInstanceAttributes,
		"SetIndexAttributes", &LuaVAOImpl::SetIndexAttributes,

		"UploadVertexBulk", &LuaVAOImpl::UploadVertexBulk,
		"UploadInstanceBulk", &LuaVAOImpl::UploadInstanceBulk,

		"UploadVertexAttribute", &LuaVAOImpl::UploadVertexAttribute,
		"UploadInstanceAttribute", &LuaVAOImpl::UploadInstanceAttribute,

		"UploadIndices", &LuaVAOImpl::UploadIndices,

		"DrawArrays", &LuaVAOImpl::DrawArrays,
		"DrawElements", &LuaVAOImpl::DrawElements
	);

	gl.set("VAO", sol::lua_nil); //because :)

	return true;
}


bool LuaVAO::PushEntries(lua_State* L)
{
	REGISTER_LUA_CFUNC(GetVAO);

	return true;
}

int LuaVAO::GetVAO(lua_State* L)
{
	return sol::stack::call_lua(L, 1, [=](const sol::optional<bool> freqUpdatedOpt) {
		return std::move(LuaVAOImpl{ freqUpdatedOpt.value_or(false) });
	});
	//LuaVAOImpl lvp{};
	//return sol::stack::push(L, std::move(lvp));
}