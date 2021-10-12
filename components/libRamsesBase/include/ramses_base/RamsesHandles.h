/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "log_system/log.h"
#include <ramses-utils.h>

#include <memory>
#include <ramses-client-api/Appearance.h>
#include <ramses-client-api/ArrayResource.h>
#include <ramses-client-api/AttributeInput.h>
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
#include <ramses-client-api/UniformInput.h>

#include <ramses-logic/LogicEngine.h>
#include <ramses-logic/RamsesAppearanceBinding.h>
#include <ramses-logic/RamsesCameraBinding.h>
#include <ramses-logic/RamsesNodeBinding.h>

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

/** RESOURCE HANDLES */
using RamsesEffect = RamsesHandle<ramses::Effect>;
using RamsesArrayResource = RamsesHandle<ramses::ArrayResource>;
using RamsesTexture2D = RamsesHandle<ramses::Texture2D>;
using RamsesTextureCube = RamsesHandle<ramses::TextureCube>;

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

	void replaceTrackedSamplers(std::vector<raco::ramses_base::RamsesTextureSampler>& newSamplers) {
		trackedSamplers_ = newSamplers;
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

private:
	// The appearance is owned by this class.
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

using UniqueRamsesAppearanceBinding = std::unique_ptr<rlogic::RamsesAppearanceBinding, std::function<void(rlogic::RamsesAppearanceBinding*)>>;
using UniqueRamsesNodeBinding = std::unique_ptr<rlogic::RamsesNodeBinding, std::function<void(rlogic::RamsesNodeBinding*)>>;
using UniqueRamsesCameraBinding = std::unique_ptr<rlogic::RamsesCameraBinding, std::function<void(rlogic::RamsesCameraBinding*)>>;

inline UniqueRamsesAppearanceBinding ramsesAppearanceBinding(ramses::Appearance& appearance, rlogic::LogicEngine* logicEngine, const std::string& name) {
	return {logicEngine->createRamsesAppearanceBinding(appearance, name), [logicEngine](rlogic::RamsesAppearanceBinding* ptr) {
				logicEngine->destroy(*ptr);
			}};
}

inline UniqueRamsesNodeBinding ramsesNodeBinding(ramses::Node& node, rlogic::LogicEngine* logicEngine) {
	return {logicEngine->createRamsesNodeBinding(node, rlogic::ERotationType::Euler_ZYX), [logicEngine](rlogic::RamsesNodeBinding* binding) {
				logicEngine->destroy(*binding);
			}};
}

inline UniqueRamsesCameraBinding ramsesCameraBinding(ramses::Camera& camera, rlogic::LogicEngine* logicEngine) {
	return {logicEngine->createRamsesCameraBinding(camera), [logicEngine](rlogic::RamsesCameraBinding* binding) {
				logicEngine->destroy(*binding);
			}};
}

};	// namespace raco::ramses_base
