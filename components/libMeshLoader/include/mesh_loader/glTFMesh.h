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

#include <glm/mat4x4.hpp>
#include <string>
#include <vector>

namespace tinygltf {
struct Primitive;
struct BufferView;
struct Accessor;
class Model;
}  // namespace tinygltf

namespace raco::mesh_loader {

class glTFMesh : public core::MeshData {
public:
	glTFMesh(const tinygltf::Model &scene, const core::MeshScenegraph &sceneGraph, const core::MeshDescriptor &descriptor);

	uint32_t numSubmeshes() const override;
	uint32_t numTriangles() const override;
	uint32_t numVertices() const override;

	std::vector<std::string> getMaterialNames() const override;

	const std::vector<uint32_t>& getIndices() const override;

	std::map<std::string, std::string> getMetadata() const override;

	const std::vector<IndexBufferRangeInfo>& submeshIndexBufferRanges() const override;

	uint32_t numAttributes() const override;
	std::string attribName(int attribute_index) const override;
	uint32_t attribDataSize(int attribute_index) const override;
	uint32_t attribElementCount(int attribute_index) const override;
	VertexAttribDataType attribDataType(int attribute_index) const override;
	const char* attribBuffer(int attribute_index) const override;

	const std::vector<glm::vec3>& triangleBuffer() const override;

private:
	struct Attribute {
		std::string name;
		VertexAttribDataType type;
		std::vector<float> data;
	};

	void loadPrimitiveData(const tinygltf::Primitive& primitive, const tinygltf::Model& scene,
		std::vector<float>& vertexBuffer,
		std::vector<std::vector<float>>& morphVertexBuffers,
		std::vector<float>& normalBuffer,
		std::vector<std::vector<float>>& morphNormalBuffers,
		std::vector<float>& tangentBuffer,
		std::vector<float>& bitangentBuffer,
		std::vector<std::vector<float>>& uvBuffers,
		std::vector<std::vector<float>>& colorBuffers,
		std::vector<std::vector<float>>& weightBuffers,
		std::vector<std::vector<float>>& jointBuffers,
		glm::dmat4* globalModelMatrix = nullptr,
		glm::dmat4* globalNormalMatrix = nullptr);

	uint32_t numTriangles_;
	uint32_t numVertices_;

	std::vector<uint32_t> indexBuffer_;
	std::vector<Attribute> attributes_;
	std::vector<IndexBufferRangeInfo> submeshIndexBufferRanges_;

	std::vector<std::string> materials_;

	std::map<std::string, std::string> metadata_;

	std::vector<glm::vec3> triangleBuffer_;
};

}  // namespace raco::mesh_loader