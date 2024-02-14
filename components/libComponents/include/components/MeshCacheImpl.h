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

#include "components/FileChangeMonitorImpl.h"
#include "core/MeshCacheInterface.h"

#include <functional>
#include <memory>
#include <string>
#include <unordered_set>

namespace raco::core {
class BaseContext;
}  // namespace raco::core

namespace raco::components {

class MeshCacheImpl : public GenericFileChangeMonitorImpl<core::MeshCache> {
public:
	MeshCacheImpl() {}

	core::SharedMeshData loadMesh(const core::MeshDescriptor& descriptor) override;
	const core::MeshScenegraph* getMeshScenegraph(const std::string& absPath) override;
	std::string getMeshError(const std::string& absPath) override;
	int getTotalMeshCount(const std::string& absPath) override;

	core::SharedAnimationSamplerData getAnimationSamplerData(const std::string& absPath, int animIndex, int samplerIndex) override;

	core::SharedSkinData loadSkin(const std::string& absPath, int skinIndex, std::string& outError) override;

private:
	virtual void unregister(std::string absPath, typename core::MeshCache::Callback* listener) override;
	virtual void notify(const std::string& absPath) override;

	core::MeshCacheEntry* getLoader(std::string absPath) override;

	void forceReloadCachedMesh(const std::string& absPath);

	std::unordered_map<std::string, core::UniqueMeshCacheEntry> meshCacheEntries_;
};

}  // namespace raco::components