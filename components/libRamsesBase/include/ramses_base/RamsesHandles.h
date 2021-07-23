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
#include <ramses-client-api/RenderGroup.h>
#include <ramses-client-api/RenderPass.h>
#include <ramses-client-api/Scene.h>
#include <ramses-client-api/Texture2D.h>
#include <ramses-client-api/TextureCube.h>
#include <ramses-client-api/TextureSampler.h>
#include <ramses-client-api/UniformInput.h>

#include <ramses-logic/LogicEngine.h>
#include <ramses-logic/RamsesAppearanceBinding.h>
#include <ramses-logic/RamsesCameraBinding.h>
#include <ramses-logic/RamsesNodeBinding.h>

#include <stdexcept>

namespace raco::ramses_base {

using RamsesObjectDeleter = std::function<void(ramses::RamsesObject*)>;
template <typename T>
using RamsesHandle = std::shared_ptr<T>;
using RamsesObject = RamsesHandle<ramses::RamsesObject>;
using RamsesScene = RamsesHandle<ramses::Scene>;
using RamsesNode = RamsesHandle<ramses::Node>;
using RamsesMeshNode = RamsesHandle<ramses::MeshNode>;
using RamsesGeometryBinding = RamsesHandle<ramses::GeometryBinding>;
using RamsesRenderGroup = RamsesHandle<ramses::RenderGroup>;
using RamsesPerspectiveCamera = RamsesHandle<ramses::PerspectiveCamera>;
using RamsesOrthographicCamera = RamsesHandle<ramses::OrthographicCamera>;
using RamsesRenderPass = RamsesHandle<ramses::RenderPass>;
using RamsesTextureSampler = RamsesHandle<ramses::TextureSampler>;

/** RESOURCE HANDLES */
using RamsesEffect = RamsesHandle<ramses::Effect>;
using RamsesArrayResource = RamsesHandle<ramses::ArrayResource>;
using RamsesTexture2D = RamsesHandle<ramses::Texture2D>;
using RamsesTextureCube = RamsesHandle<ramses::TextureCube>;

template<typename OwnerType, typename RamsesType>
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

inline RamsesNode ramsesNode(ramses::Scene* scene) {
	return {scene->createNode(), createRamsesObjectDeleter<ramses::Node>(scene)};
}

inline RamsesMeshNode ramsesMeshNode(ramses::Scene* scene) {
	return {scene->createMeshNode(), createRamsesObjectDeleter<ramses::MeshNode>(scene)};
}

inline RamsesGeometryBinding ramsesGeometryBinding(ramses::Scene* scene, const ramses::Effect& effect) {
	return {scene->createGeometryBinding(effect), createRamsesObjectDeleter<ramses::GeometryBinding>(scene)};
}

inline RamsesOrthographicCamera ramsesOrthographicCamera(ramses::Scene* scene) {
	return {scene->createOrthographicCamera(), createRamsesObjectDeleter<ramses::OrthographicCamera>(scene)};
}

inline RamsesPerspectiveCamera ramsesPerspectiveCamera(ramses::Scene* scene) {
	return {scene->createPerspectiveCamera(), createRamsesObjectDeleter<ramses::PerspectiveCamera>(scene)};
}

inline RamsesRenderGroup ramsesRenderGroup(ramses::Scene* scene) {
	return {scene->createRenderGroup(), createRamsesObjectDeleter<ramses::RenderGroup>(scene)};
}

inline RamsesRenderPass ramsesRenderPass(ramses::Scene* scene) {
	return {scene->createRenderPass(), createRamsesObjectDeleter<ramses::RenderPass>(scene)};
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
	uint32_t width,
	uint32_t height,
	ramses::ETextureFormat format,
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
	return {logicEngine->createRamsesNodeBinding(node), [logicEngine](rlogic::RamsesNodeBinding* binding) {
				logicEngine->destroy(*binding);
			}};
}

inline UniqueRamsesCameraBinding ramsesCameraBinding(ramses::Camera& camera, rlogic::LogicEngine* logicEngine) {
	return {logicEngine->createRamsesCameraBinding(camera), [logicEngine](rlogic::RamsesCameraBinding* binding) {
				logicEngine->destroy(*binding);
			}};
}

};	// namespace raco::ramses_base
