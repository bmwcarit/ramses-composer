/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "user_types/UserObjectFactory.h"

#include "core/ExternalReferenceAnnotation.h"

#include "user_types/BaseCamera.h"
#include "user_types/BaseObject.h"
#include "user_types/BaseTexture.h"
#include "user_types/CubeMap.h"
#include "user_types/LuaScript.h"
#include "user_types/Material.h"
#include "user_types/Mesh.h"
#include "user_types/MeshNode.h"
#include "user_types/Node.h"
#include "user_types/OrthographicCamera.h"
#include "user_types/PerspectiveCamera.h"
#include "user_types/Prefab.h"
#include "user_types/PrefabInstance.h"
#include "user_types/Texture.h"

namespace raco::user_types {

template <class T>
SEditorObject UserObjectFactory::createObjectInternal(const std::string& name, const std::string& id) {
	return std::make_shared<T>(name, id);
}

template <class T>
std::shared_ptr<AnnotationBase> UserObjectFactory::createAnnotationInternal() {
	return std::make_shared<T>();
}

template <class T>
data_storage::ValueBase* UserObjectFactory::createValueInternal() {
	return new Value<std::shared_ptr<T>>();
}

template <class... Args>
std::map<std::string, UserObjectFactory::TypeDescriptor> UserObjectFactory::makeTypeMap() {
	return std::map<std::string, UserObjectFactory::TypeDescriptor>{
		{Args::typeDescription.typeName, {Args::typeDescription, createObjectInternal<Args>, createValueInternal<Args>}}...};
}

template <class TPropertyType>
constexpr std::pair<std::string, std::function<TPropertyType*()>> createTypeMapPair() {
	return { TPropertyType().typeName(), []() { return new TPropertyType(); } };
}

template <class... Args>
std::map<std::string, raco::core::UserObjectFactoryInterface::ValueCreationFunction> makePropertyMapTuple(std::tuple<Args...> *dummy) {	
	return std::map<std::string, raco::core::UserObjectFactoryInterface::ValueCreationFunction>{ createTypeMapPair<Args>()...};
}

template <class... Args>
std::map<std::string, UserObjectFactory::AnnotationDescriptor> UserObjectFactory::makeAnnotationMap() {
	return std::map<std::string, UserObjectFactory::AnnotationDescriptor>{
		{Args::typeDescription.typeName, {Args::typeDescription, createAnnotationInternal<Args>}}...};
}


UserObjectFactory::UserObjectFactory() {
	properties_ = makePropertyMapTuple(static_cast<PropertyTypeMapType*>(nullptr));

	types_ = makeTypeMap<
		ProjectSettings,
        CubeMap,
		Node,
		MeshNode,
		Mesh,
		Material,
		Prefab,
		PrefabInstance,
		OrthographicCamera,
		PerspectiveCamera,
		LuaScript,
        Texture
		>();

	annotations_ = makeAnnotationMap<
		ExternalReferenceAnnotation
	>();
}

UserObjectFactory& UserObjectFactory::getInstance() {
	static UserObjectFactory* instance = nullptr;
	if (!instance) {
		instance = new UserObjectFactory();
	}
	return *instance;
}

SEditorObject UserObjectFactory::createObject(const std::string& type, const std::string& name, const std::string& id) {
	auto it = types_.find(type);
	if (it != types_.end()) {
		return it->second.createFunc(name, id);
	}

	return SEditorObject();
}

std::shared_ptr<AnnotationBase> UserObjectFactory::createAnnotation(const std::string& type) {
	auto it = annotations_.find(type);
	if (it != annotations_.end()) {
		return it->second.createFunc();
	}
	return nullptr;
}

data_storage::ValueBase* UserObjectFactory::createValue(const std::string& type) {
	{
		auto it = types_.find(type);
		if (it != types_.end()) {
			return it->second.createValueFunc();
		}
	}
	{
		auto it = properties_.find(type);
		if (it != properties_.end()) {
			return it->second();
		}
	}
	return new Value<SEditorObject>();
}

const std::map<std::string, UserObjectFactory::TypeDescriptor>& UserObjectFactory::getTypes() const {
	return types_;
}

bool UserObjectFactory::isUserCreatable(const std::string& type) const {
	return type != core::ProjectSettings::typeDescription.typeName;
}

}  // namespace raco::user_types