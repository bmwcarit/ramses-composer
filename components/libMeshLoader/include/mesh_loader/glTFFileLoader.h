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

namespace Assimp {
	class Importer;
}

struct aiNode;
struct aiScene; // forward declaration

namespace raco::mesh_loader {

class glTFFileLoader final : public raco::core::MeshCacheEntry {
public:
	glTFFileLoader(std::string absPath);
	virtual ~glTFFileLoader() = default;

	raco::core::SharedMeshData loadMesh(const raco::core::MeshDescriptor& descriptor) override;
	raco::core::MeshScenegraph getScenegraph(bool bakeAllSubmeshes) override;
	int getTotalMeshCount(bool bakeAllSubmeshes) override;
	std::string getError() override;
	void reset() override;


private:
	std::string path_;
	std::unique_ptr<Assimp::Importer> bakedSceneImporter_;
	std::unique_ptr<Assimp::Importer> unbakedSceneImporter_;
	const aiScene* bakedScene_{nullptr};
	raco::core::MeshScenegraph bakedScenegraph_;
	const aiScene* unbakedScene_{nullptr};
	raco::core::MeshScenegraph unbakedScenegraph_;
	std::string error_;

	bool buildglTFScenegraph(std::vector<core::MeshScenegraphNode>& sceneGraph, int parentIndex, aiNode* child);
	bool importglTFScene(const core::MeshDescriptor& descriptor, std::unique_ptr<Assimp::Importer>& importer, const aiScene* &scene, raco::core::MeshScenegraph &sceneGraph, unsigned flags);


};

} // namespace raco::mesh_loader