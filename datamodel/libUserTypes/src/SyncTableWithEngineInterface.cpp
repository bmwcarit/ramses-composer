/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "user_types/SyncTableWithEngineInterface.h"

#include "core/Project.h"
#include "data_storage/Value.h"
#include <algorithm>
#include "core/Link.h"

#include "user_types/CubeMap.h"
#include "user_types/Texture.h"
#include "user_types/EngineTypeAnnotation.h"
#include "user_types/UserObjectFactory.h"

namespace raco::user_types {

using raco::core::PropertyInterface;
using raco::core::PropertyInterfaceList;
using raco::core::ValueHandle;
using raco::data_storage::PrimitiveType;

template <class... Args>
raco::data_storage::ValueBase* createDynamicProperty(EnginePrimitive type) {
	switch (type) {
		case EnginePrimitive::Bool:
			return UserObjectFactory::staticCreateProperty<bool, EngineTypeAnnotation, Args...>({}, {type}, {Args()}...);
			break;
		case EnginePrimitive::Int32:
		case EnginePrimitive::UInt16:
		case EnginePrimitive::UInt32:
			return UserObjectFactory::staticCreateProperty<int, EngineTypeAnnotation, Args...>({}, {type}, {Args()}...);
			break;
		case EnginePrimitive::Double:
			return UserObjectFactory::staticCreateProperty<double, EngineTypeAnnotation, Args...>({}, {type}, {Args()}...);
			break;
		case EnginePrimitive::String:
			return UserObjectFactory::staticCreateProperty<std::string, EngineTypeAnnotation, Args...>({}, {type}, {Args()}...);
			break;

		case EnginePrimitive::Vec2f:
			return UserObjectFactory::staticCreateProperty<Vec2f, EngineTypeAnnotation, Args...>({}, {type}, {Args()}...);
			break;
		case EnginePrimitive::Vec3f:
			return UserObjectFactory::staticCreateProperty<Vec3f, EngineTypeAnnotation, Args...>({}, {type}, {Args()}...);
			break;
		case EnginePrimitive::Vec4f:
			return UserObjectFactory::staticCreateProperty<Vec4f, EngineTypeAnnotation, Args...>({}, {type}, {Args()}...);
			break;

		case EnginePrimitive::Vec2i:
			return UserObjectFactory::staticCreateProperty<Vec2i, EngineTypeAnnotation, Args...>({}, {type}, {Args()}...);
			break;
		case EnginePrimitive::Vec3i:
			return UserObjectFactory::staticCreateProperty<Vec3i, EngineTypeAnnotation, Args...>({}, {type}, {Args()}...);
			break;
		case EnginePrimitive::Vec4i:
			return UserObjectFactory::staticCreateProperty<Vec4i, EngineTypeAnnotation, Args...>({}, {type}, {Args()}...);
			break;

		case EnginePrimitive::Array:
		case EnginePrimitive::Struct:
			return UserObjectFactory::staticCreateProperty<Table, EngineTypeAnnotation, Args...>({}, {type}, {Args()}...);
			break;

		case EnginePrimitive::TextureSampler2D:
			return UserObjectFactory::staticCreateProperty<STexture, EngineTypeAnnotation, Args...>({}, {type}, {Args()}...);
			break;

		case EnginePrimitive::TextureSamplerCube:
			return UserObjectFactory::staticCreateProperty<SCubeMap, EngineTypeAnnotation, Args...>({}, {type}, {Args()}...);
			break;
	}
	return nullptr;
}

namespace {
inline size_t index_of(const ValueHandle& handle, const std::string& name) {
	for (size_t i{0}; i < handle.size(); i++) {
		if (handle[i].getPropName() == name)
			return i;
	}
	throw std::runtime_error("childIndex not found");
}

static const char propertyPathSeparator = '/';

std::string dataModelNameFromInterface(const PropertyInterface& propInterface, size_t index) {
	if (propInterface.name.empty()) {
		return std::to_string(index + 1);
	}
	return propInterface.name;
}

const PropertyInterface* findNameInInterfaces(const PropertyInterfaceList& interfaces, const std::string& name) {
	const PropertyInterface* it = nullptr;
	for (size_t index{0}; index < interfaces.size(); index++) {
		if (dataModelNameFromInterface(interfaces[index], index) == name) {
			it = &interfaces[index];
			break;
		}
	}
	return it;
}

inline void cacheRecursive(raco::core::BaseContext& context, const ValueHandle& property, const std::string& propertyPath, OutdatedPropertiesStore& outdatedPropertiesStore) {
	for (size_t i{0}; i < property.size(); i++) {
		const auto name = property[i].getPropName();
		const ValueBase* value = property[i].constValueRef();
		auto anno = value->query<EngineTypeAnnotation>();
		EnginePrimitive engineType = anno->type();

		auto fullPropPath = propertyPath + propertyPathSeparator + name;
		if (value->type() != PrimitiveType::Table) {
			outdatedPropertiesStore[std::make_pair(fullPropPath, engineType)] = value->clone(nullptr);
		} else {
			cacheRecursive(context, property[i], fullPropPath, outdatedPropertiesStore);
		}
	}
}

inline void removeProperties(raco::core::BaseContext& context, const PropertyInterfaceList& interface, const ValueHandle& property, const std::string& propertyPath, OutdatedPropertiesStore& outdatedPropertiesStore) {
	std::vector<std::string> toRemove{};
	for (size_t i{0}; i < property.size(); i++) {
		const auto name = property[i].getPropName();
		const PropertyInterface* it = findNameInInterfaces(interface, name);
		const ValueBase* value = property[i].constValueRef();
		auto anno = value->query<EngineTypeAnnotation>();
		EnginePrimitive engineType = anno->type();

		auto fullPropPath = propertyPath + propertyPathSeparator + name;
		if (it == nullptr || it->type != engineType) {
			toRemove.emplace_back(name);
			if (value->type() != PrimitiveType::Table) {
				outdatedPropertiesStore[std::make_pair(fullPropPath, engineType)] = value->clone(nullptr);
			} else {
				cacheRecursive(context, property[i], fullPropPath, outdatedPropertiesStore);
			}
		} else if (value->type() == PrimitiveType::Table) {
			removeProperties(context, it->children, property[i], fullPropPath, outdatedPropertiesStore);
		}
	}
	for (const auto& propName : toRemove) {
		context.removeProperty(property, propName);
	}
}

inline void addProperties(raco::core::BaseContext& context, const PropertyInterfaceList& interface, const ValueHandle& property, const std::string& propertyPath, const OutdatedPropertiesStore& outdatedPropertiesStore, bool linkStart, bool linkEnd) {
	for (size_t index{0}; index < interface.size(); index++) {
		const auto& iEntry = interface[index];
		const std::string name(dataModelNameFromInterface(iEntry, index));
		if (!property.hasProperty(name)) {
			std::unique_ptr<raco::data_storage::ValueBase> uniqueValue;
			if (linkStart) {
				uniqueValue = std::unique_ptr<raco::data_storage::ValueBase>(createDynamicProperty<raco::core::LinkStartAnnotation>(iEntry.type));
			} else if (linkEnd) {
				uniqueValue = std::unique_ptr<raco::data_storage::ValueBase>(createDynamicProperty<raco::core::LinkEndAnnotation>(iEntry.type));
			} else {
				uniqueValue = std::unique_ptr<raco::data_storage::ValueBase>(createDynamicProperty<>(iEntry.type));
			}
			ValueBase* newValue = context.addProperty(property, name, std::move(uniqueValue));
			auto fullPropPath = propertyPath + propertyPathSeparator + name;

			auto it = outdatedPropertiesStore.find(std::make_pair(fullPropPath, iEntry.type));

			if (it != outdatedPropertiesStore.end()) {
				raco::data_storage::ValueBase* cachedValue = it->second.get();
				if (iEntry.primitiveType() == PrimitiveType::Ref) {
					// Special case for references: perform lookup in the project by object id
					// Needed because the object might have been deleted in the meantime and we don't
					// want to set a pointer to an invalid object here.
					SEditorObject cachedObject = nullptr;
					if (cachedValue->asRef()) {
						cachedObject = context.project()->getInstanceByID(cachedValue->asRef()->objectID());
					}
					*newValue = cachedObject;
				} else {
					*newValue = *cachedValue;
				}
			}
		}
		if (iEntry.primitiveType() == PrimitiveType::Table) {
			addProperties(context, iEntry.children, property.get(name), propertyPath + propertyPathSeparator + name, outdatedPropertiesStore, linkStart, linkEnd);
		}
	}
}
}  // namespace

void syncTableWithEngineInterface(raco::core::BaseContext& context, const PropertyInterfaceList& interface, const ValueHandle& handle, OutdatedPropertiesStore& outdatedPropertiesStore, bool linkStart, bool linkEnd) {
	removeProperties(context, interface, handle, "", outdatedPropertiesStore);
	addProperties(context, interface, handle, "", outdatedPropertiesStore, linkStart, linkEnd);
}

}  // namespace raco::user_types