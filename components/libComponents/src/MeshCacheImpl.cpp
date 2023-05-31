/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "components/MeshCacheImpl.h"

#include "components/FileChangeListenerImpl.h"
#include "components/FileChangeMonitorImpl.h"
#include "core/Context.h"

#include "mesh_loader/CTMFileLoader.h"
#include "mesh_loader/glTFFileLoader.h"

#include <filesystem>
#include <memory>

namespace raco::components {

void MeshCacheImpl::unregister(std::string absPath, typename core::MeshCache::Callback *listener) {
	GenericFileChangeMonitorImpl<core::MeshCache>::unregister(absPath, listener);
	if (callbacks_.find(absPath) == callbacks_.end()) {
		meshCacheEntries_.erase(absPath);
	}
}

void MeshCacheImpl::notify(const std::string &absPath) {
	forceReloadCachedMesh(absPath);

	auto it = callbacks_.find(absPath);
	if (it != callbacks_.end()) {
		// Make a copy of the callbacks and check if each callback is still registered before invoking it
		// to allow removing watched paths from the EditorObject::updateFromExternalFile functions.
		auto callbacksCopy{it->second};
		for (auto callback : callbacksCopy) {
			if (it->second.find(callback) != it->second.end()) {
				if (callback->object() && callback->context()) {
					callback->object()->updateFromExternalFile(*callback->context());
					callback->context()->callReferencedObjectChangedHandlers(callback->object());
				}
			}
		}
	}
}

raco::core::SharedMeshData MeshCacheImpl::loadMesh(const raco::core::MeshDescriptor &descriptor) {
	auto *loader = getLoader(descriptor.absPath);
	return loader->loadMesh(descriptor);
}

const raco::core::MeshScenegraph *raco::components::MeshCacheImpl::getMeshScenegraph(const std::string &absPath) {
	auto *loader = getLoader(absPath);
	return loader->getScenegraph(absPath);
}

std::string raco::components::MeshCacheImpl::getMeshError(const std::string &absPath) {
	auto *loader = getLoader(absPath);
	return loader->getError();
}

int raco::components::MeshCacheImpl::getTotalMeshCount(const std::string &absPath) {
	auto *loader = getLoader(absPath);
	return loader->getTotalMeshCount();
}

raco::core::SharedAnimationSamplerData MeshCacheImpl::getAnimationSamplerData(const std::string &absPath, int animIndex, int samplerIndex) {
	auto *loader = getLoader(absPath);
	return loader->getAnimationSamplerData(absPath, animIndex, samplerIndex);
}

core::SharedSkinData MeshCacheImpl::loadSkin(const std::string &absPath, int skinIndex, std::string &outError) {
	auto *loader = getLoader(absPath);
	return loader->loadSkin(absPath, skinIndex, outError);
}

void MeshCacheImpl::forceReloadCachedMesh(const std::string &absPath) {
	auto *loader = getLoader(absPath);
	loader->reset();
}

bool endsWith(std::string const &text, std::string const &ending) {
	if (text.length() < ending.length()) return false;
	const auto startPos = text.length() - ending.length();

	return 0 == text.compare(startPos, ending.length(), ending);
}

raco::core::MeshCacheEntry *MeshCacheImpl::getLoader(std::string absPath) {
	// To prevent cache corpses which are not updated by file change listeners we require to call registerFileChangeHandler
	// before attempting to load a file:
	assert(callbacks_.find(absPath) != callbacks_.end());
	if (meshCacheEntries_.count(absPath) == 0) {
		if (endsWith(absPath, ".gltf") || endsWith(absPath, ".glb")) {
			meshCacheEntries_[absPath] = std::unique_ptr<raco::core::MeshCacheEntry>(new mesh_loader::glTFFileLoader(absPath));

		} else {
			meshCacheEntries_[absPath] = std::unique_ptr<raco::core::MeshCacheEntry>(new mesh_loader::CTMFileLoader(absPath));
		}
	}
	return meshCacheEntries_[absPath].get();
}

}  // namespace raco::components