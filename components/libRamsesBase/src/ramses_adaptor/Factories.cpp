/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "ramses_adaptor/Factories.h"

#include "ramses_adaptor/AnimationAdaptor.h"
#include "ramses_adaptor/AnimationChannelAdaptor.h"
#include "ramses_adaptor/CubeMapAdaptor.h"
#include "ramses_adaptor/LuaScriptAdaptor.h"
#include "ramses_adaptor/LuaInterfaceAdaptor.h"
#include "ramses_adaptor/LuaScriptModuleAdaptor.h"
#include "ramses_adaptor/MaterialAdaptor.h"
#include "ramses_adaptor/MeshAdaptor.h"
#include "ramses_adaptor/MeshNodeAdaptor.h"
#include "ramses_adaptor/NodeAdaptor.h"
#include "ramses_adaptor/OrthographicCameraAdaptor.h"
#include "ramses_adaptor/PerspectiveCameraAdaptor.h"
#include "ramses_adaptor/RenderBufferAdaptor.h"
#include "ramses_adaptor/RenderTargetAdaptor.h"
#include "ramses_adaptor/RenderPassAdaptor.h"
#include "ramses_adaptor/RenderLayerAdaptor.h"
#include "ramses_adaptor/SceneAdaptor.h"
#include "ramses_adaptor/TimerAdaptor.h"
#include "ramses_adaptor/TextureSamplerAdaptor.h"
#include "user_types/CubeMap.h"
#include "user_types/Texture.h"
#include "user_types/PrefabInstance.h"
#include <functional>
#include <map>
#include <memory>

namespace raco::ramses_adaptor {

using Factory = std::function<UniqueObjectAdaptor(SceneAdaptor* sceneAdaptor, core::SEditorObject)>;

UniqueObjectAdaptor Factories::createAdaptor(SceneAdaptor* sceneAdaptor, core::SEditorObject obj) {
	static const std::map<std::string, Factory> factoryByTypename{
		// SCENE OBJECTS
		{user_types::Node::typeDescription.typeName, [](SceneAdaptor* sceneAdaptor, core::SEditorObject obj) { return std::make_unique<NodeAdaptor>(sceneAdaptor, std::dynamic_pointer_cast<user_types::Node>(obj)); }},
		{user_types::MeshNode::typeDescription.typeName, [](SceneAdaptor* sceneAdaptor, core::SEditorObject obj) { return std::make_unique<MeshNodeAdaptor>(sceneAdaptor, std::dynamic_pointer_cast<user_types::MeshNode>(obj)); }},

		{user_types::OrthographicCamera::typeDescription.typeName, [](SceneAdaptor* sceneAdaptor, core::SEditorObject obj) { return std::make_unique<OrthographicCameraAdaptor>(sceneAdaptor, std::dynamic_pointer_cast<user_types::OrthographicCamera>(obj)); }},
		{user_types::PerspectiveCamera::typeDescription.typeName, [](SceneAdaptor* sceneAdaptor, core::SEditorObject obj) { return std::make_unique<PerspectiveCameraAdaptor>(sceneAdaptor, std::dynamic_pointer_cast<user_types::PerspectiveCamera>(obj)); }},

		{user_types::PrefabInstance::typeDescription.typeName, [](SceneAdaptor* sceneAdaptor, core::SEditorObject obj) { return std::make_unique<NodeAdaptor>(sceneAdaptor, std::dynamic_pointer_cast<user_types::Node>(obj)); }},


		// RESOURCES
		{user_types::Animation::typeDescription.typeName, [](SceneAdaptor* sceneAdaptor, core::SEditorObject obj) { return std::make_unique<AnimationAdaptor>(sceneAdaptor, std::dynamic_pointer_cast<user_types::Animation>(obj)); }},
		{user_types::AnimationChannel::typeDescription.typeName, [](SceneAdaptor* sceneAdaptor, core::SEditorObject obj) { return std::make_unique<AnimationChannelAdaptor>(sceneAdaptor, std::dynamic_pointer_cast<user_types::AnimationChannel>(obj)); }},
		{user_types::Material::typeDescription.typeName, [](SceneAdaptor* sceneAdaptor, core::SEditorObject obj) { return std::make_unique<MaterialAdaptor>(sceneAdaptor, std::dynamic_pointer_cast<user_types::Material>(obj)); }},
		{user_types::Mesh::typeDescription.typeName, [](SceneAdaptor* sceneAdaptor, core::SEditorObject obj) { return std::make_unique<MeshAdaptor>(sceneAdaptor, std::dynamic_pointer_cast<user_types::Mesh>(obj)); }},
		{user_types::Texture::typeDescription.typeName, [](SceneAdaptor* sceneAdaptor, core::SEditorObject obj) { return std::make_unique<TextureSamplerAdaptor>(sceneAdaptor, std::dynamic_pointer_cast<user_types::Texture>(obj)); }},
		{user_types::CubeMap::typeDescription.typeName, [](SceneAdaptor* sceneAdaptor, core::SEditorObject obj) { return std::make_unique<CubeMapAdaptor>(sceneAdaptor, std::dynamic_pointer_cast<user_types::CubeMap>(obj)); }},
		{user_types::LuaScriptModule::typeDescription.typeName, [](SceneAdaptor* sceneAdaptor, core::SEditorObject obj) { return std::make_unique<LuaScriptModuleAdaptor>(sceneAdaptor, std::dynamic_pointer_cast<user_types::LuaScriptModule>(obj)); }},

		{user_types::RenderBuffer::typeDescription.typeName, [](SceneAdaptor* sceneAdaptor, core::SEditorObject obj) { return std::make_unique<RenderBufferAdaptor>(sceneAdaptor, std::dynamic_pointer_cast<user_types::RenderBuffer>(obj)); }},
		{user_types::RenderTarget::typeDescription.typeName, [](SceneAdaptor* sceneAdaptor, core::SEditorObject obj) { return std::make_unique<RenderTargetAdaptor>(sceneAdaptor, std::dynamic_pointer_cast<user_types::RenderTarget>(obj)); }},
		{user_types::RenderPass::typeDescription.typeName, [](SceneAdaptor* sceneAdaptor, core::SEditorObject obj) { return std::make_unique<RenderPassAdaptor>(sceneAdaptor, std::dynamic_pointer_cast<user_types::RenderPass>(obj)); }},
		{user_types::RenderLayer::typeDescription.typeName, [](SceneAdaptor* sceneAdaptor, core::SEditorObject obj) { return std::make_unique<RenderLayerAdaptor>(sceneAdaptor, std::dynamic_pointer_cast<user_types::RenderLayer>(obj)); }},
		{user_types::Timer::typeDescription.typeName, [](SceneAdaptor* sceneAdaptor, core::SEditorObject obj) { return std::make_unique<TimerAdaptor>(sceneAdaptor, std::dynamic_pointer_cast<user_types::Timer>(obj)); }},

		// LOGIC ENGINE
		{user_types::LuaScript::typeDescription.typeName, [](SceneAdaptor* sceneAdaptor, core::SEditorObject obj) { return std::make_unique<LuaScriptAdaptor>(sceneAdaptor, std::dynamic_pointer_cast<user_types::LuaScript>(obj)); }},
		
		{user_types::LuaInterface::typeDescription.typeName, [](SceneAdaptor* sceneAdaptor, core::SEditorObject obj) { return std::make_unique<LuaInterfaceAdaptor>(sceneAdaptor, std::dynamic_pointer_cast<user_types::LuaInterface>(obj)); }}
	};

	if (factoryByTypename.find(obj->getTypeDescription().typeName) != factoryByTypename.end()) {
		return factoryByTypename.at(obj->getTypeDescription().typeName)(sceneAdaptor, obj);
	}
	return UniqueObjectAdaptor{};
}

}  // namespace raco::ramses_adaptor
