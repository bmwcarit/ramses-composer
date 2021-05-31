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

#include <assimp/scene.h>

#include <string>
#include <vector>

namespace raco::mesh_loader {

class glTFMesh : public raco::core::MeshData {
public:
	glTFMesh(const aiScene &scene, const core::MeshDescriptor &descriptor);

	uint32_t numSubmeshes() const override;
	uint32_t numTriangles() const override;
	uint32_t numVertices() const override;

	std::vector<std::string> getMaterialNames() const override;

	const std::vector<uint32_t>& getIndices() const override;

	const std::vector<IndexBufferRangeInfo>& submeshIndexBufferRanges() const override;

	uint32_t numAttributes() const override;
	std::string attribName(int attribute_index) const override;
	uint32_t attribDataSize(int attribute_index) const override;
	uint32_t attribElementCount(int attribute_index) const override;
	VertexAttribDataType attribDataType(int attribute_index) const override;
	const char* attribBuffer(int attribute_index) const override;

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

	std::vector<std::string> materials_;
};

}  // namespace raco::mesh_loader