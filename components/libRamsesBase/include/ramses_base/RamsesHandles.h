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

#include <ramses/client/ramses-utils.h>

#include <memory>
#include <ramses/client/Appearance.h>
#include <ramses/client/ArrayResource.h>
#include <ramses/client/AttributeInput.h>
#include <ramses/client/BlitPass.h>
#include <ramses/client/Camera.h>
#include <ramses/client/Effect.h>
#include <ramses/client/EffectDescription.h>
#include <ramses/client/Geometry.h>
#include <ramses/client/MeshNode.h>
#include <ramses/client/Node.h>
#include <ramses/client/OrthographicCamera.h>
#include <ramses/client/PerspectiveCamera.h>
#include <ramses/client/PickableObject.h>
#include <ramses/client/RamsesClient.h>
#include <ramses/client/RenderBuffer.h>
#include <ramses/client/RenderGroup.h>
#include <ramses/client/RenderPass.h>
#include <ramses/client/RenderTarget.h>
#include <ramses/client/RenderTargetDescription.h>
#include <ramses/client/Scene.h>
#include <ramses/client/Texture2D.h>
#include <ramses/client/TextureCube.h>
#include <ramses/client/TextureSampler.h>
#include <ramses/client/TextureSamplerMS.h>
#include <ramses/client/TextureSamplerExternal.h>
#include <ramses/client/UniformInput.h>

#include <ramses/client/logic/AnchorPoint.h>
#include <ramses/client/logic/AnimationNode.h>
#include <ramses/client/logic/DataArray.h>
#include <ramses/client/logic/LogicEngine.h>
#include <ramses/client/logic/LuaScript.h>
#include <ramses/client/logic/LuaInterface.h>
#include <ramses/client/logic/LuaModule.h>
#include <ramses/client/logic/TimerNode.h>
#include <ramses/client/logic/AppearanceBinding.h>
#include <ramses/client/logic/CameraBinding.h>
#include <ramses/client/logic/MeshNodeBinding.h>
#include <ramses/client/logic/NodeBinding.h>
#include <ramses/client/logic/RenderBufferBinding.h>
#include <ramses/client/logic/RenderPassBinding.h>
#include <ramses/client/logic/RenderGroupBinding.h>
#include <ramses/client/logic/RenderGroupBindingElements.h>
#include <ramses/client/logic/AnimationNodeConfig.h>
#include <ramses/client/logic/SkinBinding.h>

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
using RamsesTimerNode = RamsesHandle<ramses::TimerNode>;
using RamsesLuaModule = RamsesHandle<ramses::LuaModule>;
using RamsesLuaScript = RamsesHandle<ramses::LuaScript>;
using RamsesLuaInterface = RamsesHandle<ramses::LuaInterface>;
using RamsesAnchorPoint = RamsesHandle<ramses::AnchorPoint>;
using RamsesSkinBinding = RamsesHandle<ramses::SkinBinding>;
using RamsesPickableObject = RamsesHandle<ramses::PickableObject>;

	/** RESOURCE HANDLES */
using RamsesEffect = RamsesHandle<ramses::Effect>;
using RamsesArrayResource = RamsesHandle<ramses::ArrayResource>;
using RamsesArrayBuffer = RamsesHandle<ramses::ArrayBuffer>;
using RamsesTexture2D = RamsesHandle<ramses::Texture2D>;
using RamsesTextureCube = RamsesHandle<ramses::TextureCube>;
using RamsesBlitPass = RamsesHandle<ramses::BlitPass>;

template <typename RamsesType>
void destroyRamsesObject(ramses::Scene* owner, RamsesType* obj) {
	if (obj) {
		if (!owner->destroy(*obj)) {
			auto error = owner->getRamsesClient().getRamsesFramework().getLastError().value();
			LOG_ERROR(log_system::RAMSES_BACKEND, "Deleting Ramses object failed: {}", error.message);
			assert(false);
			exit(1);
		}
	}
}

template <typename RamsesType>
constexpr RamsesObjectDeleter createRamsesObjectDeleter(ramses::Scene* owner) {
	return [owner](ramses::RamsesObject* obj) {
		destroyRamsesObject(owner, static_cast<RamsesType*>(obj));
	};
}

template <typename LogicType>
void destroyLogicObject(ramses::LogicEngine* logicengine, LogicType* obj) {
	if (obj) {
		auto status = logicengine->destroy(*obj);
		if (!status) {
			auto error = logicengine->getScene().getRamsesClient().getRamsesFramework().getLastError().value();
			LOG_ERROR(log_system::RAMSES_BACKEND, "Deleting LogicEngine object failed: {}", error.message);
			assert(false);
			exit(1);
		}
	}
}


struct RamsesAppearanceHandle {
	RamsesAppearanceHandle(ramses::Scene* scene, ramses_base::RamsesEffect effect, const std::pair<uint64_t, uint64_t>& objectID)
		: appearance_(scene->createAppearance(*effect)), scene_(scene), trackedEffect_(effect) {
		if (appearance_) {
			appearance_->setUserId(objectID.first, objectID.second);
		}
	}

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

	void replaceTrackedSamplers(std::vector<ramses_base::RamsesTextureSampler>& newSamplers, std::vector<ramses_base::RamsesTextureSamplerMS>& newSamplersMS, std::vector<ramses_base::RamsesTextureSamplerExternal>& newSamplersExternal) {
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
	std::vector<ramses_base::RamsesTextureSampler> trackedSamplers_;
	std::vector<ramses_base::RamsesTextureSamplerMS> trackedSamplersMS_;
	std::vector<ramses_base::RamsesTextureSamplerExternal> trackedSamplersExternal_;
};

using RamsesAppearance = std::shared_ptr<RamsesAppearanceHandle>;

inline RamsesAppearance ramsesAppearance(ramses::Scene* scene, ramses_base::RamsesEffect effect, const std::pair<uint64_t, uint64_t>& objectID) {
	return std::make_shared<RamsesAppearanceHandle>(scene, effect, objectID);
}

inline RamsesScene ramsesScene(ramses::sceneId_t id, ramses::RamsesClient* client) {
	return {
		client->createScene(ramses::SceneConfig(id, ramses::EScenePublicationMode::LocalAndRemote)),
	
		[client](ramses::Scene* obj) {
			if (obj) {
				if (!client->destroy(*obj)) {
					auto error = client->getRamsesFramework().getLastError().value();
					LOG_ERROR(log_system::RAMSES_BACKEND, "Deleting Ramses object failed: {}", error.message);
					assert(false);
					exit(1);
				}
			}
		}};
}

struct RamsesGeometryHandle {
	RamsesGeometryHandle(ramses::Scene* scene, ramses_base::RamsesEffect effect, const std::pair<uint64_t, uint64_t>& objectID)
		: geometry_(scene->createGeometry(*effect)), scene_(scene), trackedEffect_(effect) {
		if (geometry_) {
			geometry_->setUserId(objectID.first, objectID.second);
		}
	}

	~RamsesGeometryHandle() {
		destroyRamsesObject(scene_, geometry_);
	}

	ramses::Geometry& operator*() {
		return *geometry_;
	}

	ramses::Geometry* operator->() {
		return geometry_;
	}

	void setIndices(RamsesArrayResource indexBuffer) {
		if (geometry_->setIndices(*indexBuffer)) {
			trackedMeshIndices_ = indexBuffer;
		} else {
			auto error = scene_->getRamsesClient().getRamsesFramework().getLastError().value();
			LOG_ERROR(log_system::RAMSES_ADAPTOR, error.message);
		}
	}

	void addAttributeBuffer(const ramses::AttributeInput& attribInput, RamsesArrayResource attributeBuffer) {
		if (geometry_->setInputBuffer(attribInput, *attributeBuffer)) {
			trackedMeshVertexData_.emplace_back(attributeBuffer);
		} else {
			auto error = scene_->getRamsesClient().getRamsesFramework().getLastError().value();
			LOG_ERROR(log_system::RAMSES_ADAPTOR, error.message);
		}
	}

private:
	// The geometry_ is owned by this class.
	ramses::Geometry* geometry_;

	// Kept for reference; needed to destroy the Geometry pointer in ramses.
	ramses::Scene* scene_;

	// Effect currently used by the geometry_. Needed to keep the effect alive in ramses.
	RamsesEffect trackedEffect_;

	// Attribute buffers currently used by the geometry_. Needed to keep the buffers alive in ramses.
	std::vector<ramses_base::RamsesArrayResource> trackedMeshVertexData_;
	
	// Index buffer currently used by the geometry_. Needed to keep the buffer alive in ramses.
	ramses_base::RamsesArrayResource trackedMeshIndices_;
};

using RamsesGeometry = std::shared_ptr<RamsesGeometryHandle>;

inline RamsesGeometry ramsesGeometry(ramses::Scene* scene, ramses_base::RamsesEffect effect, const std::pair<uint64_t, uint64_t>& objectID) {
	return std::make_shared<RamsesGeometryHandle>(scene, effect, objectID);
}

template<typename RamsesType, auto PToM>
struct RamsesWrapper {
	RamsesWrapper(ramses::Scene* scene, const std::pair<uint64_t, uint64_t>& objectID)
		: ramsesObject_((scene->*PToM)({})), scene_(scene) {
		if (ramsesObject_) {
			ramsesObject_->setUserId(objectID.first, objectID.second);
		}
	}

	~RamsesWrapper() {
		destroyRamsesObject(scene_, ramsesObject_);
	}

	std::string_view getName() const {
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

inline RamsesNode ramsesNode(ramses::Scene* scene, const std::pair<uint64_t, uint64_t>& objectID) {
	return std::make_shared<RamsesNodeHandle>(scene, objectID);
}

using RamsesOrthographicCameraHandle = RamsesWrapper<ramses::OrthographicCamera, &ramses::Scene::createOrthographicCamera>;
using RamsesOrthographicCamera = std::shared_ptr<RamsesOrthographicCameraHandle>;

inline RamsesOrthographicCamera ramsesOrthographicCamera(ramses::Scene* scene, const std::pair<uint64_t, uint64_t>& objectID) {
	return std::make_shared<RamsesOrthographicCameraHandle>(scene, objectID);
}

using RamsesPerspectiveCameraHandle = RamsesWrapper<ramses::PerspectiveCamera, &ramses::Scene::createPerspectiveCamera>;
using RamsesPerspectiveCamera = std::shared_ptr<RamsesPerspectiveCameraHandle>;

inline RamsesPerspectiveCamera ramsesPerspectiveCamera(ramses::Scene* scene, const std::pair<uint64_t, uint64_t>& objectID) {
	return std::make_shared<RamsesPerspectiveCameraHandle>(scene, objectID);
}


struct RamsesMeshNodeHandle {
	RamsesMeshNodeHandle(ramses::Scene* scene, const std::pair<uint64_t, uint64_t>& objectID) 
	: meshnode_(scene->createMeshNode()), scene_(scene) {
		if (meshnode_) {
			meshnode_->setUserId(objectID.first, objectID.second);
		}
	}
	
	~RamsesMeshNodeHandle() {
		destroyRamsesObject(scene_, meshnode_);
	}

	std::string_view getName() const {
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
		trackedGeometry_.reset();
	}

	void setAppearance(RamsesAppearance appearance) {
		meshnode_->setAppearance(**appearance);
		trackedAppearance_ = appearance;
	}

	void setGeometry(RamsesGeometry geometry) {
		meshnode_->setGeometry(**geometry);
		trackedGeometry_ = geometry;
	}

private:
	// The meshnode is owned by this class.
	ramses::MeshNode* meshnode_;

	// Kept for reference; needed to destroy the meshnode pointer in ramses.
	ramses::Scene* scene_;

	// Appearance currently used by the meshnode_. Needed to keep the Appearance alive in ramses.
	RamsesAppearance trackedAppearance_;

	// Geometry currently used by the meshnode_. Needed to keep the Geometry alive in ramses.
	RamsesGeometry trackedGeometry_;
};

using RamsesMeshNode = std::shared_ptr<RamsesMeshNodeHandle>;

inline RamsesMeshNode ramsesMeshNode(ramses::Scene* scene, const std::pair<uint64_t, uint64_t>& objectID) {
	return std::make_shared<RamsesMeshNodeHandle>(scene, objectID);
}

struct RamsesRenderGroupHandle;
using RamsesRenderGroup = std::shared_ptr<RamsesRenderGroupHandle>;

struct RamsesRenderGroupHandle {
	RamsesRenderGroupHandle(ramses::Scene* scene, const std::pair<uint64_t, uint64_t>& objectID)
		: renderGroup_(scene->createRenderGroup()), scene_(scene) {
		if (renderGroup_) {
			renderGroup_->setUserId(objectID.first, objectID.second);
		}
	}

	~RamsesRenderGroupHandle() {
		destroyRamsesObject(scene_, renderGroup_);
	}

	std::string_view getName() const {
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

inline RamsesRenderGroup ramsesRenderGroup(ramses::Scene* scene, const std::pair<uint64_t, uint64_t>& objectID) {
	return std::make_shared<RamsesRenderGroupHandle>(scene, objectID);
}

inline RamsesRenderBuffer ramsesRenderBuffer(ramses::Scene* scene,
	uint32_t width, uint32_t height, ramses::ERenderBufferFormat format, ramses::ERenderBufferAccessMode accessMode, uint32_t sampleCount, std::string_view name, const std::pair<uint64_t, uint64_t>& objectID) {
	auto buffer{scene->createRenderBuffer(width, height, format, accessMode, sampleCount, name)};
	if (buffer) {
		buffer->setUserId(objectID.first, objectID.second);
	} else {
		LOG_ERROR(log_system::RAMSES_ADAPTOR, "Ramses RenderBuffer creation for width = {}, height = {}, format = {}, accessMode = {}, sampleCount = {}, name = '{}' failed.", width, height, accessMode, sampleCount, name);
	}

	return {buffer, createRamsesObjectDeleter<ramses::RenderBuffer>(scene)};
}

struct RamsesRenderPassHandle {
	RamsesRenderPassHandle(std::shared_ptr<ramses::RenderPass> renderPass, const std::pair<uint64_t, uint64_t>& objectID)
		: renderPass_(renderPass) {
		if (renderPass_) {
			renderPass_->setUserId(objectID.first, objectID.second);
		}
	}

	std::string_view getName() const {
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

	void addRenderGroup(RamsesRenderGroup renderGroup, int32_t order = 0) {
		renderPass_->addRenderGroup(**renderGroup, order);
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
inline std::shared_ptr<ramses::RenderPass> ramsesRenderPassInternal(ramses::Scene* scene, CameraHandleType camera, RamsesRenderTarget target, std::string_view name) {
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
inline RamsesRenderPass ramsesRenderPass(ramses::Scene* scene, CameraHandleType camera, RamsesRenderTarget target, std::string_view name, const std::pair<uint64_t, uint64_t>& objectID) {
	if (!camera) {
		return {};
	}
	return std::make_shared<RamsesRenderPassHandle>(ramsesRenderPassInternal(scene, camera, target, name), objectID);
}

inline RamsesRenderTarget ramsesRenderTarget(ramses::Scene* scene, const ramses::RenderTargetDescription& rtDesc, const std::vector<RamsesRenderBuffer>& buffers, const std::pair<uint64_t, uint64_t>& objectID) {
	if (buffers.empty()) {
		return nullptr;
	}

	auto target{scene->createRenderTarget(rtDesc)};
	if (target) {
		target->setUserId(objectID.first, objectID.second);
	}
	return {
		target,
		[scene, forceCopy = buffers](ramses::RamsesObject* obj) {
			destroyRamsesObject(scene, static_cast<ramses::RenderTarget*>(obj));
		}};
}

template<typename T>
inline RamsesArrayResource ramsesArrayResource(ramses::Scene* scene, const std::vector<T>& data, std::string_view name = {}) {
	return {
		scene->createArrayResource<T>(data.size(), data.data(), name),
		createRamsesObjectDeleter<ramses::ArrayResource>(scene)};
}

template <typename T>
inline RamsesArrayResource ramsesArrayResource(ramses::Scene* scene, uint32_t numElements, const T* arrayData, std::string_view name = {}) {
	return {
		scene->createArrayResource(numElements, arrayData, name),
		createRamsesObjectDeleter<ramses::ArrayResource>(scene)};
}

template<typename T>
inline RamsesArrayBuffer ramsesArrayBuffer(ramses::Scene* scene, ramses::EDataType type, uint32_t numElements, const T* arrayData, std::string_view name = {}) {
	RamsesArrayBuffer result{scene->createArrayBuffer(type, numElements), createRamsesObjectDeleter<ramses::ArrayResource>(scene)};
	auto status = result->setName(name);
	status = result->updateData(0, numElements, arrayData);
	return result;
}

template<typename CameraHandleType>
inline RamsesPickableObject ramsesPickableObject(ramses::Scene* scene, RamsesArrayBuffer geometryBuffer, ramses::pickableObjectId_t pickId, CameraHandleType camera) {
	if (!camera) {
		return {};
	}

	RamsesPickableObject object{
		scene->createPickableObject(*geometryBuffer.get(), pickId),
		[scene, forceBufferCopy = geometryBuffer, forceCameraCopy = camera](ramses::RamsesObject* object) {
			destroyRamsesObject(scene, static_cast<ramses::PickableObject*>(object));
		}};

	if (object) {
		object->setCamera(**camera);
	}
	return object;
}


template <typename RamsesTexture>
inline RamsesTextureSampler ramsesTextureSampler(ramses::Scene* scene, ramses::ETextureAddressMode wrapUMode, ramses::ETextureAddressMode wrapVMode, ramses::ETextureSamplingMethod minSamplingMethod, ramses::ETextureSamplingMethod magSamplingMethod, RamsesTexture texture, uint32_t anisotropyLevel, std::string_view name, const std::pair<uint64_t, uint64_t>& objectID) {
	auto sampler = scene->createTextureSampler(wrapUMode, wrapVMode, minSamplingMethod, magSamplingMethod, *texture, anisotropyLevel, name);
	if (sampler) {
		sampler->setUserId(objectID.first, objectID.second);
	} else {
		LOG_ERROR(log_system::RAMSES_ADAPTOR, "Ramses TextureSampler creation for name = '{}' failed.", name);
	}

	return {sampler,
		[scene, forceCopy = texture](ramses::RamsesObject* obj) {
			if (!scene->destroy(*static_cast<ramses::TextureSampler*>(obj))) {
				auto error = scene->getRamsesClient().getRamsesFramework().getLastError().value();
				throw std::runtime_error(error.message);
			}
		}};
}

inline RamsesTextureSamplerMS ramsesTextureSamplerMS(ramses::Scene* scene, RamsesRenderBuffer buffer, std::string_view name, const std::pair<uint64_t, uint64_t>& objectID) {
	auto sampler = scene->createTextureSamplerMS(*buffer, name);
	if (sampler) {
		sampler->setUserId(objectID.first, objectID.second);
	} else {
		LOG_ERROR(log_system::RAMSES_ADAPTOR, "Ramses TextureSamplerMS creation for name = '{}' failed.", name);
	}

	return {
		sampler,
		[scene, forceCopy = buffer](ramses::RamsesObject* buffer) {
			destroyRamsesObject(scene, static_cast<ramses::RenderBuffer*>(buffer));
		}};
}

inline RamsesTextureSamplerExternal ramsesTextureSamplerExternal(ramses::Scene* scene, ramses::ETextureSamplingMethod minSamplingMethod, ramses::ETextureSamplingMethod magSamplingMethod, std::string_view name, const std::pair<uint64_t, uint64_t>& objectID) {
	auto sampler{scene->createTextureSamplerExternal(minSamplingMethod, magSamplingMethod, name)};
	if (sampler) {
		sampler->setUserId(objectID.first, objectID.second);
	}

	return {
		sampler,
		createRamsesObjectDeleter<ramses::TextureSamplerExternal>(scene)
	};
}

/** RESOURCE FACTORIES */

inline RamsesEffect ramsesEffect(ramses::Scene* scene, const ramses::EffectDescription& description, std::string_view name, const std::pair<uint64_t, uint64_t>& objectID) {
	auto effect{scene->createEffect(description, name)};
	if (effect) {
		effect->setUserId(objectID.first, objectID.second);
	}

	return {effect, createRamsesObjectDeleter<ramses::Effect>(scene)};
}

inline RamsesTexture2D ramsesTexture2D(ramses::Scene* scene,
	ramses::ETextureFormat format,
	uint32_t width,
	uint32_t height,
	const std::vector<ramses::MipLevelData>& mipLevelData,
	bool generateMipChain,
	const ramses::TextureSwizzle& swizzle,
	std::string_view name,
	const std::pair<uint64_t, uint64_t>& objectID) {
	auto texture{scene->createTexture2D(format, width, height, mipLevelData, generateMipChain, swizzle, name)};
	if (texture) {
		texture->setUserId(objectID.first, objectID.second);
	} else {
		LOG_ERROR(log_system::RAMSES_ADAPTOR, "Ramses TextureSampler2D creation for width = {}, height = {}, format = {}, name = '{}' failed.", width, height, format, name);
	}

	return {texture, createRamsesObjectDeleter<ramses::Texture2D>(scene)};
}

inline RamsesTexture2D ramsesTexture2DFromPng(ramses::Scene* scene, const std::string& uri, const std::pair<uint64_t, uint64_t>& objectID) {
	auto texture{ramses::RamsesUtils::CreateTextureResourceFromPng(uri.c_str(), *scene)};
	if (texture) {
		texture->setUserId(objectID.first, objectID.second);
	}
	return {texture, createRamsesObjectDeleter<ramses::Texture2D>(scene)};
}

inline RamsesTexture2D ramsesTexture2DFromPngBuffer(ramses::Scene* scene, const std::vector<unsigned char>& pngData, const std::pair<uint64_t, uint64_t>& objectID) {
	auto texture{ramses::RamsesUtils::CreateTextureResourceFromPngBuffer(pngData, *scene)};
	if (texture) {
		texture->setUserId(objectID.first, objectID.second);
	}
	return {texture, createRamsesObjectDeleter<ramses::Texture2D>(scene)};
}

inline RamsesTextureCube ramsesTextureCube(ramses::Scene* scene,
	ramses::ETextureFormat format,
	uint32_t width,
	const std::vector<ramses::CubeMipLevelData>& mipLevelData,
	bool generateMipChain,
	const ramses::TextureSwizzle& swizzle,
	std::string_view name,
	const std::pair<uint64_t, uint64_t>& objectID) {
	auto texture{scene->createTextureCube(format, width, mipLevelData, generateMipChain, swizzle, name)};
	if (texture) {
		texture->setUserId(objectID.first, objectID.second);
	}
	return {texture, createRamsesObjectDeleter<ramses::TextureCube>(scene)};
}

inline RamsesBlitPass ramsesBlitPass(ramses::Scene* scene, RamsesRenderBuffer source, RamsesRenderBuffer dest, std::string_view name, const std::pair<uint64_t, uint64_t>& objectID) {
	auto blitpass{scene->createBlitPass(*source, *dest, name)};
	if (blitpass) {
		blitpass->setUserId(objectID.first, objectID.second);
	}
	return {
		blitpass,
		[scene, forceSourceCopy = source, forceDestCopy = dest](ramses::RamsesObject* object) {
			destroyRamsesObject(scene, static_cast<ramses::BlitPass*>(object));
		}};
}

using RamsesAppearanceBinding = std::shared_ptr<ramses::AppearanceBinding>;
using UniqueRamsesDataArray = std::unique_ptr<ramses::DataArray, std::function<void(ramses::DataArray*)>>;
using RamsesMeshNodeBinding = std::shared_ptr<ramses::MeshNodeBinding>;
using RamsesNodeBinding = std::shared_ptr<ramses::NodeBinding>;
using RamsesCameraBinding = std::shared_ptr<ramses::CameraBinding>;
using UniqueRamsesRenderPassBinding = std::unique_ptr<ramses::RenderPassBinding, std::function<void(ramses::RenderPassBinding*)>>;
using RamsesRenderGroupBinding = std::shared_ptr<ramses::RenderGroupBinding>;
using RamsesRenderBufferBinding = std::shared_ptr<ramses::RenderBufferBinding>;

inline RamsesAppearanceBinding ramsesAppearanceBinding(ramses::Appearance& appearance, ramses::LogicEngine* logicEngine, const std::string& name, const std::pair<uint64_t, uint64_t> &objectID) {
	RamsesAppearanceBinding binding{logicEngine->createAppearanceBinding(appearance, name),
		[logicEngine](ramses::AppearanceBinding* binding) {
			destroyLogicObject(logicEngine, binding);
		}};

	if (binding) {
		binding->setUserId(objectID.first, objectID.second);
	}

	return binding;
}

template<typename NodeHandleType>
inline RamsesNodeBinding ramsesNodeBinding(NodeHandleType node, ramses::LogicEngine* logicEngine, ramses::ERotationType rotationType, const std::pair<uint64_t, uint64_t>& objectID) {
	RamsesNodeBinding binding{logicEngine->createNodeBinding(**node, rotationType), 
		[logicEngine, forceNodeCopy = node](ramses::NodeBinding* binding) {
				destroyLogicObject(logicEngine, binding);
			}};

	if (binding) {
		binding->setUserId(objectID.first, objectID.second);
	}

	return binding;
}

inline RamsesMeshNodeBinding ramsesMeshNodeBinding(RamsesMeshNode meshnode, ramses::LogicEngine* logicEngine, const std::string& name, const std::pair<uint64_t, uint64_t>& objectID) {
	RamsesMeshNodeBinding binding{logicEngine->createMeshNodeBinding(**meshnode, name),
		[logicEngine, forceNodeCopy = meshnode](ramses::MeshNodeBinding* binding) {
			destroyLogicObject(logicEngine, binding);
		}};

	if (binding) {
		binding->setUserId(objectID.first, objectID.second);
	}

	return binding;
}

template <typename T>
inline UniqueRamsesDataArray ramsesDataArray(const std::vector<T>& vec, ramses::LogicEngine* logicEngine, const std::string& name, const std::pair<uint64_t, uint64_t>& objectID) {
	UniqueRamsesDataArray array{logicEngine->createDataArray(vec, name), [logicEngine](ramses::DataArray* array) {
				destroyLogicObject(logicEngine, array);
			}};

	if (array) {
		array->setUserId(objectID.first, objectID.second);
	}

	return array;
}

template <typename CameraHandleType>
inline RamsesCameraBinding ramsesCameraBinding(CameraHandleType camera, ramses::LogicEngine* logicEngine, const std::pair<uint64_t, uint64_t>& objectID, bool frustumPlanes) {
	RamsesCameraBinding binding{
		frustumPlanes ? logicEngine->createCameraBindingWithFrustumPlanes(**camera) : logicEngine->createCameraBinding(**camera), 
		[logicEngine, forceCameraCopy = camera](ramses::CameraBinding* binding) {
				destroyLogicObject(logicEngine, binding);
			}};

	if (binding) {
		binding->setUserId(objectID.first, objectID.second);
	}

	return binding;
}

inline UniqueRamsesRenderPassBinding ramsesRenderPassBinding(ramses::RenderPass& renderpass, ramses::LogicEngine* logicEngine, const std::string& name, const std::pair<uint64_t, uint64_t>& objectID) {
	UniqueRamsesRenderPassBinding binding{logicEngine->createRenderPassBinding(renderpass, name), [logicEngine](ramses::RenderPassBinding* binding) {
										  destroyLogicObject(logicEngine, binding);
									  }};

	if (binding) {
		binding->setUserId(objectID.first, objectID.second);
	}

	return binding;
}


inline RamsesTimerNode ramsesTimer(ramses::LogicEngine* logicEngine, const std::string& name, const std::pair<uint64_t, uint64_t>& objectID) {
	RamsesTimerNode node{logicEngine->createTimerNode(name), [logicEngine](ramses::TimerNode* timer) {
				destroyLogicObject(logicEngine, timer);
			}};

	if (node) {
		node->setUserId(objectID.first, objectID.second);
	}

	return node;
}

inline RamsesLuaModule ramsesLuaModule(const std::string& luaContent, ramses::LogicEngine* logicEngine, ramses::LuaConfig& config, const std::string& name, const std::pair<uint64_t, uint64_t>& objectID) {

	RamsesLuaModule module{
		logicEngine->createLuaModule(luaContent, config, name), [logicEngine](ramses::LuaModule* module) {
			destroyLogicObject(logicEngine, module);
		}};

	if (module) {
		module->setUserId(objectID.first, objectID.second);
	}

	return module;
}

inline RamsesLuaScript ramsesLuaScript(ramses::LogicEngine* logicEngine, const std::string& scriptText, ramses::LuaConfig& config, std::vector<ramses_base::RamsesLuaModule> modules, const std::string& name, const std::pair<uint64_t, uint64_t>& objectID) {
	RamsesLuaScript script{
		logicEngine->createLuaScript(scriptText, config, name), 
		[logicEngine, forceCopy = modules](ramses::LuaScript* script) {
			destroyLogicObject(logicEngine, script);
		}};

	if (script) {
		script->setUserId(objectID.first, objectID.second);
	}

	return script;
}

/// Note that caller must supply modules if interface text contains modules() statement
inline RamsesLuaInterface ramsesLuaInterface(ramses::LogicEngine* logicEngine, const std::string& interfaceText, ramses::LuaConfig& config, std::vector<ramses_base::RamsesLuaModule> modules, const std::string& name, const std::pair<uint64_t, uint64_t>& objectID) {
	RamsesLuaInterface interface{
		logicEngine->createLuaInterface(interfaceText, name, config),
		[logicEngine, forceCopy = modules](ramses::LuaInterface* interface) {
			destroyLogicObject(logicEngine, interface);
		}};

	if (interface) {
		interface->setUserId(objectID.first, objectID.second);
	}
	return interface;
}

struct RamsesAnimationChannelData {
	std::string name;
	ramses::EInterpolationType interpolationType;
	UniqueRamsesDataArray keyframeTimes;
	UniqueRamsesDataArray animOutput;
	UniqueRamsesDataArray tangentIn;
	UniqueRamsesDataArray tangentOut;
};

using RamsesAnimationChannelHandle = RamsesHandle<RamsesAnimationChannelData>;

struct RamsesAnimationNodeHandle {
	RamsesAnimationNodeHandle(ramses::LogicEngine* logicEngine, ramses::AnimationNode* ramsesAnimationNode, std::vector<RamsesAnimationChannelHandle> channelHandles, const std::pair<uint64_t, uint64_t>& objectID) 
	: logicEngine_(logicEngine), animationNode_(ramsesAnimationNode), trackedAnimationChannels_(channelHandles) {
		if (animationNode_) {
			animationNode_->setUserId(objectID.first, objectID.second);
		}
	}

	~RamsesAnimationNodeHandle() {
		destroyLogicObject(logicEngine_, animationNode_);
	}

	ramses::AnimationNode& operator*() {
		return *animationNode_;
	}

	ramses::AnimationNode* operator->() {
		return animationNode_;
	}

	ramses::AnimationNode* get() {
		return animationNode_;
	}

	const std::vector<RamsesAnimationChannelHandle>& channels() const {
		return trackedAnimationChannels_;
	}

private:
	// Kept for reference. Needed to destroy the animation node.
	ramses::LogicEngine* logicEngine_;

	// The animation node is owned by this class.
	ramses::AnimationNode* animationNode_;

	// AnimationChannels currently used by the animation node. Needed to keep the effect alive in ramses.
	// May contain nullptrs;
	std::vector<RamsesAnimationChannelHandle> trackedAnimationChannels_;
};

using RamsesAnimationNode = std::shared_ptr<RamsesAnimationNodeHandle>;


inline RamsesAnimationNode ramsesAnimationNode(ramses::LogicEngine* logicEngine, const ramses::AnimationNodeConfig &config, 
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



inline RamsesAnchorPoint ramsesAnchorPoint(ramses::LogicEngine* logicEngine, RamsesNodeBinding nodeBinding, RamsesCameraBinding cameraBinding, const std::string& name, const std::pair<uint64_t, uint64_t>& objectID) {
	RamsesAnchorPoint object(
		logicEngine->createAnchorPoint(*nodeBinding, *cameraBinding, name),
			[logicEngine, forceNodeBindingCopy = nodeBinding, forceCameraBdingingCopy = cameraBinding](ramses::AnchorPoint* object) {
			destroyLogicObject(logicEngine, object);
		});

	if (object) {
		object->setUserId(objectID.first, objectID.second);
	}

	return object;
}

inline RamsesSkinBinding ramsesSkinBinding(ramses::LogicEngine* logicEngine, 
	std::vector<RamsesNodeBinding> joints,
	std::vector<glm::mat4x4>& inverseBindMatrices,
	RamsesAppearanceBinding& appearanceBinding,
	ramses::UniformInput& jointMatInput,
	const std::string& name, const std::pair<uint64_t, uint64_t>& objectID) {

	std::vector<const ramses::NodeBinding*> nodeBindings;
	for (const auto& binding : joints) {
		nodeBindings.emplace_back(binding.get());
	}

	RamsesSkinBinding object(logicEngine->createSkinBinding(nodeBindings, inverseBindMatrices, *appearanceBinding, jointMatInput, name),
		[logicEngine, forceJointsCopy = joints, forceAppearanceBindingCopy = appearanceBinding](ramses::SkinBinding* object) {
			destroyLogicObject(logicEngine, object);
		});

	if (object) {
		object->setUserId(objectID.first, objectID.second);
	}

	return object;
}

inline RamsesRenderGroupBinding ramsesRenderGroupBinding(ramses::LogicEngine* logicEngine, RamsesRenderGroup renderGroup, const ramses::RenderGroupBindingElements& elements, std::vector<RamsesRenderGroup> nestedGroups, const std::string& name, const std::pair<uint64_t, uint64_t>& objectID) {
	RamsesRenderGroupBinding binding{logicEngine->createRenderGroupBinding(**renderGroup, elements, name), 
		[logicEngine, forceRenderGroupCopy = renderGroup, forceNestedGroupCopy = nestedGroups](ramses::RenderGroupBinding* binding) {
			destroyLogicObject(logicEngine, binding);
		}};

	if (binding) {
		binding->setUserId(objectID.first, objectID.second);
	}

	return binding;
}

inline RamsesRenderBufferBinding ramsesRenderBufferBinding(RamsesRenderBuffer buffer, ramses::LogicEngine* logicEngine, const std::string& name, const std::pair<uint64_t, uint64_t>& objectID) {
	RamsesRenderBufferBinding binding{logicEngine->createRenderBufferBinding(*buffer, name),
		[logicEngine, forceNodeCopy = buffer](ramses::RenderBufferBinding* binding) {
			destroyLogicObject(logicEngine, binding);
		}};

	if (binding) {
		binding->setUserId(objectID.first, objectID.second);
	}

	return binding;
}


};	// namespace raco::ramses_base
