/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
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
#include "user_types/TextureExternal.h"
#include "user_types/EngineTypeAnnotation.h"
#include "user_types/UserObjectFactory.h"
#include "user_types/RenderBufferMS.h"

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
		case EnginePrimitive::Int64:
			return UserObjectFactory::staticCreateProperty<int64_t, EngineTypeAnnotation, Args...>({}, {type}, {Args()}...);
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
			return UserObjectFactory::staticCreateProperty<STextureSampler2DBase, EngineTypeAnnotation, Args...>({}, {type}, {Args()}...);
			break;

		case EnginePrimitive::TextureSampler2DMS:
			return UserObjectFactory::staticCreateProperty<SRenderBufferMS, EngineTypeAnnotation, Args...>({}, {type}, {Args()}...);
			break;

		case EnginePrimitive::TextureSamplerCube:
			return UserObjectFactory::staticCreateProperty<SCubeMap, EngineTypeAnnotation, Args...>({}, {type}, {Args()}...);
			break;
		case EnginePrimitive::TextureSamplerExternal:
			return UserObjectFactory::staticCreateProperty<STextureExternal, EngineTypeAnnotation, Args...>({}, {type}, {Args()}...);
			break;
	}
	return nullptr;
}

std::string dataModelPropNameForLogicEnginePropName(const std::string& propName, size_t index) {
	if (propName.empty()) {
		return std::to_string(index + 1);
	}
	return propName;
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
	return dataModelPropNameForLogicEnginePropName(propInterface.name, index);
}

std::vector<PropertyInterface>::const_iterator findNameInInterfaces(const PropertyInterfaceList& interfaces, const std::string& name) {
	for (size_t index{0}; index < interfaces.size(); index++) {
		if (dataModelNameFromInterface(interfaces[index], index) == name) {
			return std::next(interfaces.begin(), index);
		}
	}
	return interfaces.end();
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
	for (size_t propertyIndex{0}; propertyIndex < property.size(); propertyIndex++) {
		const auto name = property[propertyIndex].getPropName();
		auto it = findNameInInterfaces(interface, name);
		const ValueBase* value = property[propertyIndex].constValueRef();
		auto anno = value->query<EngineTypeAnnotation>();
		EnginePrimitive engineType = anno->type();

		auto fullPropPath = propertyPath + propertyPathSeparator + name;
		if (it == interface.end() || it->type != engineType || std::distance(interface.begin(), it) != propertyIndex) {
			toRemove.emplace_back(name);
			if (value->type() != PrimitiveType::Table) {
				outdatedPropertiesStore[std::make_pair(fullPropPath, engineType)] = value->clone(nullptr);
			} else {
				cacheRecursive(context, property[propertyIndex], fullPropPath, outdatedPropertiesStore);
			}
		} else if (value->type() == PrimitiveType::Table) {
			removeProperties(context, it->children, property[propertyIndex], fullPropPath, outdatedPropertiesStore);
		}
	}
	for (const auto& propName : toRemove) {
		context.removeProperty(property, propName);
	}
}

inline void addProperties(raco::core::BaseContext& context, const PropertyInterfaceList& interface, const ValueHandle& property, const std::string& propertyPath, const OutdatedPropertiesStore& outdatedPropertiesStore, bool linkStart, bool linkEnd, std::function<const ValueBase*(const std::string& fullPropPath, raco::core::EnginePrimitive engineType)> cacheLookupFunc) {
	for (size_t interfaceIndex{0}; interfaceIndex < interface.size(); interfaceIndex++) {
		const auto& iEntry = interface[interfaceIndex];
		const std::string name(dataModelNameFromInterface(iEntry, interfaceIndex));
		if (!property.hasProperty(name)) {
			std::unique_ptr<raco::data_storage::ValueBase> uniqueValue;
			bool isRefType = PropertyInterface::primitiveType(iEntry.type) == PrimitiveType::Ref;
			if (linkStart && linkEnd && !isRefType) {
				uniqueValue = std::unique_ptr<raco::data_storage::ValueBase>(createDynamicProperty<raco::core::LinkStartAnnotation, raco::core::LinkEndAnnotation>(iEntry.type));
			} else if (linkStart && !isRefType) {
				uniqueValue = std::unique_ptr<raco::data_storage::ValueBase>(createDynamicProperty<raco::core::LinkStartAnnotation>(iEntry.type));
			} else if (linkEnd && !isRefType) {
				uniqueValue = std::unique_ptr<raco::data_storage::ValueBase>(createDynamicProperty<raco::core::LinkEndAnnotation>(iEntry.type));
			} else {
				uniqueValue = std::unique_ptr<raco::data_storage::ValueBase>(createDynamicProperty<>(iEntry.type));
			}
			ValueBase* newValue = context.addProperty(property, name, std::move(uniqueValue), interfaceIndex);
			auto fullPropPath = propertyPath + propertyPathSeparator + name;

			if (iEntry.primitiveType() != PrimitiveType::Table) {
				const ValueBase* cachedValue = cacheLookupFunc(fullPropPath, iEntry.type);
				if (cachedValue) {
					if (iEntry.primitiveType() == PrimitiveType::Ref) {
						// Special case for references: perform lookup in the project by object id
						// Needed because the object might have been deleted in the meantime and we don't
						// want to set a pointer to an invalid object here.
						SEditorObject cachedObject = nullptr;
						if (cachedValue->asRef()) {
							cachedObject = context.project()->getInstanceByID(cachedValue->asRef()->objectID());
						}
						// Use the context to set reference properties to make sure the onAfterAddReferenceToThis handlers are called.
						context.set(property.get(name), cachedObject);
					} else {
						*newValue = *cachedValue;
					}
				}
			}
		}
		if (iEntry.primitiveType() == PrimitiveType::Table) {
			addProperties(context, iEntry.children, property.get(name), propertyPath + propertyPathSeparator + name, outdatedPropertiesStore, linkStart, linkEnd, cacheLookupFunc);
		}
	}
}
}  // namespace

void syncTableWithEngineInterface(raco::core::BaseContext& context, const PropertyInterfaceList& interface, const ValueHandle& handle, OutdatedPropertiesStore& outdatedPropertiesStore, bool linkStart, bool linkEnd, std::function<const ValueBase*(const std::string& fullPropPath, raco::core::EnginePrimitive engineType)> cacheLookupFunc) {
	removeProperties(context, interface, handle, "", outdatedPropertiesStore);
	addProperties(context, interface, handle, "", outdatedPropertiesStore, linkStart, linkEnd, cacheLookupFunc);
}

void syncTableWithEngineInterface(raco::core::BaseContext& context, const PropertyInterfaceList& interface, const ValueHandle& handle, OutdatedPropertiesStore& outdatedPropertiesStore, bool linkStart, bool linkEnd) {
	syncTableWithEngineInterface(context, interface, handle, outdatedPropertiesStore, linkStart, linkEnd, [&outdatedPropertiesStore](const std::string& fullPropPath, raco::core::EnginePrimitive engineType) -> const ValueBase* {
		auto it = outdatedPropertiesStore.find(std::make_pair(fullPropPath, engineType));
		if (it != outdatedPropertiesStore.end()) {
			return it->second.get();
		}
		return nullptr;
	});
}


}  // namespace raco::user_types