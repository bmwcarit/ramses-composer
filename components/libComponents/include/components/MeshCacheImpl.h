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

#include "core/MeshCacheInterface.h"

#include <functional>
#include <memory>
#include <string>
#include <unordered_set>

namespace raco::core {
	class BaseContext;
}
namespace raco::components {

class RamsesMeshObjectCallback;

class MeshCacheImpl : public core::MeshCache {
public:
	MeshCacheImpl(core::BaseContext& ctx);

	UniqueListener registerFileChangedHandler(std::string absPath, core::FileChangeCallback callback) override;
	void unregisterListener(const std::string& absPath, RamsesMeshObjectCallback* callback);
	core::SharedMeshData loadMesh(const raco::core::MeshDescriptor& descriptor) override;
	core::MeshScenegraph getMeshScenegraph(const std::string& absPath, bool bakeAllSubmeshes) override;
	std::string getMeshError(const std::string& absPath) override;
	int getTotalMeshCount(const std::string& absPath, bool bakeAllSubmeshes) override;

private:
	void forceReloadCachedMesh(const std::string& absPath);
	void onAfterMeshFileUpdate(const std::string& meshFileAbsPath);
	core::MeshCacheEntry* getLoader(std::string absPath) override;

	core::BaseContext& context_;
	std::unordered_map<std::string, UniqueListener> meshFileChangeListeners_;
	std::unordered_map<std::string, std::unordered_set<RamsesMeshObjectCallback*>> meshObjectCallbacks_;
	std::unordered_map<std::string, core::UniqueMeshCacheEntry> meshCacheEntries_;
};

}