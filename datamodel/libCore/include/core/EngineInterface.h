/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#pragma once

#include "core/BasicTypes.h"
#include <map>
#include <string>
#include <cassert>

namespace raco::core {

// !!! Careful: !!!
// We can't change the numeric value of the EUserTypeEnumerations members since these 
// are serialized via the EnumerationAnnotation::type_ property.
enum class EUserTypeEnumerations {
	Undefined = 0,
	CullMode = 1,
	BlendOperation = 2,
	BlendFactor = 3,
	DepthFunction = 4,
	TextureAddressMode = 5,
    TextureMinSamplingMethod = 6,
	TextureMagSamplingMethod = 7,
	TextureFormat = 8,
	// TextureOrigin = 9, // unused
	RenderBufferFormat = 10,
	RenderLayerOrder = 11,
	RenderLayerMaterialFilterMode = 12,
	FrustumType = 13,
	StencilFunction = 14,
	StencilOperation = 15
};

// Collects types of all possible dynamic properties, i.e. lua in/out properties and material uniforms.
// !!! Careful: !!!
// We can't change the numeric value of the EnginePrimitive members since these are
// serialized via the EngineTypeAnnotation::engineType_ property.
enum class EnginePrimitive {
	Undefined,
	Bool,
	Int32,
	UInt16,
	UInt32,
	Double,
	String,
	Vec2f,
	Vec3f,
	Vec4f,
	Vec2i,
	Vec3i,
	Vec4i,
	Struct,
	Array,
	TextureSampler2D,
	TextureSampler3D,
	TextureSamplerCube,
	// Types added later, in the bottom of the enum to avoid file format changing
	Int64,
	TextureSampler2DMS,
	TextureSamplerExternal
};

struct PropertyInterface;
using PropertyInterfaceList = std::vector<PropertyInterface>;
struct PropertyInterface {
	PropertyInterface(const std::string_view _name, EnginePrimitive _type) : name{_name}, type{_type} {}

	static PropertyInterface makeArrayOf(const std::string_view name, EnginePrimitive componentType, size_t numComponents) {
		PropertyInterface interface(name, EnginePrimitive::Array);
		for (size_t index = 0; index < numComponents; index++) {
			interface.children.emplace_back(std::string(), componentType);
		}
		return interface;
	}

	std::string name;
	EnginePrimitive type;
	PropertyInterfaceList children{};

	static data_storage::PrimitiveType primitiveType(EnginePrimitive type) {
		static std::map<EnginePrimitive, data_storage::PrimitiveType> typeMap = {
			{EnginePrimitive::Bool, data_storage::PrimitiveType::Bool},
			{EnginePrimitive::Int32, data_storage::PrimitiveType::Int},
			{EnginePrimitive::Int64, data_storage::PrimitiveType::Int64},
			{EnginePrimitive::UInt16, data_storage::PrimitiveType::Int},
			{EnginePrimitive::UInt32, data_storage::PrimitiveType::Int},
			{EnginePrimitive::Double, data_storage::PrimitiveType::Double},
			{EnginePrimitive::String, data_storage::PrimitiveType::String},
			{EnginePrimitive::Vec2f, data_storage::PrimitiveType::Struct},
			{EnginePrimitive::Vec3f, data_storage::PrimitiveType::Struct},
			{EnginePrimitive::Vec4f, data_storage::PrimitiveType::Struct},
			{EnginePrimitive::Vec2i, data_storage::PrimitiveType::Struct},
			{EnginePrimitive::Vec3i, data_storage::PrimitiveType::Struct},
			{EnginePrimitive::Vec4i, data_storage::PrimitiveType::Struct},
			{EnginePrimitive::Array, data_storage::PrimitiveType::Table},
			{EnginePrimitive::Struct, data_storage::PrimitiveType::Table},
			{EnginePrimitive::TextureSampler2D, data_storage::PrimitiveType::Ref},
			{EnginePrimitive::TextureSampler2DMS, data_storage::PrimitiveType::Ref},
			{EnginePrimitive::TextureSampler3D, data_storage::PrimitiveType::Ref},
			{EnginePrimitive::TextureSamplerCube, data_storage::PrimitiveType::Ref},
			{EnginePrimitive::TextureSamplerExternal, data_storage::PrimitiveType::Ref}
		};

		auto it = typeMap.find(type);
		assert(it != typeMap.end());
		return it->second;
	}

	data_storage::PrimitiveType primitiveType() const noexcept {
		return primitiveType(type);
	}
};

class EngineInterface {
public:
	virtual ~EngineInterface() = default;

	// Parse shaders using Ramses and return set of uniforms with name and type.
	// Returns true if shader can be successfully parsed.
	virtual bool parseShader(const std::string& vertexShader, const std::string& geometryShader, const std::string& fragmentShader, const std::string& shaderDefines, PropertyInterfaceList& outUniforms, raco::core::PropertyInterfaceList& outAttributes, std::string& error) = 0;

	// Parse luascripts using ramses logic and return set of in and out parameters with name and type.
	// Returns true if script can be successfully parsed.
	virtual bool parseLuaScript(const std::string& luaScript, const std::string& scriptName, const std::vector<std::string>& stdModules, const raco::data_storage::Table& modules, PropertyInterfaceList& outInputs, PropertyInterfaceList& outOutputs, std::string& error) = 0;

	// Parse lua interface definition using ramses logic and return set of inout parameters with name and type.
	// Returns true if script can be successfully parsed.
	virtual bool parseLuaInterface(const std::string& luaScript, const std::vector<std::string>& stdModules, const raco::data_storage::Table& modules, bool useModules, PropertyInterfaceList& outInputs, std::string& error) = 0;

	// Parse luascript module using ramses logic.
	// Returns true if module can be successfully parsed.
	virtual bool parseLuaScriptModule(raco::core::SEditorObject object, const std::string& luaScriptModule, const std::string& moduleName, const std::vector<std::string>& stdModules, std::string& outError) = 0;

	virtual void removeModuleFromCache(raco::core::SCEditorObject object) = 0;
	virtual void clearModuleCache() = 0;


	// Extract module dependencies from lua script using ramses logic to parse the script.
	//
	// @param luaScript the code of the luascript
	// @param moduleList will be set by to the names of the modules used by the lua script. is not cleared on success.
	// @param outError will be set to the error message if parsing was not succesful. is not cleared on success.
	// @return True indicates success at parsing and extracting the dependencies. If false outError will contain the error message.
	virtual bool extractLuaDependencies(const std::string& luaScript, std::vector<std::string>& moduleList, std::string &outError) = 0;

	virtual std::string luaNameForPrimitiveType(EnginePrimitive engineType) const = 0;
};

}  // namespace raco::core
