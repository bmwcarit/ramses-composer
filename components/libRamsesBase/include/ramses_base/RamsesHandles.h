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

#include "log_system/log.h"
#include "ramses_base/Utils.h"
#include "ramses_base/LogicEngineFormatter.h"

#include <ramses-utils.h>

#include <memory>
#include <ramses-client-api/Appearance.h>
#include <ramses-client-api/ArrayResource.h>
#include <ramses-client-api/AttributeInput.h>
#include <ramses-client-api/BlitPass.h>
#include <ramses-client-api/Camera.h>
#include <ramses-client-api/Effect.h>
#include <ramses-client-api/EffectDescription.h>
#include <ramses-client-api/GeometryBinding.h>
#include <ramses-client-api/MeshNode.h>
#include <ramses-client-api/Node.h>
#include <ramses-client-api/OrthographicCamera.h>
#include <ramses-client-api/PerspectiveCamera.h>
#include <ramses-client-api/RamsesClient.h>
#include <ramses-client-api/RenderBuffer.h>
#include <ramses-client-api/RenderGroup.h>
#include <ramses-client-api/RenderPass.h>
#include <ramses-client-api/RenderTarget.h>
#include <ramses-client-api/RenderTargetDescription.h>
#include <ramses-client-api/Scene.h>
#include <ramses-client-api/Texture2D.h>
#include <ramses-client-api/TextureCube.h>
#include <ramses-client-api/TextureSampler.h>
#include <ramses-client-api/TextureSamplerMS.h>
#include <ramses-client-api/TextureSamplerExternal.h>
#include <ramses-client-api/UniformInput.h>

#include <ramses-logic/AnchorPoint.h>
#include <ramses-logic/AnimationNode.h>
#include <ramses-logic/DataArray.h>
#include <ramses-logic/LogicEngine.h>
#include <ramses-logic/LuaScript.h>
#include <ramses-logic/LuaInterface.h>
#include <ramses-logic/LuaModule.h>
#include <ramses-logic/TimerNode.h>
#include <ramses-logic/RamsesAppearanceBinding.h>
#include <ramses-logic/RamsesCameraBinding.h>
#include <ramses-logic/RamsesNodeBinding.h>
#include <ramses-logic/RamsesRenderPassBinding.h>
#include <ramses-logic/RamsesRenderGroupBinding.h>
#include <ramses-logic/RamsesRenderGroupBindingElements.h>
#include <ramses-logic/AnimationNodeConfig.h>
#include <ramses-logic/SkinBinding.h>

#include <map>
#include <stdexcept>
#include <set>

namespace raco::ramses_base {

using RamsesObjectDeleter = std::function<void(ramses::RamsesObject*)>;
template <typename T>
using RamsesHandle = std::shared_ptr<T>;
using RamsesObject = RamsesHandle<ramses::RamsesObject>;
using RamsesScene = RamsesHandle<ramses::Scene>;
using RamsesRenderBuffer = RamsesHandle<ramses::RenderBuffer>;
using RamsesRenderTarget = RamsesHandle<ramses::RenderTarget>;
using RamsesTextureSampler = RamsesHandle<ramses::TextureSampler>;
using RamsesTextureSamplerMS = RamsesHandle<ramses::TextureSamplerMS>;
using RamsesTextureSamplerExternal = RamsesHandle<ramses::TextureSamplerExternal>;
using RamsesTimerNode = RamsesHandle<rlogic::TimerNode>;
using RamsesLuaModule = RamsesHandle<rlogic::LuaModule>;
using RamsesLuaScript = RamsesHandle<rlogic::LuaScript>;
using RamsesLuaInterface = RamsesHandle<rlogic::LuaInterface>;
using RamsesAnchorPoint = RamsesHandle<rlogic::AnchorPoint>;
using RamsesSkinBinding = RamsesHandle<rlogic::SkinBinding>;

	/** RESOURCE HANDLES */
using RamsesEffect = RamsesHandle<ramses::Effect>;
using RamsesArrayResource = RamsesHandle<ramses::ArrayResource>;
using RamsesTexture2D = RamsesHandle<ramses::Texture2D>;
using RamsesTextureCube = RamsesHandle<ramses::TextureCube>;
using RamsesBlitPass = RamsesHandle<ramses::BlitPass>;

template <typename OwnerType, typename RamsesType>
void destroyRamsesObject(OwnerType* owner, RamsesType* obj) {
	if (obj) {
		auto status = owner->destroy(*obj);
		if (ramses::StatusOK != status) {
			LOG_ERROR(raco::log_system::RAMSES_BACKEND, "Deleting Ramses object failed: {}", owner->getStatusMessage(status));
			assert(false);
			exit(1);
		}
	}
}

template <typename RamsesType, typename OwnerType>
constexpr RamsesObjectDeleter createRamsesObjectDeleter(OwnerType* owner) {
	return [owner](ramses::RamsesObject* obj) {
		destroyRamsesObject(owner, static_cast<RamsesType*>(obj));
	};
}

template <typename LogicType>
void destroyLogicObject(rlogic::LogicEngine* logicengine, LogicType* obj) {
	if (obj) {
		auto status = logicengine->destroy(*obj);
		if (!status) {
			LOG_ERROR(raco::log_system::RAMSES_BACKEND, "Deleting LogicEngine object failed: {}", LogicEngineErrors{*logicengine});
			assert(false);
			exit(1);
		}
	}
}


struct RamsesAppearanceHandle {
	RamsesAppearanceHandle(ramses::Scene* scene, raco::ramses_base::RamsesEffect effect)
		: appearance_(scene->createAppearance(*effect)), scene_(scene), trackedEffect_(effect) {}

	~RamsesAppearanceHandle() {
		destroyRamsesObject(scene_, appearance_);
	}

	ramses::Appearance& operator*() {
		return *appearance_;
	}

	ramses::Appearance* operator->() {
		return appearance_;
	}

	ramses::Appearance* get() {
		return appearance_;
	}

	void replaceTrackedSamplers(std::vector<raco::ramses_base::RamsesTextureSampler>& newSamplers, std::vector<raco::ramses_base::RamsesTextureSamplerMS>& newSamplersMS, std::vector<raco::ramses_base::RamsesTextureSamplerExternal>& newSamplersExternal) {
		trackedSamplers_ = newSamplers;
		trackedSamplersMS_ = newSamplersMS;
		trackedSamplersExternal_ = newSamplersExternal;
	}

	RamsesEffect effect() {
		return trackedEffect_;
	}

private:
	// The appearance is owned by this class.
	ramses::Appearance* appearance_;

	// Kept for reference; needed to destroy the appearance pointer in ramses.
	ramses::Scene* scene_;

	// Effect currently used by the appearance_. Needed to keep the effect alive in ramses.
	RamsesEffect trackedEffect_;

	// Samplers currently in use by the appearance_. Needed to keep the samplers alive in ramses.
	std::vector<raco::ramses_base::RamsesTextureSampler> trackedSamplers_;
	std::vector<raco::ramses_base::RamsesTextureSamplerMS> trackedSamplersMS_;
	std::vector<raco::ramses_base::RamsesTextureSamplerExternal> trackedSamplersExternal_;
};

using RamsesAppearance = std::shared_ptr<RamsesAppearanceHandle>;

inline RamsesAppearance ramsesAppearance(ramses::Scene* scene, raco::ramses_base::RamsesEffect effect) {
	return std::make_shared<RamsesAppearanceHandle>(scene, effect);
}

inline RamsesScene ramsesScene(ramses::sceneId_t id, ramses::RamsesClient* client) {
	return {client->createScene(id), createRamsesObjectDeleter<ramses::Scene>(client)};
}

struct RamsesGeometryBindingHandle {
	RamsesGeometryBindingHandle(ramses::Scene* scene, raco::ramses_base::RamsesEffect effect)
		: geometryBinding_(scene->createGeometryBinding(*effect)), scene_(scene), trackedEffect_(effect) {
	}

	~RamsesGeometryBindingHandle() {
		destroyRamsesObject(scene_, geometryBinding_);
	}

	ramses::GeometryBinding& operator*() {
		return *geometryBinding_;
	}

	ramses::GeometryBinding* operator->() {
		return geometryBinding_;
	}

	void setIndices(RamsesArrayResource indexBuffer) {
		auto status = geometryBinding_->setIndices(*indexBuffer);
		if (status == ramses::StatusOK) {
			trackedMeshIndices_ = indexBuffer;
		} else {
			LOG_ERROR(raco::log_system::RAMSES_ADAPTOR, geometryBinding_->getStatusMessage(status));
		}
	}

	void addAttributeBuffer(const ramses::AttributeInput& attribInput, RamsesArrayResource attributeBuffer) {
		auto status = geometryBinding_->setInputBuffer(attribInput, *attributeBuffer);
		if (status == ramses::StatusOK) {
			trackedMeshVertexData_.emplace_back(attributeBuffer);
		} else {
			LOG_ERROR(raco::log_system::RAMSES_ADAPTOR, geometryBinding_->getStatusMessage(status));
		}
	}

private:
	// The geometryBinding_ is owned by this class.
	ramses::GeometryBinding* geometryBinding_;

	// Kept for reference; needed to destroy the GeometryBinding pointer in ramses.
	ramses::Scene* scene_;

	// Effect currently used by the geometryBinding_. Needed to keep the effect alive in ramses.
	RamsesEffect trackedEffect_;

	// Attribute buffers currently used by the geometryBinding_. Needed to keep the buffers alive in ramses.
	std::vector<raco::ramses_base::RamsesArrayResource> trackedMeshVertexData_;
	
	// Index buffer currently used by the geometryBinding_. Needed to keep the buffer alive in ramses.
	raco::ramses_base::RamsesArrayResource trackedMeshIndices_;
};

using RamsesGeometryBinding = std::shared_ptr<RamsesGeometryBindingHandle>;

inline RamsesGeometryBinding ramsesGeometryBinding(ramses::Scene* scene, raco::ramses_base::RamsesEffect effect) {
	return std::make_shared<RamsesGeometryBindingHandle>(scene, effect);
}

template<typename RamsesType, auto PToM>
struct RamsesWrapper {
	RamsesWrapper(ramses::Scene* scene)
		: ramsesObject_((scene->*PToM)(nullptr)), scene_(scene) {
	}

	~RamsesWrapper() {
		destroyRamsesObject(scene_, ramsesObject_);
	}

	const char* getName() const {
		return ramsesObject_->getName();
	}

	void setName(const char* name) {
		ramsesObject_->setName(name);
	}

	RamsesType& operator*() {
		return *ramsesObject_;
	}

	RamsesType* operator->() {
		return ramsesObject_;
	}

	RamsesType* get() {
		return ramsesObject_;
	}

private:
	// The ramsesObject_ is owned by this class.
	RamsesType* ramsesObject_;

	// Kept for reference; needed to destroy the meshnode pointer in ramses.
	ramses::Scene* scene_;
};

using RamsesNodeHandle = RamsesWrapper<ramses::Node, &ramses::Scene::createNode>;
using RamsesNode = std::shared_ptr<RamsesNodeHandle>;

inline RamsesNode ramsesNode(ramses::Scene* scene) {
	return std::make_shared<RamsesNodeHandle>(scene);
}

using RamsesOrthographicCameraHandle = RamsesWrapper<ramses::OrthographicCamera, &ramses::Scene::createOrthographicCamera>;
using RamsesOrthographicCamera = std::shared_ptr<RamsesOrthographicCameraHandle>;

inline RamsesOrthographicCamera ramsesOrthographicCamera(ramses::Scene* scene) {
	return std::make_shared<RamsesOrthographicCameraHandle>(scene);
}

using RamsesPerspectiveCameraHandle = RamsesWrapper<ramses::PerspectiveCamera, &ramses::Scene::createPerspectiveCamera>;
using RamsesPerspectiveCamera = std::shared_ptr<RamsesPerspectiveCameraHandle>;

inline RamsesPerspectiveCamera ramsesPerspectiveCamera(ramses::Scene* scene) {
	return std::make_shared<RamsesPerspectiveCameraHandle>(scene);
}


struct RamsesMeshNodeHandle {
	RamsesMeshNodeHandle(ramses::Scene* scene) 
	: meshnode_(scene->createMeshNode()), scene_(scene) {
	}
	
	~RamsesMeshNodeHandle() {
		destroyRamsesObject(scene_, meshnode_);
	}

	const char* getName() const {
		return meshnode_->getName();
	}

	void setName(const char* name) {
		meshnode_->setName(name);
	}

	ramses::MeshNode& operator*() {
		return *meshnode_;
	}

	ramses::MeshNode* get() {
		return meshnode_;
	}

	void removeAppearanceAndGeometry() {
		meshnode_->removeAppearanceAndGeometry();
		trackedAppearance_.reset();
		trackedGeometryBinding_.reset();
	}

	void setAppearance(RamsesAppearance appearance) {
		meshnode_->setAppearance(**appearance);
		trackedAppearance_ = appearance;
	}

	void setGeometryBinding(RamsesGeometryBinding geometryBinding) {
		meshnode_->setGeometryBinding(**geometryBinding);
		trackedGeometryBinding_ = geometryBinding;
	}

private:
	// The meshnode is owned by this class.
	ramses::MeshNode* meshnode_;

	// Kept for reference; needed to destroy the meshnode pointer in ramses.
	ramses::Scene* scene_;

	// Appearance currently used by the meshnode_. Needed to keep the Appearance alive in ramses.
	RamsesAppearance trackedAppearance_;

	// GeometryBinding currently used by the meshnode_. Needed to keep the GeometryBinding alive in ramses.
	RamsesGeometryBinding trackedGeometryBinding_;
};

using RamsesMeshNode = std::shared_ptr<RamsesMeshNodeHandle>;

inline RamsesMeshNode ramsesMeshNode(ramses::Scene* scene) {
	return std::make_shared<RamsesMeshNodeHandle>(scene);
}

struct RamsesRenderGroupHandle;
using RamsesRenderGroup = std::shared_ptr<RamsesRenderGroupHandle>;

struct RamsesRenderGroupHandle {
	RamsesRenderGroupHandle(ramses::Scene* scene)
		: renderGroup_(scene->createRenderGroup()), scene_(scene) {}

	~RamsesRenderGroupHandle() {
		destroyRamsesObject(scene_, renderGroup_);
	}

	const char* getName() const {
		return renderGroup_->getName();
	}

	void setName(const char* name) {
		renderGroup_->setName(name);
	}

	ramses::RenderGroup& operator*() {
		return *renderGroup_;
	}

	void removeAllRenderables() {
		renderGroup_->removeAllRenderables();
		trackedRenderables_.clear();
	}

	void removeAllRenderGroups() {
		renderGroup_->removeAllRenderGroups();
		trackedGroups_.clear();
	}

	void addMeshNode(RamsesMeshNode meshnode, int32_t orderWithinGroup) {
		renderGroup_->addMeshNode(**meshnode, orderWithinGroup);
		trackedRenderables_[meshnode] = orderWithinGroup;		
	}

	void addRenderGroup(RamsesRenderGroup group, int32_t orderWithinGroup) {
		renderGroup_->addRenderGroup(**group, orderWithinGroup);
		trackedGroups_[group] = orderWithinGroup;		
	}

	bool containsMeshNode(RamsesMeshNode meshnode) const {
		return trackedRenderables_.find(meshnode) != trackedRenderables_.end();		
	}

	int getMeshNodeOrder(RamsesMeshNode meshnode) const {
		if (auto it = trackedRenderables_.find(meshnode); it != trackedRenderables_.end()) {
			return it->second;
		}
		assert(false);
		return 0;
	}

	bool containsRenderGroup(RamsesRenderGroup group) const {
		return trackedGroups_.find(group) != trackedGroups_.end();
	}

	int getRenderGroupOrder(RamsesRenderGroup group) const {
		if (auto it = trackedGroups_.find(group); it != trackedGroups_.end()) {
			return it->second;
		}
		assert(false);
		return 0;
	}

	const std::map<RamsesRenderGroup, int>& containedRenderGroups() const {
		return trackedGroups_;
	}

	bool empty() const {
		return trackedRenderables_.empty() && trackedGroups_.empty();
	}

private:
	// The rendergroup is owned by this class.
	ramses::RenderGroup* renderGroup_;

	// Kept for reference; needed to destroy the render group pointer in ramses.
	ramses::Scene* scene_;

	// MeshNodes currently used by the renderGroup_. Needed to keep the MeshNodes alive in ramses.
	// Also used to track the order indices for the mesh nodes.
	std::map<RamsesMeshNode, int> trackedRenderables_;

	// Nested RenderGroups currently used by the renderGroup_. Needed to keep the RenderGroups alive in ramses.
	// Also used to track the order indices for the render groups.
	std::map<RamsesRenderGroup, int> trackedGroups_;
};

inline RamsesRenderGroup ramsesRenderGroup(ramses::Scene* scene) {
	return std::make_shared<RamsesRenderGroupHandle>(scene);
}

inline RamsesRenderBuffer ramsesRenderBuffer(ramses::Scene* scene,
	uint32_t width, uint32_t height, ramses::ERenderBufferType type, ramses::ERenderBufferFormat format, ramses::ERenderBufferAccessMode accessMode, uint32_t sampleCount = 0u, const char* name = nullptr) {
	return {scene->createRenderBuffer(width, height, type, format, accessMode, sampleCount, name),
		createRamsesObjectDeleter<ramses::RenderBuffer>(scene)};
}

struct RamsesRenderPassHandle {
	RamsesRenderPassHandle(std::shared_ptr<ramses::RenderPass> renderPass)
		: renderPass_(renderPass) {}

	const char* getName() const {
		return renderPass_->getName();
	}

	void setName(const char* name) {
		renderPass_->setName(name);
	}

	ramses::RenderPass& operator*() {
		return *renderPass_;
	}

	void removeAllRenderGroups() {
		renderPass_->removeAllRenderGroups();
		trackedRenderGroups_.clear();
	}

	void addRenderGroup(RamsesRenderGroup renderGroup) {
		renderPass_->addRenderGroup(**renderGroup);
		trackedRenderGroups_.insert(renderGroup);
	}

private:
	// The appearance is owned by this class.
	std::shared_ptr<ramses::RenderPass> renderPass_;

	// MeshNodes currently used by the renderPass_. Needed to keep the effect alive in ramses.
	std::set<RamsesRenderGroup> trackedRenderGroups_;
};

using RamsesRenderPass = std::shared_ptr<RamsesRenderPassHandle>;

template <typename CameraHandleType>
inline std::shared_ptr<ramses::RenderPass> ramsesRenderPassInternal(ramses::Scene* scene, CameraHandleType camera, RamsesRenderTarget target, const char* name = nullptr) {
	if (!camera) {
		return {};
	}

	std::shared_ptr<ramses::RenderPass> renderPass{scene->createRenderPass(name),
		[scene, forceCameraCopy = camera, forceTargetCopy = target](ramses::RamsesObject* obj) {
			destroyRamsesObject(scene, static_cast<ramses::RenderPass*>(obj));
		}};
	renderPass->setCamera(**camera);
	renderPass->setRenderTarget(target.get());
	return renderPass;
}

template <typename CameraHandleType>
inline RamsesRenderPass ramsesRenderPass(ramses::Scene* scene, CameraHandleType camera, RamsesRenderTarget target, const char* name = nullptr) {
	if (!camera) {
		return {};
	}
	return std::make_shared<RamsesRenderPassHandle>(ramsesRenderPassInternal(scene, camera, target, name));
}

inline RamsesRenderTarget ramsesRenderTarget(ramses::Scene* scene, const ramses::RenderTargetDescription& rtDesc, const std::vector<RamsesRenderBuffer>& buffers) {
	if (buffers.empty()) {
		return nullptr;
	}

	return {
		scene->createRenderTarget(rtDesc),
		[scene, forceCopy = buffers](ramses::RamsesObject* obj) {
			destroyRamsesObject(scene, static_cast<ramses::RenderTarget*>(obj));
		}};
}

inline RamsesArrayResource ramsesArrayResource(ramses::Scene* scene, ramses::EDataType type, uint32_t numElements, const void* arrayData, const char* name = nullptr) {
	RamsesArrayResource result{scene->createArrayResource(type, numElements, arrayData), createRamsesObjectDeleter<ramses::ArrayResource>(scene)};
	result->setName(name);
	return result;
}

template <typename RamsesTexture>
inline RamsesTextureSampler ramsesTextureSampler(ramses::Scene* scene, ramses::ETextureAddressMode wrapUMode, ramses::ETextureAddressMode wrapVMode, ramses::ETextureSamplingMethod minSamplingMethod, ramses::ETextureSamplingMethod magSamplingMethod, RamsesTexture texture, uint32_t anisotropyLevel, const char* name = nullptr) {
	auto ptr = scene->createTextureSampler(wrapUMode, wrapVMode, minSamplingMethod, magSamplingMethod, *texture, anisotropyLevel, name);
	return {ptr,
		[scene, forceCopy = texture](ramses::RamsesObject* obj) {
			auto status = scene->destroy(*static_cast<ramses::TextureSampler*>(obj));
			if (ramses::StatusOK != status) {
				throw std::runtime_error(obj->getStatusMessage(status));
			}
		}};
}

inline RamsesTextureSamplerMS ramsesTextureSamplerMS(ramses::Scene* scene, RamsesRenderBuffer buffer, const char* name = nullptr) {
	auto ptr = scene->createTextureSamplerMS(*buffer, name);
	return {
		ptr,
		[scene, forceCopy = buffer](ramses::RamsesObject* buffer) {
			destroyRamsesObject(scene, static_cast<ramses::RenderBuffer*>(buffer));
		}};
}

inline RamsesTextureSamplerExternal ramsesTextureSamplerExternal(ramses::Scene* scene, ramses::ETextureSamplingMethod minSamplingMethod, ramses::ETextureSamplingMethod magSamplingMethod, const char* name = nullptr) {
	return {
		scene->createTextureSamplerExternal(minSamplingMethod, magSamplingMethod, name),
		createRamsesObjectDeleter<ramses::TextureSamplerExternal>(scene)
	};
}

/** RESOURCE FACTORIES */

inline RamsesEffect ramsesEffect(ramses::Scene* scene, const ramses::EffectDescription& description, const char* name = nullptr) {
	return {scene->createEffect(description, ramses::ResourceCacheFlag_DoNotCache, name), createRamsesObjectDeleter<ramses::Effect>(scene)};
}

inline RamsesTexture2D ramsesTexture2D(ramses::Scene* scene,
	ramses::ETextureFormat format,
	uint32_t width,
	uint32_t height,
	uint32_t mipMapCount,
	const ramses::MipLevelData mipLevelData[],
	bool generateMipChain,
	const ramses::TextureSwizzle& swizzle,
	ramses::resourceCacheFlag_t cacheFlag,
	const char* name = nullptr) {
	return {scene->createTexture2D(format, width, height, mipMapCount, mipLevelData, generateMipChain, swizzle, cacheFlag, name), createRamsesObjectDeleter<ramses::Texture2D>(scene)};
}

inline RamsesTexture2D ramsesTexture2DFromPng(ramses::Scene* scene, const std::string& uri) {
	return {ramses::RamsesUtils::CreateTextureResourceFromPng(uri.c_str(), *scene),
		createRamsesObjectDeleter<ramses::Texture2D>(scene)};
}

inline RamsesTexture2D ramsesTexture2DFromPngBuffer(ramses::Scene* scene, const std::vector<unsigned char>& pngData) {
	return {ramses::RamsesUtils::CreateTextureResourceFromPngBuffer(pngData, *scene),
		createRamsesObjectDeleter<ramses::Texture2D>(scene)};
}

inline RamsesTextureCube ramsesTextureCube(ramses::Scene* scene,
	ramses::ETextureFormat format,
	uint32_t width,
	uint32_t mipMapCount,
	const ramses::CubeMipLevelData mipLevelData[],
	bool generateMipChain,
	const ramses::TextureSwizzle& swizzle,
	ramses::resourceCacheFlag_t cacheFlag,
	const char* name = nullptr) {
	return {scene->createTextureCube(format, width, mipMapCount, mipLevelData, generateMipChain, swizzle, cacheFlag, name),
		createRamsesObjectDeleter<ramses::TextureCube>(scene)};
}

inline RamsesBlitPass ramsesBlitPass(ramses::Scene* scene, RamsesRenderBuffer source, RamsesRenderBuffer dest,
	const char* name = nullptr) {
	return {
		scene->createBlitPass(*source, *dest, name),
		[scene, forceSourceCopy = source, forceDestCopy = dest](ramses::RamsesObject* object) {
			destroyRamsesObject(scene, static_cast<ramses::BlitPass*>(object));
		}};
}

using RamsesAppearanceBinding = std::shared_ptr<rlogic::RamsesAppearanceBinding>;
using UniqueRamsesDataArray = std::unique_ptr<rlogic::DataArray, std::function<void(rlogic::DataArray*)>>;
using RamsesNodeBinding = std::shared_ptr<rlogic::RamsesNodeBinding>;
using RamsesCameraBinding = std::shared_ptr<rlogic::RamsesCameraBinding>;
using UniqueRamsesRenderPassBinding = std::unique_ptr<rlogic::RamsesRenderPassBinding, std::function<void(rlogic::RamsesRenderPassBinding*)>>;
using RamsesRenderGroupBinding = std::shared_ptr<rlogic::RamsesRenderGroupBinding>;


inline RamsesAppearanceBinding ramsesAppearanceBinding(ramses::Appearance& appearance, rlogic::LogicEngine* logicEngine, const std::string& name, const std::pair<uint64_t, uint64_t> &objectID) {
	RamsesAppearanceBinding binding{logicEngine->createRamsesAppearanceBinding(appearance, name),
		[logicEngine](rlogic::RamsesAppearanceBinding* binding) {
			destroyLogicObject(logicEngine, binding);
		}};

	if (binding) {
		binding->setUserId(objectID.first, objectID.second);
	}

	return binding;
}

template<typename NodeHandleType>
inline RamsesNodeBinding ramsesNodeBinding(NodeHandleType node, rlogic::LogicEngine* logicEngine, rlogic::ERotationType rotationType, const std::pair<uint64_t, uint64_t>& objectID) {
	RamsesNodeBinding binding{logicEngine->createRamsesNodeBinding(**node, rotationType), 
		[logicEngine, forceNodeCopy = node](rlogic::RamsesNodeBinding* binding) {
				destroyLogicObject(logicEngine, binding);
			}};

	if (binding) {
		binding->setUserId(objectID.first, objectID.second);
	}

	return binding;
}

template <typename T>
inline UniqueRamsesDataArray ramsesDataArray(const std::vector<T>& vec, rlogic::LogicEngine* logicEngine, const std::string& name, const std::pair<uint64_t, uint64_t>& objectID) {
	UniqueRamsesDataArray array{logicEngine->createDataArray(vec, name), [logicEngine](rlogic::DataArray* array) {
				destroyLogicObject(logicEngine, array);
			}};

	if (array) {
		array->setUserId(objectID.first, objectID.second);
	}

	return array;
}

template <typename CameraHandleType>
inline RamsesCameraBinding ramsesCameraBinding(CameraHandleType camera, rlogic::LogicEngine* logicEngine, const std::pair<uint64_t, uint64_t>& objectID, bool frustumPlanes) {
	RamsesCameraBinding binding{
		frustumPlanes ? logicEngine->createRamsesCameraBindingWithFrustumPlanes(**camera) : logicEngine->createRamsesCameraBinding(**camera), 
		[logicEngine, forceCameraCopy = camera](rlogic::RamsesCameraBinding* binding) {
				destroyLogicObject(logicEngine, binding);
			}};

	if (binding) {
		binding->setUserId(objectID.first, objectID.second);
	}

	return binding;
}

inline UniqueRamsesRenderPassBinding ramsesRenderPassBinding(ramses::RenderPass& renderpass, rlogic::LogicEngine* logicEngine, const std::string& name, const std::pair<uint64_t, uint64_t>& objectID) {
	UniqueRamsesRenderPassBinding binding{logicEngine->createRamsesRenderPassBinding(renderpass, name), [logicEngine](rlogic::RamsesRenderPassBinding* binding) {
										  destroyLogicObject(logicEngine, binding);
									  }};

	if (binding) {
		binding->setUserId(objectID.first, objectID.second);
	}

	return binding;
}


inline RamsesTimerNode ramsesTimer(rlogic::LogicEngine* logicEngine, const std::string& name, const std::pair<uint64_t, uint64_t>& objectID) {
	RamsesTimerNode node{logicEngine->createTimerNode(name), [logicEngine](rlogic::TimerNode* timer) {
				destroyLogicObject(logicEngine, timer);
			}};

	if (node) {
		node->setUserId(objectID.first, objectID.second);
	}

	return node;
}

inline RamsesLuaModule ramsesLuaModule(const std::string& luaContent, rlogic::LogicEngine* logicEngine, rlogic::LuaConfig& config, const std::string& name, const std::pair<uint64_t, uint64_t>& objectID) {

	RamsesLuaModule module{
		logicEngine->createLuaModule(luaContent, config, name), [logicEngine](rlogic::LuaModule* module) {
			destroyLogicObject(logicEngine, module);
		}};

	if (module) {
		module->setUserId(objectID.first, objectID.second);
	}

	return module;
}

inline RamsesLuaScript ramsesLuaScript(rlogic::LogicEngine* logicEngine, const std::string& scriptText, rlogic::LuaConfig& config, std::vector<raco::ramses_base::RamsesLuaModule> modules, const std::string& name, const std::pair<uint64_t, uint64_t>& objectID) {
	RamsesLuaScript script{
		logicEngine->createLuaScript(scriptText, config, name), 
		[logicEngine, forceCopy = modules](rlogic::LuaScript* script) {
			destroyLogicObject(logicEngine, script);
		}};

	if (script) {
		script->setUserId(objectID.first, objectID.second);
	}

	return script;
}


/// Old style creation function: doesn't generate error if interface text contains modules() statement
/// used at feature level < 5
inline RamsesLuaInterface ramsesLuaInterface(rlogic::LogicEngine* logicEngine, const std::string& interfaceText, const std::string& name, const std::pair<uint64_t, uint64_t>& objectID) {
	RamsesLuaInterface interface {
		logicEngine->createLuaInterface(interfaceText, name),
			[logicEngine](rlogic::LuaInterface* interface) {
				destroyLogicObject(logicEngine, interface);
			}
	};

	if (interface) {
		interface->setUserId(objectID.first, objectID.second);
	}
	return interface;
}

/// New style creation function: must supply modules if interface text contains modules() statement
/// used at feature level >= 5
inline RamsesLuaInterface ramsesLuaInterface(rlogic::LogicEngine* logicEngine, const std::string& interfaceText, rlogic::LuaConfig& config, std::vector<raco::ramses_base::RamsesLuaModule> modules, const std::string& name, const std::pair<uint64_t, uint64_t>& objectID) {
	RamsesLuaInterface interface{
		logicEngine->createLuaInterface(interfaceText, name, config),
		[logicEngine, forceCopy = modules](rlogic::LuaInterface* interface) {
			destroyLogicObject(logicEngine, interface);
		}};

	if (interface) {
		interface->setUserId(objectID.first, objectID.second);
	}
	return interface;
}

struct RamsesAnimationChannelData {
	std::string name;
	rlogic::EInterpolationType interpolationType;
	UniqueRamsesDataArray keyframeTimes;
	UniqueRamsesDataArray animOutput;
	UniqueRamsesDataArray tangentIn;
	UniqueRamsesDataArray tangentOut;
};

using RamsesAnimationChannelHandle = RamsesHandle<RamsesAnimationChannelData>;

struct RamsesAnimationNodeHandle {
	RamsesAnimationNodeHandle(rlogic::LogicEngine* logicEngine, rlogic::AnimationNode* ramsesAnimationNode, std::vector<RamsesAnimationChannelHandle> channelHandles, const std::pair<uint64_t, uint64_t>& objectID) 
	: logicEngine_(logicEngine), animationNode_(ramsesAnimationNode), trackedAnimationChannels_(channelHandles) {
		if (animationNode_) {
			animationNode_->setUserId(objectID.first, objectID.second);
		}
	}

	~RamsesAnimationNodeHandle() {
		destroyLogicObject(logicEngine_, animationNode_);
	}

	rlogic::AnimationNode& operator*() {
		return *animationNode_;
	}

	rlogic::AnimationNode* operator->() {
		return animationNode_;
	}

	rlogic::AnimationNode* get() {
		return animationNode_;
	}

	const std::vector<RamsesAnimationChannelHandle>& channels() const {
		return trackedAnimationChannels_;
	}

private:
	// Kept for reference. Needed to destroy the animation node.
	rlogic::LogicEngine* logicEngine_;

	// The animation node is owned by this class.
	rlogic::AnimationNode* animationNode_;

	// AnimationChannels currently used by the animation node. Needed to keep the effect alive in ramses.
	// May contain nullptrs;
	std::vector<RamsesAnimationChannelHandle> trackedAnimationChannels_;
};

using RamsesAnimationNode = std::shared_ptr<RamsesAnimationNodeHandle>;


inline RamsesAnimationNode ramsesAnimationNode(rlogic::LogicEngine* logicEngine, const rlogic::AnimationNodeConfig &config, 
std::vector<RamsesAnimationChannelHandle> channelHandles, const std::string& name, const std::pair<uint64_t, uint64_t>& objectID) {
	if (channelHandles.empty()) {
		return {};
	}

	auto animationNode = logicEngine->createAnimationNode(config, name);
	if (animationNode) {
		return std::make_shared<RamsesAnimationNodeHandle>(logicEngine, animationNode, channelHandles, objectID);
	}
	return {};
}



inline RamsesAnchorPoint ramsesAnchorPoint(rlogic::LogicEngine* logicEngine, RamsesNodeBinding nodeBinding, RamsesCameraBinding cameraBinding, const std::string& name, const std::pair<uint64_t, uint64_t>& objectID) {
	RamsesAnchorPoint object(
		logicEngine->createAnchorPoint(*nodeBinding, *cameraBinding, name),
			[logicEngine, forceNodeBindingCopy = nodeBinding, forceCameraBdingingCopy = cameraBinding](rlogic::AnchorPoint* object) {
			destroyLogicObject(logicEngine, object);
		});

	if (object) {
		object->setUserId(objectID.first, objectID.second);
	}

	return object;
}

inline RamsesSkinBinding ramsesSkinBinding(rlogic::LogicEngine* logicEngine, 
	std::vector<RamsesNodeBinding> joints,
	std::vector<std::array<float, 16>>& inverseBindMatrices,
	RamsesAppearanceBinding& appearanceBinding,
	ramses::UniformInput& jointMatInput,
	const std::string& name, const std::pair<uint64_t, uint64_t>& objectID) {

	std::vector<const rlogic::RamsesNodeBinding*> nodeBindings;
	for (const auto& binding : joints) {
		nodeBindings.emplace_back(binding.get());
	}

	RamsesSkinBinding object(logicEngine->createSkinBinding(nodeBindings, inverseBindMatrices, *appearanceBinding, jointMatInput, name),
		[logicEngine, forceJointsCopy = joints, forceAppearanceBindingCopy = appearanceBinding](rlogic::SkinBinding* object) {
			destroyLogicObject(logicEngine, object);
		});

	if (object) {
		object->setUserId(objectID.first, objectID.second);
	}

	return object;
}

inline RamsesRenderGroupBinding ramsesRenderGroupBinding(rlogic::LogicEngine* logicEngine, RamsesRenderGroup renderGroup, const rlogic::RamsesRenderGroupBindingElements& elements, std::vector<RamsesRenderGroup> nestedGroups, const std::string& name, const std::pair<uint64_t, uint64_t>& objectID) {
	RamsesRenderGroupBinding binding{logicEngine->createRamsesRenderGroupBinding(**renderGroup, elements, name), 
		[logicEngine, forceRenderGroupCopy = renderGroup, forceNestedGroupCopy = nestedGroups](rlogic::RamsesRenderGroupBinding* binding) {
			destroyLogicObject(logicEngine, binding);
		}};

	if (binding) {
		binding->setUserId(objectID.first, objectID.second);
	}

	return binding;
}


};	// namespace raco::ramses_base
