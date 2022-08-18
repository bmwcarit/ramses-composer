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

#include <string>
#include <vector>

class CTMimporter;

namespace raco::mesh_loader {

class CTMMesh : public raco::core::MeshData {
public:
	CTMMesh(CTMimporter& importer);

	uint32_t numSubmeshes() const override;
	uint32_t numTriangles() const override;
	uint32_t numVertices() const override;


	const std::vector<uint32_t>& getIndices() const override;
	std::vector<std::string> getMaterialNames() const override;

	const std::vector<IndexBufferRangeInfo>& submeshIndexBufferRanges() const override;

	uint32_t numAttributes() const override;
	std::string attribName(int attribIndex) const override;
	uint32_t attribDataSize(int attribIndex) const override;
	uint32_t attribElementCount(int attribIndex) const override;
	VertexAttribDataType attribDataType(int attribIndex) const override;
	const char* attribBuffer(int attribIndex) const override;

private:
	struct Attribute {
		std::string name;
		VertexAttribDataType type;
		std::vector<float> data;
	};

	uint32_t numTriangles_;
	uint32_t numVertices_;

	std::vector<uint32_t> indexBuffer_;
	std::vector<Attribute> attributes_;
	std::vector<IndexBufferRangeInfo> submeshIndexBufferRanges_;
};

}  // namespace raco::mesh_loader