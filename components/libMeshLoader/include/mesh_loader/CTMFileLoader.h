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

class CTMimporter;

namespace raco::mesh_loader {

class CTMFileLoader : public raco::core::MeshCacheEntry {
public:
	CTMFileLoader(std::string absPath);
	virtual ~CTMFileLoader() = default;

	raco::core::SharedMeshData loadMesh(const raco::core::MeshDescriptor& descriptor) override;
	std::string getError() override;
	std::string getWarning() override;
	void reset() override;
	raco::core::MeshScenegraph getScenegraph() override;
	int getTotalMeshCount() override;

private:
	bool loadFile();

	std::string path_;
	std::string error_;
	std::unique_ptr<CTMimporter> importer_;
	bool valid_;
};

}  // namespace raco::mesh_loader