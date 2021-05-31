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
#include "FileChangeListenerImpl.h"
#include "log_system/log.h"
#include "components/FileChangeMonitorImpl.h"

#include "mesh_loader/CTMFileLoader.h"
#include "mesh_loader/glTFFileLoader.h"

#include "utils/stdfilesystem.h"
#include <memory>

namespace raco::components {

// FileChangeListener-derived class without QtFileSystemWatcher (the callbacks will be triggered from meshFileChangeListeners_)
class RamsesMeshObjectCallback : public raco::core::FileChangeListener {
public:
	RamsesMeshObjectCallback(std::string &absPath, raco::core::FileChangeCallback &callbackHandler, raco::core::BaseContext &context)
		: path_(absPath), fileChangeCallback_(callbackHandler), currentContext_(context) {
	}

	std::string getPath() const override {
		return path_.generic_string();
	}

	void operator()() {
		fileChangeCallback_(currentContext_);
	}

private:
	std::filesystem::path path_;
	raco::core::FileChangeCallback fileChangeCallback_;
	raco::core::BaseContext &currentContext_;
};

MeshCacheImpl::MeshCacheImpl(raco::core::BaseContext &ctx) : context_(ctx) {}

raco::core::FileChangeMonitor::UniqueListener MeshCacheImpl::registerFileChangedHandler(std::string absPath, raco::core::FileChangeCallback callback) {
	if (meshFileChangeListeners_.count(absPath) == 0) {
		meshFileChangeListeners_[absPath] = context_.fileChangeMonitor()->registerFileChangedHandler(absPath,
			{nullptr, [this, absPath](auto &ctx) {
				 forceReloadCachedMesh(absPath);
				 onAfterMeshFileUpdate(absPath);
			 }});
	}

	auto *l = new RamsesMeshObjectCallback{absPath, callback, context_};

	meshObjectCallbacks_[absPath].insert(l);

	return UniqueListener(l, [this, absPath, l](auto *fileChangeListener) {
		this->unregisterListener(absPath, l);
		delete fileChangeListener;
	});
}

void MeshCacheImpl::unregisterListener(const std::string &absPath, RamsesMeshObjectCallback *callback) {
	if (meshObjectCallbacks_.count(absPath) > 0) {
		meshObjectCallbacks_[absPath].erase(callback);
		if (meshObjectCallbacks_[absPath].empty()) {
			meshObjectCallbacks_.erase(absPath);
			meshFileChangeListeners_.erase(absPath);
		}
	}
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
	if (meshObjectCallbacks_.count(meshFileAbsPath) > 0) {
		for (const auto &meshObjectCallback : meshObjectCallbacks_[meshFileAbsPath]) {
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