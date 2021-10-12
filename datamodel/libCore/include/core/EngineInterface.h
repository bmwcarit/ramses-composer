/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#pragma once

#include "data_storage/BasicTypes.h"
#include <map>
#include <string>
#include <cassert>

namespace raco::core {

enum EngineEnumeration {
	Undefined = 0,
	CullMode,
	BlendOperation,
	BlendFactor,
	DepthFunction,
	TextureAddressMode,
    TextureMinSamplingMethod,
	TextureMagSamplingMethod,
	TextureFormat,
	TextureOrigin,
	RenderBufferFormat,
	RenderLayerOrder,
	RenderLayerMaterialFilterFlag
};

// Collects types of all possible dynamic properties, i.e. lua in/out properties and material uniforms
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
	TextureSamplerCube
};

struct PropertyInterface;
using PropertyInterfaceList = std::vector<PropertyInterface>;
struct PropertyInterface {
	PropertyInterface(const std::string_view _name, EnginePrimitive _type) : name{_name}, type{_type} {}

	std::string name;
	EnginePrimitive type;
	PropertyInterfaceList children{};

	static data_storage::PrimitiveType primitiveType(EnginePrimitive type) {
		static std::map<EnginePrimitive, data_storage::PrimitiveType> typeMap = {
			{EnginePrimitive::Bool, data_storage::PrimitiveType::Bool},
			{EnginePrimitive::Int32, data_storage::PrimitiveType::Int},
			{EnginePrimitive::UInt16, data_storage::PrimitiveType::Int},
			{EnginePrimitive::UInt32, data_storage::PrimitiveType::Int},
			{EnginePrimitive::Double, data_storage::PrimitiveType::Double},
			{EnginePrimitive::String, data_storage::PrimitiveType::String},
			{EnginePrimitive::Vec2f, data_storage::PrimitiveType::Vec2f},
			{EnginePrimitive::Vec3f, data_storage::PrimitiveType::Vec3f},
			{EnginePrimitive::Vec4f, data_storage::PrimitiveType::Vec4f},
			{EnginePrimitive::Vec2i, data_storage::PrimitiveType::Vec2i},
			{EnginePrimitive::Vec3i, data_storage::PrimitiveType::Vec3i},
			{EnginePrimitive::Vec4i, data_storage::PrimitiveType::Vec4i},
			{EnginePrimitive::Array, data_storage::PrimitiveType::Table},
			{EnginePrimitive::Struct, data_storage::PrimitiveType::Table},
			{EnginePrimitive::TextureSampler2D, data_storage::PrimitiveType::Ref},
			{EnginePrimitive::TextureSampler3D, data_storage::PrimitiveType::Ref},
			{EnginePrimitive::TextureSamplerCube, data_storage::PrimitiveType::Ref}};

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
	virtual bool parseShader(const std::string& vertexShader, const std::string& geometryShader, const std::string& fragmentShader, const std::string& shaderDefines, PropertyInterfaceList& outUniforms, raco::core::PropertyInterfaceList& outAttributes, std::string& error) = 0;
	virtual bool parseLuaScript(const std::string& luaScript, PropertyInterfaceList& outInputs, PropertyInterfaceList& outOutputs, std::string& error) = 0;
	virtual const std::map<int, std::string>& enumerationDescription(EngineEnumeration type) const = 0;
};

}  // namespace raco::core
