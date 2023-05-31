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

#include "core/DynamicEditorObject.h"

namespace raco::serialization::proxy {

using namespace raco::data_storage;

template <const char* Name, class Base = DynamicEditorObject>
class Proxy : public Base {
public:
	Proxy(std::string name = std::string(), std::string id = std::string())
		: Base(name, id) {
	}

	static inline const ReflectionInterface::TypeDescriptor typeDescription = {Name, true};
	ReflectionInterface::TypeDescriptor const& getTypeDescription() const override {
		return typeDescription;
	}
};

extern const char projectSettingsTypeName[];
using ProjectSettings = Proxy<projectSettingsTypeName>;
using SProjectSettings = std::shared_ptr<ProjectSettings>;

extern const char meshTypeName[];
using Mesh = Proxy<meshTypeName>;
using SMesh = std::shared_ptr<Mesh>;

extern const char nodeTypeName[];
using Node = Proxy<nodeTypeName>;
using SNode = std::shared_ptr<Node>;

extern const char meshNodeTypeName[];
using MeshNode = Proxy<meshNodeTypeName, Node>;
using SMeshNode = std::shared_ptr<MeshNode>;

extern const char materialTypeName[];
using Material = Proxy<materialTypeName>;
using SMaterial = std::shared_ptr<Material>;

extern const char luaScriptTypeName[];
using LuaScript = Proxy<luaScriptTypeName>;
using SLuaScript = std::shared_ptr<LuaScript>;

extern const char luaInterfaceTypeName[];
using LuaInterface = Proxy<luaInterfaceTypeName>;
using SLuaInterface = std::shared_ptr<LuaInterface>;

extern const char luaScriptModuleTypeName[];
using LuaScriptModule = Proxy<luaScriptModuleTypeName>;
using SLuaScriptModule = std::shared_ptr<LuaScriptModule>;

extern const char anchorPointTypeName[];
using AnchorPoint = Proxy<anchorPointTypeName>;
using SAnchorPoint = std::shared_ptr<AnchorPoint>;

extern const char animationTypeName[];
using Animation = Proxy<animationTypeName>;
using SAnimation = std::shared_ptr<Animation>;

extern const char animationChannelTypeName[];
using AnimationChannel = Proxy<animationChannelTypeName>;
using SAnimationChannel = std::shared_ptr<AnimationChannel>;

extern const char textureSampler2DBaseTypeName[];
using TextureSampler2DBase = Proxy<textureSampler2DBaseTypeName>;
using STextureSampler2DBase = std::shared_ptr<TextureSampler2DBase>;

extern const char textureTypeName[];
using Texture = Proxy<textureTypeName, TextureSampler2DBase>;
using STexture = std::shared_ptr<Texture>;

// BaseTexture -> CubeMap
// BaseTexture -> TextureSampler2DBase -> Texture
//                                     -> RenderBuffer

extern const char textureExternalTypeName[];
using TextureExternal = Proxy<textureExternalTypeName>;
using STextureExternal = std::shared_ptr<TextureExternal>;


extern const char blitPassTypeName[];
using BlitPass = Proxy<blitPassTypeName>;
using SBlitPass = std::shared_ptr<BlitPass>;

extern const char cubeMapTypeName[];
using CubeMap = Proxy<cubeMapTypeName>;
using SCubeMap = std::shared_ptr<CubeMap>;

extern const char baseCameraTypeName[];
using BaseCamera = Proxy<baseCameraTypeName, Node>;
using SBaseCamera = std::shared_ptr<BaseCamera>;

extern const char perspectiveCameraTypeName[];
using PerspectiveCamera = Proxy<perspectiveCameraTypeName, BaseCamera>;
using SPerspectiveCamera = std::shared_ptr<PerspectiveCamera>;

extern const char orthographicCameraTypeName[];
using OrthographicCamera = Proxy<orthographicCameraTypeName, BaseCamera>;
using SOrthographicCamera = std::shared_ptr<OrthographicCamera>;

extern const char renderBufferTypeName[];
using RenderBuffer = Proxy<renderBufferTypeName, TextureSampler2DBase>;
using SRenderBuffer = std::shared_ptr<RenderBuffer>;

extern const char renderBufferMSTypeName[];
using RenderBufferMS = Proxy<renderBufferMSTypeName>;
using SRenderBufferMS = std::shared_ptr<RenderBufferMS>;

extern const char renderLayerTypeName[];
using RenderLayer = Proxy<renderLayerTypeName>;
using SRenderLayer = std::shared_ptr<RenderLayer>;

extern const char renderPassTypeName[];
using RenderPass = Proxy<renderPassTypeName>;
using SRenderPass = std::shared_ptr<RenderPass>;

extern const char renderTargetTypeName[];
using RenderTarget = Proxy<renderTargetTypeName>;
using SRenderTarget = std::shared_ptr<RenderTarget>;

extern const char prefabTypeName[];
using Prefab = Proxy<prefabTypeName>;
using SPrefab = std::shared_ptr<Prefab>;

extern const char prefabInstanceTypeName[];
using PrefabInstance = Proxy<prefabInstanceTypeName>;
using SPrefabInstance = std::shared_ptr<PrefabInstance>;

extern const char skinTypeName[];
using Skin = Proxy<skinTypeName>;
using SSkin = std::shared_ptr<Skin>;

extern const char timerTypeName[];
using Timer = Proxy<timerTypeName>;
using STimer = std::shared_ptr<Timer>;


template<const char* Name>
class StructProxy : public DynamicGenericStruct {
public:
	static inline const TypeDescriptor typeDescription = {Name, true};
	TypeDescriptor const& getTypeDescription() const override {
		return typeDescription;
	}
	bool serializationRequired() const override {
		return true;
	}

	StructProxy() : DynamicGenericStruct() {}

	StructProxy(const StructProxy& other, std::function<SEditorObject(SEditorObject)>* translateRef = nullptr) {
		for (size_t i = 0; i < other.dynamicProperties_.size(); i++) {
			addProperty(other.name(i), other.get(i)->clone(translateRef), -1);
		}
	}

	StructProxy& operator=(const StructProxy& other) {
		removeAllProperties();
		for (size_t i = 0; i < other.dynamicProperties_.size(); i++) {
			addProperty(other.name(i), other.get(i)->clone(nullptr), -1);
		}
		return *this;
	}

	void copyAnnotationData(const StructProxy& other) {
		for (size_t i = 0; i < dynamicProperties_.size(); i++) {
			get(i)->copyAnnotationData(*other.get(i));
		}
	}
};

extern const char vec2fTypeName[];
using Vec2f = StructProxy<vec2fTypeName>;
extern const char vec3fTypeName[];
using Vec3f = StructProxy<vec3fTypeName>;
extern const char vec4fTypeName[];
using Vec4f = StructProxy<vec4fTypeName>;

extern const char vec2iTypeName[];
using Vec2i = StructProxy<vec2iTypeName>;
extern const char vec3iTypeName[];
using Vec3i = StructProxy<vec3iTypeName>;
extern const char vec4iTypeName[];
using Vec4i = StructProxy<vec4iTypeName>;

extern const char colorWriteMaskTypeName[];
using ColorWriteMask = StructProxy<colorWriteMaskTypeName>;

extern const char blendmodeOptionsTypeName[];
using BlendOptions = StructProxy<blendmodeOptionsTypeName>;

extern const char cameraViewportTypeName[];
using CameraViewport = StructProxy<cameraViewportTypeName>;

extern const char perspectiveFrustumTypeName[];
using PerspectiveFrustum = StructProxy<perspectiveFrustumTypeName>;

extern const char orthographicFrustumTypeName[];
using OrthographicFrustum = StructProxy<orthographicFrustumTypeName>;

extern const char defaultResourceDirectoriesTypeName[];
using DefaultResourceDirectories = StructProxy<defaultResourceDirectoriesTypeName>;

extern const char luaStandardModuleSelectionTypeName[];
using LuaStandardModuleSelection = StructProxy<luaStandardModuleSelectionTypeName>;

extern const char scissorOptionsTypeName[];
using ScissorOptions = StructProxy<scissorOptionsTypeName>;

extern const char stencilOptionsTypeName[];
using StencilOptions = StructProxy<stencilOptionsTypeName>;

extern const char timerInputTypeName[];
using TimerInput = StructProxy<timerInputTypeName>;

extern const char timerOutputTypeName[];
using TimerOutput = StructProxy<timerOutputTypeName>;

extern const char anchorPointOutputsTypeName[];
using AnchorPointOutputs = StructProxy<anchorPointOutputsTypeName>;

}  // namespace raco::serialization::proxy
