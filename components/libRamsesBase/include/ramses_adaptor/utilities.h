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

#include "components/DataChangeDispatcher.h"
#include "core/CodeControlledPropertyModifier.h"
#include "core/CoreFormatter.h"
#include "core/EditorObject.h"
#include "core/Handles.h"
#include "core/MeshCacheInterface.h"
#include "data_storage/Value.h"
#include "ramses_adaptor/BuildOptions.h"
#include "ramses_adaptor/SceneAdaptor.h"
#include "ramses_base/RamsesHandles.h"
#include "user_types/EngineTypeAnnotation.h"
#include "user_types/Material.h"
#include "user_types/Node.h"
#include "utils/MathUtils.h"

#include <memory>
#include <ramses-logic/Property.h>
#include <ramses-logic/RamsesNodeBinding.h>
#include <type_traits>

namespace raco::ramses_adaptor {

static constexpr const char* defaultVertexShader =
	"#version 300 es\n\
		precision mediump float;\n\
		in vec3 a_Position;\n\
		\n\
		uniform mat4 mvpMatrix;\n\
		void main() {\n\
			gl_Position = mvpMatrix * vec4(a_Position.xyz, 1.0);\n\
		}";

static constexpr const char* defaultVertexShaderWithNormals =
	R"(
#version 300 es
precision mediump float;
in vec3 a_Position;
in vec3 a_Normal;
out float lambertian;
uniform mat4 mvpMatrix;
void main() {
	lambertian = mix(0.4, 0.8, max(abs(dot(vec3(1.5, 2.4, 1.0), a_Normal)), 0.0));
	gl_Position = mvpMatrix * vec4(a_Position, 1.0);
}
)";

static constexpr const char* defaultFragmentShader =
	"#version 300 es\n\
		precision mediump float;\n\
		\n\
		out vec4 FragColor;\n\
		\n\
		void main() {\n\
			FragColor = vec4(1.0, 0.0, 0.2, 1.0); \n\
		}";

static constexpr const char* defaultFragmentShaderWithNormals =
	R"(
#version 300 es
precision mediump float;
in float lambertian;
out vec4 fragColor;
void main() {
	fragColor = vec4(1.0, 0.5, 0.0, 1.0) * lambertian;
}
)";

static constexpr const char* defaultEffectName = "raco::ramses_adaptor::DefaultEffectWithoutNormals";
static constexpr const char* defaultEffectWithNormalsName = "raco::ramses_adaptor::DefaultEffectWithNormals";
static constexpr const char* defaultAppearanceName = "raco::ramses_adaptor::DefaultAppearanceWithoutNormals";
static constexpr const char* defaultAppearanceWithNormalsName = "raco::ramses_adaptor::DefaultAppearanceWithNormals";
static constexpr const char* defaultIndexDataBufferName = "raco::ramses_adaptor::DefaultIndexDataBuffer";
static constexpr const char* defaultVertexDataBufferName = "raco::ramses_adaptor::DefaultVertexDataBuffer";
static constexpr const char* defaultRenderGroupName = "raco::ramses_adaptor::DefaultRenderGroup";
static constexpr const char* defaultRenderPassName = "raco::ramses_adaptor::DefaultRenderPass";

struct Vec3f {
	float x, y, z;
	bool operator==(const Vec3f& other) const {
		return x == other.x && y == other.y && z == other.z;
	}
	bool operator!=(const Vec3f& other) const {
		return x != other.x || y != other.y || z != other.z;
	}
};

struct Rotation final : public Vec3f {
	template <typename DataType>
	static void sync(const std::shared_ptr<DataType>& source, ramses::Node& target) {
		static_assert(std::is_base_of<user_types::EditorObject, DataType>::value);
		Rotation value{from<DataType>(source)};
		auto status = target.setRotation(value.x, value.y, value.z, raco::ramses_adaptor::RAMSES_ROTATION_CONVENTION);
		assert(status == ramses::StatusOK);
	}
	static Rotation from(const ramses::Node& node) {
		Rotation result;
		ramses::ERotationConvention convention;
		auto status = node.getRotation(result.x, result.y, result.z, convention);
		assert(status == ramses::StatusOK);
		return result;
	}
	template <typename DataType>
	static Rotation from(const std::shared_ptr<DataType>& node) {
		static_assert(std::is_base_of<user_types::EditorObject, DataType>::value);
		return {static_cast<float>(node->rotation_->x.asDouble()), static_cast<float>(node->rotation_->y.asDouble()), static_cast<float>(node->rotation_->z.asDouble())};
	}
};

struct Translation final : public Vec3f {
	template <typename DataType>
	static void sync(const std::shared_ptr<DataType>& source, ramses::Node& target) {
		static_assert(std::is_base_of<user_types::EditorObject, DataType>::value);
		Translation value{from<DataType>(source)};
		auto status = target.setTranslation(value.x, value.y, value.z);
		assert(status == ramses::StatusOK);
	}
	static Translation from(ramses::Node& node) {
		Translation result;
		auto status = node.getTranslation(result.x, result.y, result.z);
		assert(status == ramses::StatusOK);
		return result;
	}
	template <typename DataType>
	static Translation from(const std::shared_ptr<DataType>& node) {
		static_assert(std::is_base_of<user_types::EditorObject, DataType>::value);
		return {static_cast<float>(node->translation_->x.asDouble()), static_cast<float>(node->translation_->y.asDouble()), static_cast<float>(node->translation_->z.asDouble())};
	}
};

struct Scaling final : public Vec3f {
	template <typename DataType>
	static void sync(const std::shared_ptr<DataType>& source, ramses::Node& target) {
		static_assert(std::is_base_of<user_types::EditorObject, DataType>::value);
		Scaling value{from<DataType>(source)};
		auto status = target.setScaling(value.x, value.y, value.z);
		assert(status == ramses::StatusOK);
	}
	static Scaling from(const ramses::Node& node) {
		Scaling result;
		auto status = node.getScaling(result.x, result.y, result.z);
		assert(status == ramses::StatusOK);
		return result;
	}
	template <typename DataType>
	static Scaling from(const std::shared_ptr<DataType>& node) {
		static_assert(std::is_base_of<user_types::EditorObject, DataType>::value);
		return {static_cast<float>(node->scaling_->x.asDouble()), static_cast<float>(node->scaling_->y.asDouble()), static_cast<float>(node->scaling_->z.asDouble())};
	}
};

constexpr ramses::EDataType convert(core::MeshData::VertexAttribDataType type) {
	switch (type) {
		case core::MeshData::VertexAttribDataType::VAT_Float:
			return ramses::EDataType::Float;
		case core::MeshData::VertexAttribDataType::VAT_Float2:
			return ramses::EDataType::Vector2F;
		case core::MeshData::VertexAttribDataType::VAT_Float3:
			return ramses::EDataType::Vector3F;
		case core::MeshData::VertexAttribDataType::VAT_Float4:
			return ramses::EDataType::Vector4F;
		default:
			assert(false && "Unknown VertexAttribDataType");
	}
	return ramses::EDataType::Float;
}

template <typename... Args>
inline const rlogic::Property* propertyByNames(const rlogic::Property* property, Args... names);

template <typename T, typename... Args>
inline const rlogic::Property* propertyByNames(const rlogic::Property* property, const T& name, Args... names) {
	if constexpr (sizeof...(Args) > 0) {
		return propertyByNames(property->getChild(name), names...);
	} else {
		return property->getChild(name);
	}
}

inline bool setLuaInputInEngine(rlogic::Property* property, const core::ValueHandle& valueHandle) {
	assert(property != nullptr);
	using core::PrimitiveType;

	auto success{false};
	switch (valueHandle.type()) {
		case PrimitiveType::Double:
			success = property->set(static_cast<float>(valueHandle.as<double>()));
			break;
		case PrimitiveType::Int:
			success = property->set(valueHandle.as<int>());
			break;
		case PrimitiveType::Int64:
			success = property->set(valueHandle.as<int64_t>());
			break;
		case PrimitiveType::Bool:
			success = property->set(valueHandle.as<bool>());
			break;
		case PrimitiveType::Struct: {
			auto typeDesc = &valueHandle.constValueRef()->asStruct().getTypeDescription();
			if (typeDesc == &core::Vec2f::typeDescription) {
				success = property->set(rlogic::vec2f{valueHandle[0].as<float>(), valueHandle[1].as<float>()});
			} else if (typeDesc == &core::Vec3f::typeDescription) {
				success = property->set(rlogic::vec3f{valueHandle[0].as<float>(), valueHandle[1].as<float>(), valueHandle[2].as<float>()});
			} else if (typeDesc == &core::Vec4f::typeDescription) {
				success = property->set(rlogic::vec4f{valueHandle[0].as<float>(), valueHandle[1].as<float>(), valueHandle[2].as<float>(), valueHandle[3].as<float>()});
			} else if (typeDesc == &core::Vec2i::typeDescription) {
				success = property->set(rlogic::vec2i{valueHandle[0].as<int>(), valueHandle[1].as<int>()});
			} else if (typeDesc == &core::Vec3i::typeDescription) {
				success = property->set(rlogic::vec3i{valueHandle[0].as<int>(), valueHandle[1].as<int>(), valueHandle[2].as<int>()});
			} else if (typeDesc == &core::Vec4i::typeDescription) {
				success = property->set(rlogic::vec4i{valueHandle[0].as<int>(), valueHandle[1].as<int>(), valueHandle[2].as<int>(), valueHandle[3].as<int>()});
			}
			break;
		}
		case PrimitiveType::String:
			success = property->set(valueHandle.as<std::string>());
			break;
		case PrimitiveType::Table:
			success = true;
			for (size_t i{0}; i < valueHandle.size(); i++) {
				if (property->getType() == rlogic::EPropertyType::Array) {
					success = setLuaInputInEngine(property->getChild(i), valueHandle[i]) && success;
				} else {
					success = setLuaInputInEngine(property->getChild(valueHandle[i].getPropName()), valueHandle[i]) && success;
				}
			}
			break;
	}
	LOG_WARNING_IF(log_system::RAMSES_ADAPTOR, !success, "Script set properties failed: {}", property->getName());
	return success;
}


void getOutputFromEngine(const rlogic::Property& property, const core::ValueHandle& valueHandle, core::DataChangeRecorder& recorder);

inline void getComplexLuaOutputFromEngine(const rlogic::Property& property, const core::ValueHandle& valueHandle, core::DataChangeRecorder& recorder) {
	for (size_t i{0}; i < valueHandle.size(); i++) {
		if (property.getType() == rlogic::EPropertyType::Array) {
			getOutputFromEngine(*property.getChild(i), valueHandle[i], recorder);
		} else {
			getOutputFromEngine(*property.getChild(valueHandle[i].getPropName()), valueHandle[i], recorder);
		}
	}
}

inline void getOutputFromEngine(const rlogic::Property& property, const core::ValueHandle& valueHandle, core::DataChangeRecorder& recorder) {
	using core::PrimitiveType;

	// read quaternion rotation data
	if (valueHandle.isVec3f() && property.getType() == rlogic::EPropertyType::Vec4f) {
		auto [x, y, z, w] = property.get<rlogic::vec4f>().value();
		auto [eulerX, eulerY, eulerZ] = raco::utils::math::quaternionToXYZDegrees(x, y, z, w);
		core::CodeControlledPropertyModifier::setVec3f(valueHandle, eulerX, eulerY, eulerZ, recorder);
		return;
	}

	switch (valueHandle.type()) {
		case PrimitiveType::Double: {
			core::CodeControlledPropertyModifier::setPrimitive<double>(valueHandle, property.get<float>().value(), recorder);
			break;
		}
		case PrimitiveType::Int: {
			core::CodeControlledPropertyModifier::setPrimitive(valueHandle, property.get<int>().value(), recorder);
			break;
		}
		case PrimitiveType::Int64: {
			core::CodeControlledPropertyModifier::setPrimitive(valueHandle, property.get<int64_t>().value(), recorder);
			break;
		}
		case PrimitiveType::Bool: {
			core::CodeControlledPropertyModifier::setPrimitive(valueHandle, property.get<bool>().value(), recorder);
			break;
		}
		case PrimitiveType::String: {
			core::CodeControlledPropertyModifier::setPrimitive(valueHandle, property.get<std::string>().value(), recorder);
			break;
		}
		case PrimitiveType::Struct: {
			auto typeDesc = &valueHandle.constValueRef()->asStruct().getTypeDescription();
			if (typeDesc == &core::Vec2f::typeDescription) {
				auto [x, y] = property.get<rlogic::vec2f>().value();
				core::CodeControlledPropertyModifier::setVec2f(valueHandle, x, y, recorder);
			} else if (typeDesc == &core::Vec3f::typeDescription) {
				auto [x, y, z] = property.get<rlogic::vec3f>().value();
				core::CodeControlledPropertyModifier::setVec3f(valueHandle, x, y, z, recorder);
			} else if (typeDesc == &core::Vec4f::typeDescription) {
				auto [x, y, z, w] = property.get<rlogic::vec4f>().value();
				core::CodeControlledPropertyModifier::setVec4f(valueHandle, x, y, z, w, recorder);
			} else if (typeDesc == &core::Vec2i::typeDescription) {
				auto [i1, i2] = property.get<rlogic::vec2i>().value();
				core::CodeControlledPropertyModifier::setVec2i(valueHandle, i1, i2, recorder);
			} else if (typeDesc == &core::Vec3i::typeDescription) {
				auto [i1, i2, i3] = property.get<rlogic::vec3i>().value();
				core::CodeControlledPropertyModifier::setVec3i(valueHandle, i1, i2, i3, recorder);
			} else if (typeDesc == &core::Vec4i::typeDescription) {
				auto [i1, i2, i3, i4] = property.get<rlogic::vec4i>().value();
				core::CodeControlledPropertyModifier::setVec4i(valueHandle, i1, i2, i3, i4, recorder);
			} else {
				getComplexLuaOutputFromEngine(property, valueHandle, recorder);	
			}
			break;
		}
		case PrimitiveType::Table: {
			getComplexLuaOutputFromEngine(property, valueHandle, recorder);
			break;		
		}
	}
}

template<typename PropertyType, typename RamsesType>
std::vector<RamsesType> flattenUniformArrayOfVector(const core::ValueHandle& handle, int numComponents) {
	std::vector<RamsesType> values;
	for (size_t index = 0; index < handle.size(); index++) {
		for (size_t component = 0; component < numComponents; component++) {
			values.emplace_back(handle[index][component].as<PropertyType>());
		}
	}
	return values;
}

inline void setUniform(ramses::Appearance* appearance, const core::ValueHandle& valueHandle) {
	using core::PrimitiveType;
	ramses::UniformInput input;
	appearance->getEffect().findUniformInput(valueHandle.getPropName().c_str(), input);

	auto engineTypeAnno = valueHandle.query<raco::user_types::EngineTypeAnnotation>();
	switch (engineTypeAnno->type()) {
		case core::EnginePrimitive::Double:
			appearance->setInputValueFloat(input, valueHandle.as<float>());
			break;
		case core::EnginePrimitive::Int32:
		case core::EnginePrimitive::UInt16:
		case core::EnginePrimitive::UInt32:
			appearance->setInputValueInt32(input, static_cast<int32_t>(valueHandle.as<int>()));
			break;

		case core::EnginePrimitive::Vec2f:
			appearance->setInputValueVector2f(input, valueHandle[0].as<float>(), valueHandle[1].as<float>());
			break;

		case core::EnginePrimitive::Vec3f:
			appearance->setInputValueVector3f(input, valueHandle[0].as<float>(), valueHandle[1].as<float>(), valueHandle[2].as<float>());
			break;

		case core::EnginePrimitive::Vec4f:
			appearance->setInputValueVector4f(input, valueHandle[0].as<float>(), valueHandle[1].as<float>(), valueHandle[2].as<float>(), valueHandle[3].as<float>());
			break;

		case core::EnginePrimitive::Vec2i:
			appearance->setInputValueVector2i(input, static_cast<int32_t>(valueHandle[0].as<int>()), static_cast<int32_t>(valueHandle[1].as<int>()));
			break;

		case core::EnginePrimitive::Vec3i:
			appearance->setInputValueVector3i(input, static_cast<int32_t>(valueHandle[0].as<int>()), static_cast<int32_t>(valueHandle[1].as<int>()), static_cast<int32_t>(valueHandle[2].as<int>()));
			break;

		case core::EnginePrimitive::Vec4i:
			appearance->setInputValueVector4i(input, static_cast<int32_t>(valueHandle[0].as<int>()), static_cast<int32_t>(valueHandle[1].as<int>()), static_cast<int32_t>(valueHandle[2].as<int>()), static_cast<int32_t>(valueHandle[3].as<int>()));
			break;

		case core::EnginePrimitive::Array: {
			assert(valueHandle.size() >= 1);

			auto elementAnno = valueHandle[0].query<user_types::EngineTypeAnnotation>();
			switch (elementAnno->type()) {
				case core::EnginePrimitive::Double: {
					std::vector<float> values;
					for (size_t index = 0; index < valueHandle.size(); index++) {
						values.emplace_back(valueHandle[index].as<float>());
					}
					appearance->setInputValueFloat(input, values.size(), values.data());
					break;
				}

				case core::EnginePrimitive::Int32:
				case core::EnginePrimitive::UInt16:
				case core::EnginePrimitive::UInt32: {
					std::vector<int32_t> values;
					for (size_t index = 0; index < valueHandle.size(); index++) {
						values.emplace_back(valueHandle[index].as<int>());
					}
					appearance->setInputValueInt32(input, values.size(), values.data());
					break;
				}

				case core::EnginePrimitive::Vec2f: {
					auto values{flattenUniformArrayOfVector<double, float>(valueHandle, 2)};
					appearance->setInputValueVector2f(input, valueHandle.size(), values.data());
					break;
				}

				case core::EnginePrimitive::Vec3f: {
					auto values{flattenUniformArrayOfVector<double, float>(valueHandle, 3)};
					appearance->setInputValueVector3f(input, valueHandle.size(), values.data());
					break;
				}

				case core::EnginePrimitive::Vec4f: {
					auto values{flattenUniformArrayOfVector<double, float>(valueHandle, 4)};
					appearance->setInputValueVector4f(input, valueHandle.size(), values.data());
					break;
				}

				case core::EnginePrimitive::Vec2i: {
					auto values{flattenUniformArrayOfVector<int, int32_t>(valueHandle, 2)};
					appearance->setInputValueVector2i(input, valueHandle.size(), values.data());
					break;
				}

				case core::EnginePrimitive::Vec3i: {
					auto values{flattenUniformArrayOfVector<int, int32_t>(valueHandle, 3)};
					appearance->setInputValueVector3i(input, valueHandle.size(), values.data());
					break;
				}

				case core::EnginePrimitive::Vec4i: {
					auto values{flattenUniformArrayOfVector<int, int32_t>(valueHandle, 4)};
					appearance->setInputValueVector4i(input, valueHandle.size(), values.data());
					break;
				}

				default:
					break;
			}
		}

		default:
			break;
	}
}

inline void setDepthWrite(ramses::Appearance* appearance, const core::ValueHandle& valueHandle) {
	appearance->setDepthWrite(valueHandle.as<bool>() ? ramses::EDepthWrite_Enabled : ramses::EDepthWrite_Disabled);
}

inline void setDepthFunction(ramses::Appearance* appearance, const core::ValueHandle& valueHandle) {
	assert(valueHandle.type() == raco::core::PrimitiveType::Int);
	assert(valueHandle.asInt() >= 0 && valueHandle.asInt() < ramses::EDepthFunc_NUMBER_OF_ELEMENTS);
	appearance->setDepthFunction(static_cast<ramses::EDepthFunc>(valueHandle.asInt()));
}

inline ramses::EDepthWrite getDepthWriteMode(const ramses::Appearance* appearance) {
	ramses::EDepthWrite depthWrite;
	appearance->getDepthWriteMode(depthWrite);
	return depthWrite;
}

inline void setBlendMode(ramses::Appearance* appearance, const core::ValueHandle& options) {
	int colorOp = options.get("blendOperationColor").as<int>();
	assert(colorOp >= 0 && colorOp < ramses::EBlendOperation_NUMBER_OF_ELEMENTS);
	int alphaOp = options.get("blendOperationAlpha").as<int>();
	assert(alphaOp >= 0 && alphaOp < ramses::EBlendOperation_NUMBER_OF_ELEMENTS);
	appearance->setBlendingOperations(static_cast<ramses::EBlendOperation>(colorOp), static_cast<ramses::EBlendOperation>(alphaOp));

	int srcColor = options.get("blendFactorSrcColor").as<int>();
	assert(srcColor >= 0 && srcColor < ramses::EBlendFactor_NUMBER_OF_ELEMENTS);
	int destColor = options.get("blendFactorDestColor").as<int>();
	assert(destColor >= 0 && destColor < ramses::EBlendFactor_NUMBER_OF_ELEMENTS);
	int srcAlpha = options.get("blendFactorSrcAlpha").as<int>();
	assert(srcAlpha >= 0 && srcAlpha < ramses::EBlendFactor_NUMBER_OF_ELEMENTS);
	int destAlpha = options.get("blendFactorDestAlpha").as<int>();
	assert(destAlpha >= 0 && destAlpha < ramses::EBlendFactor_NUMBER_OF_ELEMENTS);
	appearance->setBlendingFactors(
		static_cast<ramses::EBlendFactor>(srcColor),
		static_cast<ramses::EBlendFactor>(destColor),
		static_cast<ramses::EBlendFactor>(srcAlpha),
		static_cast<ramses::EBlendFactor>(destAlpha));
}

inline void setBlendColor(ramses::Appearance* appearance, const core::ValueHandle& color) {
	appearance->setBlendingColor(
		color.get("x").as<float>(),
		color.get("y").as<float>(),
		color.get("z").as<float>(),
		color.get("w").as<float>());
}

inline void setCullMode(ramses::Appearance* appearance, const core::ValueHandle& valueHandle) {
	assert(valueHandle.type() == raco::core::PrimitiveType::Int);
	assert(valueHandle.asInt() >= 0 && valueHandle.asInt() < ramses::ECullMode::ECullMode_NUMBER_OF_ELEMENTS);
	appearance->setCullingMode(static_cast<ramses::ECullMode>(valueHandle.asInt()));
}

inline bool isArrayOfStructs(const rlogic::Property& property) {
	return property.getType() == rlogic::EPropertyType::Array && property.getChildCount() > 0 && property.getChild(0)->getType() == rlogic::EPropertyType::Struct;
}

raco::ramses_base::RamsesNodeBinding lookupNodeBinding(const SceneAdaptor* sceneAdaptor, core::SEditorObject node);

};	// namespace raco::ramses_adaptor
