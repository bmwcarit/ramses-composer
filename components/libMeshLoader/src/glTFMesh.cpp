/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "mesh_loader/glTFMesh.h"

#include <assimp/scene.h>
#include <stdexcept>
#include <vector>

namespace raco::mesh_loader {

using namespace raco::core;

glTFMesh::glTFMesh(const aiScene &scene, const core::MeshDescriptor &descriptor) : numTriangles_(0), numVertices_(0) {
	// Not included: Bones, textures, materials, node structure, etc.

	// set up buffers we are going to fill in the loop
	std::vector<float> vertexBuffer{};
	std::vector<float> normalBuffer{};
	std::vector<float> tangentBuffer{};
	std::vector<float> bitangentBuffer{};
	std::vector<std::vector<float>> uvBuffers;
	std::vector<int> uvBufferSizes;
	std::vector<std::vector<float>> colorBuffers;

	// Collect all meshes or selected mesh.
	unsigned meshIndexLimit = (descriptor.bakeAllSubmeshes) ? scene.mNumMeshes : descriptor.submeshIndex + 1;
	for (unsigned meshIndex = (descriptor.bakeAllSubmeshes) ? 0 : descriptor.submeshIndex; meshIndex < meshIndexLimit; ++meshIndex) {
		const aiMesh &mesh = *scene.mMeshes[meshIndex];

		// TODO enable this again once we have meshnode submesh support:
		//materials_.emplace_back(scene.mMaterials[mesh.mMaterialIndex]->GetName().C_Str());

		// Collect our vertices.
		for (unsigned vertexIndex = 0; vertexIndex < mesh.mNumVertices; ++vertexIndex) {
			const aiVector3D &vertex{mesh.mVertices[vertexIndex]};
			vertexBuffer.insert(vertexBuffer.end(), {vertex.x, vertex.y, vertex.z});

			const auto &normal{mesh.mNormals[vertexIndex]};
			normalBuffer.insert(normalBuffer.end(), {normal.x, normal.y, normal.z});

			if (mesh.HasTangentsAndBitangents()) {
				const auto &tangent{mesh.mTangents[vertexIndex]};
				tangentBuffer.insert(tangentBuffer.end(), {tangent.x, tangent.y, tangent.z});
				const auto &bitangent{mesh.mBitangents[vertexIndex]};
				bitangentBuffer.insert(bitangentBuffer.end(), {bitangent.x, bitangent.y, bitangent.z});
			}
		}

		// Collect our faces/indexes
		// Note: we build the correct submesh ranges here in anticipation of submesh support in the meshnode.
		IndexBufferRangeInfo bufferRange = {static_cast<uint32_t>(indexBuffer_.size()), 0};
		for (unsigned faceIndex = 0; faceIndex < mesh.mNumFaces; ++faceIndex) {
			const auto &face{mesh.mFaces[faceIndex]};

			for (unsigned vertexIndex = 0; vertexIndex < face.mNumIndices; ++vertexIndex) {
				indexBuffer_.push_back(face.mIndices[vertexIndex] + numVertices_);
				++bufferRange.count;
			}
		}

		// Add our mesh to the buffer ranges.
		submeshIndexBufferRanges_.push_back(bufferRange);

		// Collect all the UV maps.
		for (unsigned uvChannelIndex = 0; uvChannelIndex < mesh.GetNumUVChannels(); ++uvChannelIndex) {
			// Expand the buffers if we need more.
			if (uvBuffers.size() <= uvChannelIndex) {
				uvBuffers.emplace_back(std::vector<float>{});
			}

			std::vector<float> &uvBuffer{uvBuffers[uvChannelIndex]};

			// Collect the UV coordinates.
			const auto numComponents = mesh.mNumUVComponents[uvChannelIndex];
			for (size_t vertexIndex = 0; vertexIndex < mesh.mNumVertices; ++vertexIndex) {
				const aiVector3D &coordinate = mesh.mTextureCoords[uvChannelIndex][vertexIndex];
				uvBuffer.insert(uvBuffer.end(), {coordinate.x, coordinate.y});
				if (numComponents == 3) {
					uvBuffer.push_back(coordinate.z);
				}
			}
			uvBufferSizes.push_back(numComponents);
		}  // end UV channel loop

		// Collect colors
		for (unsigned colorChannelIndex = 0; colorChannelIndex < mesh.GetNumColorChannels(); ++colorChannelIndex) {
			if (colorBuffers.size() <= colorChannelIndex) {
				colorBuffers.emplace_back(std::vector<float>{});
			}
			std::vector<float> &colorBuffer{colorBuffers[colorChannelIndex]};
			for (size_t vertexIndex = 0; vertexIndex < mesh.mNumVertices; ++vertexIndex) {
				const aiColor4D color = mesh.mColors[colorChannelIndex][vertexIndex];
				colorBuffer.insert(colorBuffer.end(), {color.r, color.g, color.b, color.a});
			}
		}

		numVertices_ += mesh.mNumVertices;
		numTriangles_ += mesh.mNumFaces;
	}  // end mesh loop

	// TODO: only single material mesh right now; use full information from loop above when we have submesh support in meshnode
	submeshIndexBufferRanges_ = {{0, static_cast<uint32_t>(indexBuffer_.size())}};
	materials_ = {"material"};

	// Add the vertices
	attributes_.emplace_back(Attribute{
		ATTRIBUTE_POSITION,
		VertexAttribDataType::VAT_Float3,
		vertexBuffer});

	// Add the normals
	attributes_.emplace_back(Attribute{
		ATTRIBUTE_NORMAL,
		VertexAttribDataType::VAT_Float3,
		normalBuffer});

	if (tangentBuffer.size() > 0 && tangentBuffer.size() == bitangentBuffer.size() && tangentBuffer.size() == vertexBuffer.size()) {
		attributes_.emplace_back(Attribute{ATTRIBUTE_TANGENT, VertexAttribDataType::VAT_Float3, tangentBuffer});
		attributes_.emplace_back(Attribute{ATTRIBUTE_BITANGENT, VertexAttribDataType::VAT_Float3, bitangentBuffer});
	}

	// Add the UV maps
	for (int bufferIndex = 0; bufferIndex < uvBuffers.size(); ++bufferIndex) {
		const unsigned numComponents = uvBufferSizes[bufferIndex];
		const std::string indexCharacter = (bufferIndex == 0) ? "" : std::to_string(bufferIndex);

		// Check that uv buffer uses the same number of vertices as the vertex buffers;
		if (vertexBuffer.size() / 3 == uvBuffers[bufferIndex].size() / numComponents) {
			if (numComponents == 3) {
				attributes_.emplace_back(Attribute{
					std::string{ATTRIBUTE_UVWMAP} + indexCharacter,
					VertexAttribDataType::VAT_Float3,
					uvBuffers[bufferIndex]});
			} else {
				attributes_.emplace_back(Attribute{
					std::string{ATTRIBUTE_UVMAP} + indexCharacter,
					VertexAttribDataType::VAT_Float2,
					uvBuffers[bufferIndex]});
			}
		}
	}

	for (unsigned colorChannelIndex = 0; colorChannelIndex < colorBuffers.size(); ++colorChannelIndex) {
		const std::string indexCharacter = (colorChannelIndex == 0) ? "" : std::to_string(colorChannelIndex);

		// Check that color buffer uses the same number of vertices as the vertex buffers;
		if (vertexBuffer.size() / 3 == colorBuffers[colorChannelIndex].size() / 4) {
			attributes_.emplace_back(Attribute{
				std::string(ATTRIBUTE_COLOR) + indexCharacter,
				VertexAttribDataType::VAT_Float4,
				colorBuffers[colorChannelIndex]});
		}
	}
}

uint32_t glTFMesh::numSubmeshes() const {
	return static_cast<uint32_t>(submeshIndexBufferRanges_.size());
}

uint32_t glTFMesh::numTriangles() const {
	return numTriangles_;
}

uint32_t glTFMesh::numVertices() const {
	return numVertices_;
}

std::vector<std::string> glTFMesh::getMaterialNames() const {
	return materials_;
}

const std::vector<uint32_t> &glTFMesh::getIndices() const {
	return indexBuffer_;
}

const std::vector<MeshData::IndexBufferRangeInfo> &glTFMesh::submeshIndexBufferRanges() const {
	return submeshIndexBufferRanges_;
}

uint32_t glTFMesh::numAttributes() const {
	return static_cast<uint32_t>(attributes_.size());
}

std::string glTFMesh::attribName(int attribute_index) const {
	return attributes_.at(attribute_index).name;
}

uint32_t glTFMesh::attribDataSize(int attribute_index) const {
	return static_cast<uint32_t>(attributes_.at(attribute_index).data.size() * sizeof(float));
}

uint32_t glTFMesh::attribElementCount(int attribute_index) const {
	switch (attributes_.at(attribute_index).type) {
		case VertexAttribDataType::VAT_Float2:
			return static_cast<uint32_t>(attributes_.at(attribute_index).data.size() / 2);
		case VertexAttribDataType::VAT_Float3:
			return static_cast<uint32_t>(attributes_.at(attribute_index).data.size() / 3);
		case VertexAttribDataType::VAT_Float4:
			return static_cast<uint32_t>(attributes_.at(attribute_index).data.size() / 4);
		case VertexAttribDataType::VAT_Float:  // Falls through
			return static_cast<uint32_t>(attributes_.at(attribute_index).data.size());
		default:  // NOLINT(clang-diagnostic-covered-switch-default)
			throw std::range_error("Not a valid attribute type.");
	}
}

MeshData::VertexAttribDataType glTFMesh::attribDataType(int attribute_index) const {
	return attributes_.at(attribute_index).type;
}

const char *glTFMesh::attribBuffer(int attribute_index) const {
	return reinterpret_cast<const char *>(attributes_.at(attribute_index).data.data());
}

}  // namespace raco::mesh_loader
