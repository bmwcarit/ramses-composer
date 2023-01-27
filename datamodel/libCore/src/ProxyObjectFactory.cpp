/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "core/ProxyObjectFactory.h"

#include "core/ProxyTypes.h"
#include "core/ExternalReferenceAnnotation.h"

#include <spdlog/fmt/fmt.h>

namespace raco::serialization::proxy {

	template <class T>
	SEditorObject ProxyObjectFactory::createObjectInternal(const std::string& name, const std::string& id) {
		return std::make_shared<T>(name, id);
	}

	template <class T>
	std::shared_ptr<AnnotationBase> ProxyObjectFactory::createAnnotationInternal() {
		return std::make_shared<T>();
	}

	template <class T>
	data_storage::ValueBase* ProxyObjectFactory::createValueInternal() {
		return new Value<std::shared_ptr<T>>();
	}

	template <class... Args>
	std::map<std::string, ProxyObjectFactory::TypeDescriptor> ProxyObjectFactory::makeTypeMap() {
		return std::map<std::string, ProxyObjectFactory::TypeDescriptor>{
			{Args::typeDescription.typeName, { Args::typeDescription, createObjectInternal<Args>, createValueInternal<Args> }}...};
	}

	template <class TPropertyType>
	constexpr std::pair<std::string, std::function<TPropertyType* ()>> createTypeMapPair() {
		return { TPropertyType().typeName(), []() { return new TPropertyType(); } };
	}

	template <class... Args>
	std::map<std::string, raco::core::UserObjectFactoryInterface::ValueCreationFunction> ProxyObjectFactory::makePropertyMapTuple(std::tuple<Args...>* dummy) {
		return std::map<std::string, raco::core::UserObjectFactoryInterface::ValueCreationFunction>{ createTypeMapPair<Args>()...};
	}

	template <class... Args>
	std::map<std::string, ProxyObjectFactory::AnnotationDescriptor> ProxyObjectFactory::makeAnnotationMap() {
		return std::map<std::string, ProxyObjectFactory::AnnotationDescriptor>{
			{Args::typeDescription.typeName, { Args::typeDescription, createAnnotationInternal<Args> }}...};
	}


	ProxyObjectFactory::ProxyObjectFactory() {
		properties_ = makePropertyMapTuple(static_cast<PropertyTypeMapType*>(nullptr));

		// This contains proxy types defined in ProxyTypes.h from namespace raco::serialization::proxy
		// Don't add the normal user_types here.
		// Instead create a new proxy type in ProxyTypes.h and add that in the call below.
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
			LuaScript,
			LuaInterface,
			LuaScriptModule,
			Texture,
			TextureExternal,
			RenderBuffer,
			RenderBufferMS,
			RenderLayer,
			RenderTarget,
			RenderPass,
			Skin,
			Timer
		>();

		annotations_ = makeAnnotationMap<
			raco::core::ExternalReferenceAnnotation
		>();
	}

	ProxyObjectFactory& ProxyObjectFactory::getInstance() {
		static ProxyObjectFactory* instance = nullptr;
		if (!instance) {
			instance = new ProxyObjectFactory();
		}
		return *instance;
	}

	SEditorObject ProxyObjectFactory::createObject(const std::string& type, const std::string& name, const std::string& id) const {
		auto it = types_.find(type);
		if (it != types_.end()) {
			return it->second.createFunc(name, id);
		}
		throw std::runtime_error(fmt::format("ProxyObjectFactory can't create object of unknown type '{}'", type));
		return SEditorObject();
	}

	std::shared_ptr<AnnotationBase> ProxyObjectFactory::createAnnotation(const std::string& type) const {
		auto it = annotations_.find(type);
		if (it != annotations_.end()) {
			return it->second.createFunc();
		}
		throw std::runtime_error(fmt::format("ProxyObjectFactory can't create annotation of unknown type '{}'", type));
		return nullptr;
	}

	data_storage::ValueBase* ProxyObjectFactory::createValue(const std::string& type) const {
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
		throw std::runtime_error(fmt::format("ProxyObjectFactory can't create value of unknown type '{}'", type));
		return {};
	}

	const std::map<std::string, ProxyObjectFactory::TypeDescriptor>& ProxyObjectFactory::getTypes() const {
		return types_;
	}

	bool ProxyObjectFactory::isUserCreatable(const std::string& type, int featureLevel) const {
		return true;
	}

	const std::map<std::string, ProxyObjectFactory::ValueCreationFunction>& ProxyObjectFactory::getProperties() const {
		return properties_;
	}

	}  // namespace raco::user_types