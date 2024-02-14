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

#include "core/MeshCacheInterface.h"

class CTMimporter;

namespace raco::mesh_loader {

class CTMFileLoader : public core::MeshCacheEntry {
public:
	CTMFileLoader(std::string absPath);
	virtual ~CTMFileLoader() = default;

	core::SharedMeshData loadMesh(const core::MeshDescriptor& descriptor) override;
	std::string getError() override;
	void reset() override;
	const core::MeshScenegraph* getScenegraph(const std::string& absPath) override;
	int getTotalMeshCount() override;
	core::SharedAnimationSamplerData getAnimationSamplerData(const std::string& absPath, int animIndex, int samplerIndex) override;

	core::SharedSkinData loadSkin(const std::string& absPath, int skinIndex, std::string& outError) override;

private:
	bool loadFile();

	std::string path_;
	std::string error_;
	std::unique_ptr<CTMimporter> importer_;
	bool valid_;
};

}  // namespace raco::mesh_loader