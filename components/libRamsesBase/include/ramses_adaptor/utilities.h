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
#include "ramses_base/EnumerationTranslations.h"
#include "ramses_base/RamsesHandles.h"
#include "user_types/EngineTypeAnnotation.h"
#include "user_types/Enumerations.h"
#include "user_types/Material.h"
#include "user_types/Node.h"
#include "utils/MathUtils.h"

#include <memory>
#include <ramses/client/logic/Property.h>
#include <ramses/client/logic/NodeBinding.h>
#include <type_traits>

namespace raco::ramses_adaptor {

class SceneAdaptor;


struct BoundingBox {
	BoundingBox()
		: min_(std::numeric_limits<float>::max()),
		  max_(std::numeric_limits<float>::lowest()) {
	}

	BoundingBox(glm::vec3 min, glm::vec3 max)
		: min_(min),
		  max_(max) {
	}

	glm::vec3 min_;
	glm::vec3 max_;

	bool empty() const {
		return min_.x >= max_.x || min_.y >= max_.y || min_.z >= max_.z;
	}

	glm::vec3 center() const {
		return (min_ + max_) / 2.0f;
	}

	float size() const {
		return glm::length(max_ - min_) / 2.0f;
	}

	void merge(glm::vec3 point) {
		min_ = glm::min(min_, point);
		max_ = glm::max(max_, point);
	}

	 void merge(const BoundingBox& bbox) {
		min_ = glm::min(min_, bbox.min_);
		max_ = glm::max(max_, bbox.max_);
	 }
};


inline glm::vec3 getRamsesRotation(const ramses::Node* ramsesNode) {
	glm::vec3 vec;
	ramsesNode->getRotation(vec);
	return vec;
}

inline glm::vec3 getRacoRotation(user_types::SNode node) {
	return core::ValueHandle(node, &user_types::Node::rotation_).as<glm::vec3>();
}

inline glm::vec3 getRamsesTranslation(const ramses::Node* ramsesNode) {
	glm::vec3 vec;
	ramsesNode->getTranslation(vec);
	return vec;
}

inline glm::vec3 getRacoTranslation(user_types::SNode node) {
	return core::ValueHandle(node, &user_types::Node::translation_).as<glm::vec3>();
}

inline glm::vec3 getRamsesScaling(const ramses::Node* ramsesNode) {
	glm::vec3 vec;
	ramsesNode->getScaling(vec);
	return vec;
}

inline glm::vec3 getRacoScaling(user_types::SNode node) {
	return core::ValueHandle(node, &user_types::Node::scaling_).as<glm::vec3>();
}

template <typename T, typename... Args>
inline const ramses::Property* propertyByNames(const ramses::Property* property, const T& name, Args... names) {
	if constexpr (sizeof...(Args) > 0) {
		return propertyByNames(property->getChild(name), names...);
	} else {
		return property->getChild(name);
	}
}

inline bool setLuaInputInEngine(ramses::Property* property, const core::ValueHandle& valueHandle) {
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
				success = property->set(ramses::vec2f{valueHandle[0].as<float>(), valueHandle[1].as<float>()});
			} else if (typeDesc == &core::Vec3f::typeDescription) {
				success = property->set(ramses::vec3f{valueHandle[0].as<float>(), valueHandle[1].as<float>(), valueHandle[2].as<float>()});
			} else if (typeDesc == &core::Vec4f::typeDescription) {
				success = property->set(ramses::vec4f{valueHandle[0].as<float>(), valueHandle[1].as<float>(), valueHandle[2].as<float>(), valueHandle[3].as<float>()});
			} else if (typeDesc == &core::Vec2i::typeDescription) {
				success = property->set(ramses::vec2i{valueHandle[0].as<int>(), valueHandle[1].as<int>()});
			} else if (typeDesc == &core::Vec3i::typeDescription) {
				success = property->set(ramses::vec3i{valueHandle[0].as<int>(), valueHandle[1].as<int>(), valueHandle[2].as<int>()});
			} else if (typeDesc == &core::Vec4i::typeDescription) {
				success = property->set(ramses::vec4i{valueHandle[0].as<int>(), valueHandle[1].as<int>(), valueHandle[2].as<int>(), valueHandle[3].as<int>()});
			}
			break;
		}
		case PrimitiveType::String:
			success = property->set(valueHandle.as<std::string>());
			break;
		case PrimitiveType::Table:
			success = true;
			for (size_t i{0}; i < valueHandle.size(); i++) {
				if (property->getType() == ramses::EPropertyType::Array) {
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

bool getOutputFromEngine(const ramses::Property& property, const core::ValueHandle& valueHandle, core::DataChangeRecorder& recorder);

inline bool getComplexLuaOutputFromEngine(const ramses::Property& property, const core::ValueHandle& valueHandle, core::DataChangeRecorder& recorder) {
	bool changed = false;
	for (size_t i{0}; i < valueHandle.size(); i++) {
		if (property.getType() == ramses::EPropertyType::Array) {
			changed = getOutputFromEngine(*property.getChild(i), valueHandle[i], recorder) || changed;
		} else {
			changed = getOutputFromEngine(*property.getChild(valueHandle[i].getPropName()), valueHandle[i], recorder) || changed;
		}
	}
	return changed;
}

inline bool getOutputFromEngine(const ramses::Property& property, const core::ValueHandle& valueHandle, core::DataChangeRecorder& recorder) {
	using core::PrimitiveType;

	// read quaternion rotation data
	if (valueHandle.isVec3f() && property.getType() == ramses::EPropertyType::Vec4f) {
		auto v = property.get<ramses::vec4f>().value();
		auto [eulerX, eulerY, eulerZ] = utils::math::quaternionToXYZDegrees(v.x, v.y, v.z, v.w);
		return core::CodeControlledPropertyModifier::setVec3f(valueHandle, eulerX, eulerY, eulerZ, recorder);
	}

	switch (valueHandle.type()) {
		case PrimitiveType::Double: {
			return core::CodeControlledPropertyModifier::setPrimitive<double>(valueHandle, property.get<float>().value(), recorder);
			break;
		}
		case PrimitiveType::Int: {
			return core::CodeControlledPropertyModifier::setPrimitive(valueHandle, property.get<int>().value(), recorder);
			break;
		}
		case PrimitiveType::Int64: {
			return core::CodeControlledPropertyModifier::setPrimitive(valueHandle, property.get<int64_t>().value(), recorder);
			break;
		}
		case PrimitiveType::Bool: {
			return core::CodeControlledPropertyModifier::setPrimitive(valueHandle, property.get<bool>().value(), recorder);
			break;
		}
		case PrimitiveType::String: {
			return core::CodeControlledPropertyModifier::setPrimitive(valueHandle, property.get<std::string>().value(), recorder);
			break;
		}
		case PrimitiveType::Struct: {
			auto typeDesc = &valueHandle.constValueRef()->asStruct().getTypeDescription();
			if (typeDesc == &core::Vec2f::typeDescription) {
				auto v = property.get<ramses::vec2f>().value();
				return core::CodeControlledPropertyModifier::setVec2f(valueHandle, v.x, v.y, recorder);
			} else if (typeDesc == &core::Vec3f::typeDescription) {
				auto v = property.get<ramses::vec3f>().value();
				return core::CodeControlledPropertyModifier::setVec3f(valueHandle, v.x, v.y, v.z, recorder);
			} else if (typeDesc == &core::Vec4f::typeDescription) {
				auto v = property.get<ramses::vec4f>().value();
				return core::CodeControlledPropertyModifier::setVec4f(valueHandle, v.x, v.y, v.z, v.w, recorder);
			} else if (typeDesc == &core::Vec2i::typeDescription) {
				auto v = property.get<ramses::vec2i>().value();
				return core::CodeControlledPropertyModifier::setVec2i(valueHandle, v.x, v.y, recorder);
			} else if (typeDesc == &core::Vec3i::typeDescription) {
				auto v = property.get<ramses::vec3i>().value();
				return core::CodeControlledPropertyModifier::setVec3i(valueHandle, v.x, v.y, v.z, recorder);
			} else if (typeDesc == &core::Vec4i::typeDescription) {
				auto v = property.get<ramses::vec4i>().value();
				return core::CodeControlledPropertyModifier::setVec4i(valueHandle, v.x, v.y, v.z, v.w, recorder);
			} else {
				return getComplexLuaOutputFromEngine(property, valueHandle, recorder);
			}
			break;
		}
		case PrimitiveType::Table: {
			return getComplexLuaOutputFromEngine(property, valueHandle, recorder);
			break;
		}
	}
	return false;
}

inline ramses::EDepthWrite getDepthWriteMode(const ramses::Appearance* appearance) {
	ramses::EDepthWrite depthWrite;
	appearance->getDepthWriteMode(depthWrite);
	return depthWrite;
}

template <typename UniformType>
void ramsesSetUniform(ramses::Appearance& appearance, std::string_view uniformName, UniformType value) {
	ramses::UniformInput uniform = appearance.getEffect().findUniformInput(uniformName).value();
	appearance.setInputValue(uniform, value);
}

inline bool isArrayOfStructs(const ramses::Property& property) {
	return property.getType() == ramses::EPropertyType::Array && property.getChildCount() > 0 && property.getChild(0)->getType() == ramses::EPropertyType::Struct;
}

ramses_base::RamsesNodeBinding lookupNodeBinding(const SceneAdaptor* sceneAdaptor, core::SEditorObject node);

struct DependencyNode {
	core::SEditorObject object;
	core::SEditorObjectSet referencedObjects;
};

std::vector<DependencyNode> buildSortedDependencyGraph(core::SEditorObjectSet const& objects);

ramses_base::RamsesArrayResource arrayResourceFromAttribute(ramses::Scene* scene, core::SharedMeshData mesh, int attribIndex, std::string_view name);

};	// namespace raco::ramses_adaptor
