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

#include "mesh_loader/glTFBufferData.h"
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/vec3.hpp>
#include <log_system/log.h>
#include <optional>
#include <stdexcept>
#include <vector>

namespace raco::mesh_loader {

using namespace raco::core;


glTFMesh::glTFMesh(const tinygltf::Model &scene, core::MeshScenegraph &sceneGraph, const core::MeshDescriptor &descriptor) : numTriangles_(0), numVertices_(0) {
	// Not included: Bones, textures, materials, node structure, etc.

	// set up buffers we are going to fill in the loop
	std::vector<float> vertexBuffer{};
	std::vector<float> normalBuffer{};
	std::vector<float> tangentBuffer{};
	std::vector<float> bitangentBuffer{};
	std::vector<std::vector<float>> uvBuffers(MAX_NUMBER_TEXTURECOORDS);
	std::vector<std::vector<float>> colorBuffers(MAX_NUMBER_COLORS);

	std::vector<tinygltf::Primitive> flattenedPrimitiveList;

	for (const auto &mesh : scene.meshes) {
		for (const auto &prim : mesh.primitives) {
			flattenedPrimitiveList.emplace_back(prim);
		}
	}

	// Collect all meshes or selected mesh.
	if (!descriptor.bakeAllSubmeshes) {
		for (auto primitiveIndex = descriptor.submeshIndex; primitiveIndex < descriptor.submeshIndex + 1; ++primitiveIndex) {
			auto primitive = flattenedPrimitiveList[primitiveIndex];

			// TODO enable this again once we have meshnode submesh support:
			//materials_.emplace_back(scene.materials[primitive.material].name);

			loadPrimitiveData(primitive, scene, vertexBuffer, normalBuffer, tangentBuffer, bitangentBuffer, uvBuffers, colorBuffers);
		}
	} else {
		// calculate local node transformations to later transfer them to the node's vertex positions
		// see: https://github.com/KhronosGroup/glTF-Tutorials/blob/master/gltfTutorial/gltfTutorial_004_ScenesNodes.md#global-transforms-of-nodes

		auto generateTrafoMatrix = [](const core::MeshScenegraphNode &node) {
			auto trafoMatrix = glm::identity<glm::dmat4x4>();
			trafoMatrix = glm::translate(trafoMatrix, glm::dvec3(node.transformations.translation[0], node.transformations.translation[1], node.transformations.translation[2]));
			auto rotationRadians = glm::radians(glm::dvec3{node.transformations.rotation[0], node.transformations.rotation[1], node.transformations.rotation[2]});
			trafoMatrix = glm::rotate(trafoMatrix, rotationRadians.x, {1, 0, 0});
			trafoMatrix = glm::rotate(trafoMatrix, rotationRadians.y, {0, 1, 0});
			trafoMatrix = glm::rotate(trafoMatrix, rotationRadians.z, {0, 0, 1});
			trafoMatrix = glm::scale(trafoMatrix, {node.transformations.scale[0], node.transformations.scale[1], node.transformations.scale[2]});

			return trafoMatrix;
		};

		std::vector<glm::dmat4x4> nodeTrafos(sceneGraph.nodes.size());
		for (auto i = 0; i < sceneGraph.nodes.size(); ++i) {
			auto &node = sceneGraph.nodes[i].value();
			nodeTrafos[i] = generateTrafoMatrix(node);
			// Cleaning out the extracted transformation for easier debugging 
			node.transformations.translation = {0, 0, 0};
			node.transformations.rotation = {0, 0, 0};
			node.transformations.scale = {1, 1, 1};
		}


		for (auto nodeIndex = 0; nodeIndex < sceneGraph.nodes.size(); ++nodeIndex) {		
			const auto &node = sceneGraph.nodes[nodeIndex].value();
			if (node.subMeshIndeces.empty()) {
				continue;
			}

			auto globalTransformation = glm::identity<glm::dmat4x4>();

			auto currentIndex = nodeIndex;
			while (currentIndex != -1) {
				globalTransformation = nodeTrafos[currentIndex] * globalTransformation;
				currentIndex = sceneGraph.nodes[currentIndex]->parentIndex;
			}

			for (const auto &primitiveIndex : sceneGraph.nodes[nodeIndex]->subMeshIndeces) {
				auto primitive = flattenedPrimitiveList[*primitiveIndex];
				loadPrimitiveData(primitive, scene, vertexBuffer, normalBuffer, tangentBuffer, bitangentBuffer, uvBuffers, colorBuffers, &globalTransformation);
			}
		}
	}

	// TODO: only single material mesh right now; use full information from loop above when we have submesh support in meshnode
	submeshIndexBufferRanges_ = {{0, static_cast<uint32_t>(indexBuffer_.size())}};
	materials_ = {"material"};

	// Add the vertices
	attributes_.emplace_back(Attribute{
		ATTRIBUTE_POSITION,
		VertexAttribDataType::VAT_Float3,
		vertexBuffer});

	// Add the normals
	if (!normalBuffer.empty()) {
		attributes_.emplace_back(Attribute{
			ATTRIBUTE_NORMAL,
			VertexAttribDataType::VAT_Float3,
			normalBuffer});
	}

	if (!tangentBuffer.empty() && tangentBuffer.size() == bitangentBuffer.size() && tangentBuffer.size() == vertexBuffer.size()) {
		attributes_.emplace_back(Attribute{ATTRIBUTE_TANGENT, VertexAttribDataType::VAT_Float3, tangentBuffer});
		attributes_.emplace_back(Attribute{ATTRIBUTE_BITANGENT, VertexAttribDataType::VAT_Float3, bitangentBuffer});
	}

	// Add the UV maps
	for (int bufferIndex = 0; bufferIndex < uvBuffers.size(); ++bufferIndex) {
		const std::string indexCharacter = (bufferIndex == 0) ? "" : std::to_string(bufferIndex);

		// Check that uv buffer uses the same number of vertices as the vertex buffers;
		if (vertexBuffer.size() / 3 == uvBuffers[bufferIndex].size() / 2) {
			attributes_.emplace_back(Attribute{
				std::string{ATTRIBUTE_UVMAP} + indexCharacter,
				VertexAttribDataType::VAT_Float2,
				uvBuffers[bufferIndex]});
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

void glTFMesh::loadPrimitiveData(const tinygltf::Primitive &primitive, const tinygltf::Model &scene, std::vector<float> &vertexBuffer, std::vector<float> &normalBuffer, std::vector<float> &tangentBuffer, std::vector<float> &bitangentBuffer, std::vector<std::vector<float>> &uvBuffers, std::vector<std::vector<float>> &colorBuffers, glm::dmat4 *rootTrafoMatrix) {
	if (primitive.attributes.find("POSITION") == primitive.attributes.end()) {
		LOG_ERROR(log_system::MESH_LOADER, "primitive has no position attributes defined");
		return;
	}

	std::optional<glTFBufferData> normalData;
	std::optional<glTFBufferData> tangentData;
	std::array<std::optional<glTFBufferData>, MAX_NUMBER_TEXTURECOORDS> bufferTexCoordDatas;
	std::array<std::optional<glTFBufferData>, MAX_NUMBER_COLORS> bufferColorDatas;

	if (primitive.attributes.find("NORMAL") != primitive.attributes.end()) {
		normalData.emplace(scene, primitive.attributes.at("NORMAL"), std::set<int>{TINYGLTF_COMPONENT_TYPE_FLOAT});

		if (primitive.attributes.find("TANGENT") != primitive.attributes.end()) {
			tangentData.emplace(scene, primitive.attributes.at("TANGENT"), std::set<int>{TINYGLTF_COMPONENT_TYPE_FLOAT});
		}
	}

	for (auto uvChannel = 0; uvChannel < MAX_NUMBER_TEXTURECOORDS; ++uvChannel) {
		auto texCoordName = fmt::format("TEXCOORD_{}", uvChannel);
		if (primitive.attributes.find(texCoordName) != primitive.attributes.end()) {
			bufferTexCoordDatas[uvChannel].emplace(scene, primitive.attributes.at(texCoordName), std::set<int>{TINYGLTF_COMPONENT_TYPE_FLOAT, TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE, TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT});
		}
	}

	for (auto colorChannel = 0; colorChannel < MAX_NUMBER_COLORS; ++colorChannel) {
		auto colorName = fmt::format("COLOR_{}", colorChannel);
		if (primitive.attributes.find(colorName) != primitive.attributes.end()) {
			bufferColorDatas[colorChannel].emplace(scene,primitive.attributes.at(colorName), std::set<int>{TINYGLTF_COMPONENT_TYPE_FLOAT, TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE, TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT});
		}
	}

	glTFBufferData posData(scene, primitive.attributes.at("POSITION"), std::set<int>{TINYGLTF_COMPONENT_TYPE_FLOAT});

	for (size_t vertexIndex = 0; vertexIndex < posData.accessor_.count; vertexIndex++) {
		auto position = posData.getDataAt<float>(vertexIndex);
		if (rootTrafoMatrix) {
			auto transformedVerts = *rootTrafoMatrix * glm::dvec4(position[0], position[1], position[2], 1);
			vertexBuffer.insert(vertexBuffer.end(), {static_cast<float>(transformedVerts.x), static_cast<float>(transformedVerts.y), static_cast<float>(transformedVerts.z)});
		} else {
			vertexBuffer.insert(vertexBuffer.end(), {position[0], position[1], position[2]});
		}

		if (normalData) {
			auto currentNormal = normalData->getDataAt<float>(vertexIndex);
			normalBuffer.insert(normalBuffer.end(), currentNormal.begin(), currentNormal.end());

			if (tangentData) {
				auto tangent = tangentData->getDataAt<float>(vertexIndex);
				assert(tangent.size() == 4);

				tangentBuffer.insert(tangentBuffer.end(), tangent.begin(), tangent.end());
				auto tangentW = tangent[3];

				// cross tangent with normal and multiply with tangent.W to get bitangent
				auto bitangentXYZ = {tangentW * (currentNormal[1] * tangent[2] - currentNormal[2] * tangent[1]),
					tangentW * (currentNormal[2] * tangent[0] - currentNormal[0] * tangent[2]),
					tangentW * (currentNormal[0] * tangent[1] - currentNormal[1] * tangent[0])};
				bitangentBuffer.insert(bitangentBuffer.end(), bitangentXYZ.begin(), bitangentXYZ.end());

			}

		}

		for (auto uvChannel = 0; uvChannel < MAX_NUMBER_TEXTURECOORDS; ++uvChannel) {
			if (bufferTexCoordDatas[uvChannel]) {
				std::vector<float> &uvBuffer{uvBuffers[uvChannel]};
				auto textureData = bufferTexCoordDatas[uvChannel]->getDataAt<float>(vertexIndex);
				assert(textureData.size() == 2);

				uvBuffer.insert(uvBuffer.end(), textureData.begin(), textureData.end());
			}
		}

		for (auto colorChannel = 0; colorChannel < MAX_NUMBER_COLORS; ++colorChannel) {
			if (bufferColorDatas[colorChannel]) {
				std::vector<float> &colorBuffer{colorBuffers[colorChannel]};
				std::vector<float> colorData(4, 1.0F);

				switch (bufferColorDatas[colorChannel]->accessor_.componentType) {
					case TINYGLTF_PARAMETER_TYPE_FLOAT: {
						auto floatColorData = bufferColorDatas[colorChannel]->getDataAt<float>(vertexIndex);
						assert(floatColorData.size() % 3 == 0 || floatColorData.size() % 4 == 0);

						colorData.resize(floatColorData.size(), 1.0F);
						for (auto i = 0; i < floatColorData.size(); ++i) {
							colorData[i] = floatColorData[i];
						}

						break;
					}
					case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE: {
						auto byteColorData = bufferColorDatas[colorChannel]->getDataAt<uint8_t>(vertexIndex);
						assert(byteColorData.size() % 3 == 0 || byteColorData.size() % 4 == 0);

						colorData.resize(byteColorData.size(), 1.0F);
						for (auto i = 0; i < byteColorData.size(); ++i) {
							colorData[i] = byteColorData[i] / (0.0F + std::numeric_limits<uint8_t>().max());
						}

						break;
					}
					case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT: {
						auto shortColorData = bufferColorDatas[colorChannel]->getDataAt<uint16_t>(vertexIndex);
						assert(shortColorData.size() % 3 == 0 || shortColorData.size() % 4 == 0);

						colorData.resize(shortColorData.size(), 1.0F);
						for (auto i = 0; i < shortColorData.size(); ++i) {
							colorData[i] = shortColorData[i] / (0.0F + std::numeric_limits<uint16_t>().max());
						}
						break;
					}
					default: {
						throw std::range_error("No valid color attribute type found.");
						return;
					}
				}

				colorBuffer.insert(colorBuffer.end(), colorData.begin(), colorData.end());
			}
		}
	}

	// Collect our faces/indexes
	// Note: we build the correct submesh ranges here in anticipation of submesh support in the meshnode.
	IndexBufferRangeInfo bufferRange = {static_cast<uint32_t>(indexBuffer_.size()), 0};

	if (primitive.indices > -1) {
		glTFBufferData indexBufferData(scene, primitive.indices, {TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT, TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT, TINYGLTF_TEXTURE_TYPE_UNSIGNED_BYTE});
		auto indexAccessorCount = indexBufferData.accessor_.count;

		switch (indexBufferData.accessor_.componentType) {
			case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT: {
				for (size_t index = 0; index < indexAccessorCount; index++) {
					auto data = indexBufferData.getDataAt<uint32_t>(index, false).front();
					indexBuffer_.emplace_back(data + numVertices_);
				}
				break;
			}
			case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT: {
				for (size_t index = 0; index < indexAccessorCount; index++) {
					auto data = indexBufferData.getDataAt<uint16_t>(index, false).front();
					indexBuffer_.emplace_back(data + numVertices_);
				}
				break;
			}
			case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE: {
				for (size_t index = 0; index < indexAccessorCount; index++) {
					auto data = indexBufferData.getDataAt<uint8_t>(index, false).front();
					indexBuffer_.emplace_back(data + numVertices_);
				}
				break;
			}
			default: {
				throw std::range_error("No valid primitive index attribute type found.");
				return;
			}
		}
		bufferRange.count += indexAccessorCount;
		numTriangles_ += indexAccessorCount / 3;
	} else {
		auto indexCount = posData.accessor_.count;

		for (auto index = 0; index < indexCount; ++index) {
			indexBuffer_.emplace_back(index + numVertices_);
		}

		bufferRange.count += indexCount;
		numTriangles_ += indexCount / 3;
	}
	submeshIndexBufferRanges_.push_back(bufferRange);

	numVertices_ += posData.accessor_.count;
}

}  // namespace raco::mesh_loader
