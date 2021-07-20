/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "components/MeshCacheImpl.h"

#include "core/Context.h"
#include "components/FileChangeListenerImpl.h"
#include "log_system/log.h"
#include "components/FileChangeMonitorImpl.h"

#include "mesh_loader/CTMFileLoader.h"
#include "mesh_loader/glTFFileLoader.h"

#include "utils/stdfilesystem.h"
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
	onAfterMeshFileUpdate(absPath);
}

raco::core::SharedMeshData MeshCacheImpl::loadMesh(const raco::core::MeshDescriptor &descriptor) {
	auto *loader = getLoader(descriptor.absPath);
	return loader->loadMesh(descriptor);
}

raco::core::MeshScenegraph raco::components::MeshCacheImpl::getMeshScenegraph(const std::string &absPath, bool bakeAllSubmeshes) {
	auto *loader = getLoader(absPath);
	return loader->getScenegraph(bakeAllSubmeshes);
}

std::string raco::components::MeshCacheImpl::getMeshError(const std::string &absPath) {
	auto *loader = getLoader(absPath);
	return loader->getError();
}

int raco::components::MeshCacheImpl::getTotalMeshCount(const std::string &absPath, bool bakeAllSubmeshes) {
	auto *loader = getLoader(absPath);
	return loader->getTotalMeshCount(bakeAllSubmeshes);
}

void MeshCacheImpl::forceReloadCachedMesh(const std::string &absPath) {
	auto *loader = getLoader(absPath);
	loader->reset();
}

void MeshCacheImpl::onAfterMeshFileUpdate(const std::string &meshFileAbsPath) {
	auto it = callbacks_.find(meshFileAbsPath);
	if (it != callbacks_.end()) {
		for (const auto &meshObjectCallback : it->second) {
			(*meshObjectCallback)();
		}
	}
}

bool endsWith(std::string const &text, std::string const &ending) {
	if (text.length() < ending.length()) return false;
	const auto startPos = text.length() - ending.length();

	return 0 == text.compare(startPos, ending.length(), ending);
}

raco::core::MeshCacheEntry *MeshCacheImpl::getLoader(std::string absPath) {
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