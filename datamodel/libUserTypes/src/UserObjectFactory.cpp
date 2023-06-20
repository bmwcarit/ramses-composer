/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "user_types/UserObjectFactory.h"

#include "core/ExternalReferenceAnnotation.h"
#include "core/ProjectSettings.h"

#include "user_types/AnchorPoint.h"
#include "user_types/Animation.h"
#include "user_types/AnimationChannel.h"
#include "user_types/BaseCamera.h"
#include "user_types/BaseObject.h"
#include "user_types/BaseTexture.h"
#include "user_types/BlitPass.h"
#include "user_types/CubeMap.h"
#include "user_types/LuaInterface.h"
#include "user_types/LuaScript.h"
#include "user_types/LuaScriptModule.h"
#include "user_types/Material.h"
#include "user_types/Mesh.h"
#include "user_types/MeshNode.h"
#include "user_types/Node.h"
#include "user_types/OrthographicCamera.h"
#include "user_types/PerspectiveCamera.h"
#include "user_types/Prefab.h"
#include "user_types/PrefabInstance.h"
#include "user_types/RenderBuffer.h"
#include "user_types/RenderBufferMS.h"
#include "user_types/RenderLayer.h"
#include "user_types/RenderTarget.h"
#include "user_types/RenderPass.h"
#include "user_types/Skin.h"
#include "user_types/Texture.h"
#include "user_types/TextureExternal.h"
#include "user_types/Timer.h"

namespace raco::user_types {

template <class T>
std::shared_ptr<AnnotationBase> UserObjectFactory::createAnnotationInternal() {
	return std::make_shared<T>();
}

template <class T>
std::shared_ptr<ClassWithReflectedMembers> UserObjectFactory::createStructInternal() {
	return std::make_shared<T>();
}

template <class T>
data_storage::ValueBase* UserObjectFactory::createStructValueInternal() {
	return new Value<T>();
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
std::map<std::string, raco::core::UserObjectFactoryInterface::ValueCreationFunction> UserObjectFactory::makePropertyMapTuple(std::tuple<Args...>* dummy) {	
	return std::map<std::string, raco::core::UserObjectFactoryInterface::ValueCreationFunction>{ createTypeMapPair<Args>()...};
}

template <class... Args>
std::map<std::string, UserObjectFactory::AnnotationDescriptor> UserObjectFactory::makeAnnotationMap() {
	return std::map<std::string, UserObjectFactory::AnnotationDescriptor>{
		{Args::typeDescription.typeName, {Args::typeDescription, createAnnotationInternal<Args>}}...};
}

template <class... Args>
std::map<std::string, UserObjectFactory::StructDescriptor> UserObjectFactory::makeStructTypeMap() {
	return std::map<std::string, UserObjectFactory::StructDescriptor>{
		{Args::typeDescription.typeName, {Args::typeDescription, createStructInternal<Args>, createStructValueInternal<Args>}}...};
}

UserObjectFactory::UserObjectFactory() {
	properties_ = makePropertyMapTuple(static_cast<PropertyTypeMapType*>(nullptr));

	types_ = makeTypeMap<
		ProjectSettings,
		AnchorPoint,
		Animation,
		AnimationChannel,
		BlitPass,
		CubeMap,
		Node,
		MeshNode,
		Mesh,
		Material,
		Prefab,
		PrefabInstance,
		OrthographicCamera,
		PerspectiveCamera,
		LuaInterface,
		LuaScript,
		LuaScriptModule,
		Texture,
		TextureExternal,
		Timer,
		RenderBuffer,
		RenderBufferMS,
		RenderLayer,
		RenderTarget,
		RenderPass,
		Skin
		>();

	annotations_ = makeAnnotationMap<
		ExternalReferenceAnnotation
	>();

	structTypes_ = makeStructTypeMap<
		raco::core::Vec2f,
		raco::core::Vec3f,
		raco::core::Vec4f,
		raco::core::Vec2i,
		raco::core::Vec3i,
		raco::core::Vec4i,
		ColorWriteMask,
		BlendOptions,
		DefaultResourceDirectories,
		CameraViewport,
		OrthographicFrustum,
		LuaStandardModuleSelection,
		ScissorOptions,
		StencilOptions,
		TimerInput,
		TimerOutput,
		AnchorPointOutputs>();
}

UserObjectFactory& UserObjectFactory::getInstance() {
	static UserObjectFactory* instance = nullptr;
	if (!instance) {
		instance = new UserObjectFactory();
	}
	return *instance;
}

SEditorObject UserObjectFactory::createObject(const std::string& type, const std::string& name, const std::string& id) const {
	auto it = types_.find(type);
	if (it != types_.end()) {
		return it->second.createFunc(name, id);
	}

	return SEditorObject();
}

std::shared_ptr<AnnotationBase> UserObjectFactory::createAnnotation(const std::string& type) const {
	auto it = annotations_.find(type);
	if (it != annotations_.end()) {
		return it->second.createFunc();
	}
	return nullptr;
}

std::shared_ptr<ClassWithReflectedMembers> UserObjectFactory::createStruct(const std::string& type) const {
	auto it = structTypes_.find(type);
	if (it != structTypes_.end()) {
		return it->second.createFunc();
	}
	return {};
}

data_storage::ValueBase* UserObjectFactory::createValue(const std::string& type) const {
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
	if (auto it = structTypes_.find(type); it != structTypes_.end()) {
		return it->second.createValueFunc();
	}

	return new Value<SEditorObject>();
}

const std::map<std::string, UserObjectFactory::TypeDescriptor>& UserObjectFactory::getTypes() const {
	return types_;
}

bool UserObjectFactory::isUserCreatable(const std::string& type, int featureLevel) const {
	auto it = types_.find(type);
	return it != types_.end() && type != core::ProjectSettings::typeDescription.typeName && it->second.description.featureLevel <= featureLevel;
}

const std::map<std::string, UserObjectFactory::StructDescriptor>& UserObjectFactory::getStructTypes() const {
	return structTypes_;
}

const std::map<std::string, UserObjectFactory::ValueCreationFunction>& UserObjectFactory::getProperties() const {
	return properties_;
}

}  // namespace raco::user_types