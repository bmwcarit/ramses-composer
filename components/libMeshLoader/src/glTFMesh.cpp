/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
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

glTFMesh::glTFMesh(const tinygltf::Model &scene, const core::MeshScenegraph &sceneGraph, const core::MeshDescriptor &descriptor) : numTriangles_(0), numVertices_(0) {
	// Not included: Bones, textures, materials, node structure, etc.

	// set up buffers we are going to fill in the loop
	std::vector<float> vertexBuffer{};
	std::vector<float> normalBuffer{};
	std::vector<float> tangentBuffer{};
	std::vector<float> bitangentBuffer{};
	std::vector<std::vector<float>> uvBuffers;
	std::vector<std::vector<float>> colorBuffers;
	std::vector<std::vector<float>> weightBuffers;
	std::vector<std::vector<float>> jointBuffers;

	std::vector<std::vector<float>> morphVertexBuffers;
	std::vector<std::vector<float>> morphNormalBuffers;

	std::vector<std::pair<tinygltf::Primitive, int>> flattenedPrimitiveList;

	for (int meshIndex = 0; meshIndex < scene.meshes.size(); ++meshIndex) {
		const auto &mesh = scene.meshes[meshIndex];
		for (const auto &prim : mesh.primitives) {
			flattenedPrimitiveList.emplace_back(prim, meshIndex);
		}
	}

	// Collect all meshes or selected mesh.
	if (!descriptor.bakeAllSubmeshes) {
		for (auto primitiveIndex = descriptor.submeshIndex; primitiveIndex < descriptor.submeshIndex + 1; ++primitiveIndex) {
			auto primitiveEntry = flattenedPrimitiveList[primitiveIndex];
			const auto &originMesh = scene.meshes[primitiveEntry.second];

			const auto &extras = originMesh.extras;
			if (extras.IsObject() && extras.Size() > 0) {
				for (const auto &key : extras.Keys()) {
					const auto &value = extras.Get(key);
					if (value.IsString()) {
						metadata_[key] = value.Get<std::string>();
					}
				}
			}

			// TODO enable this again once we have meshnode submesh support:
			// materials_.emplace_back(scene.materials[primitive.material].name);

			loadPrimitiveData(primitiveEntry.first, scene, vertexBuffer, morphVertexBuffers, normalBuffer, morphNormalBuffers, tangentBuffer, bitangentBuffer, uvBuffers, colorBuffers, weightBuffers, jointBuffers);
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
		}

		for (auto nodeIndex = 0; nodeIndex < sceneGraph.nodes.size(); ++nodeIndex) {
			const auto &node = sceneGraph.nodes[nodeIndex].value();
			if (node.subMeshIndices.empty()) {
				continue;
			}

			auto globalModelMatrix = glm::identity<glm::dmat4x4>();

			auto currentIndex = nodeIndex;
			while (currentIndex != -1) {
				globalModelMatrix = nodeTrafos[currentIndex] * globalModelMatrix;
				currentIndex = sceneGraph.nodes[currentIndex]->parentIndex;
			}

			// Calculate correct normal matrix:
			// normal matrix = transpose(inverse(model_matrix)))
			auto globalNormalMatrix = glm::dmat4x4(glm::transpose(glm::inverse(glm::dmat3x3(globalModelMatrix))));

			for (const auto &primitiveIndex : sceneGraph.nodes[nodeIndex]->subMeshIndices) {
				auto primitive = flattenedPrimitiveList[*primitiveIndex];
				loadPrimitiveData(primitive.first, scene, vertexBuffer, morphVertexBuffers, normalBuffer, morphNormalBuffers, tangentBuffer, bitangentBuffer, uvBuffers, colorBuffers, weightBuffers, jointBuffers, &globalModelMatrix, &globalNormalMatrix);
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

	for (size_t index = 0; index < morphVertexBuffers.size(); index++) {
		if (!morphVertexBuffers[index].empty()) {
			attributes_.emplace_back(Attribute{
				fmt::format("{}_Morph_{}", ATTRIBUTE_POSITION, index),
				VertexAttribDataType::VAT_Float3,
				morphVertexBuffers[index]});
		}
	}

	// Add the normals
	if (!normalBuffer.empty()) {
		attributes_.emplace_back(Attribute{
			ATTRIBUTE_NORMAL,
			VertexAttribDataType::VAT_Float3,
			normalBuffer});
	}

	for (size_t index = 0; index < morphNormalBuffers.size(); index++) {
		if (!morphNormalBuffers[index].empty()) {
			attributes_.emplace_back(Attribute{
				fmt::format("{}_Morph_{}", ATTRIBUTE_NORMAL, index),
				VertexAttribDataType::VAT_Float3,
				morphNormalBuffers[index]});
		}
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
		// TODO This implicitly only support VEC4 color buffers even if loadPrimitiveData allows for VEC3. Why?
		if (vertexBuffer.size() / 3 == colorBuffers[colorChannelIndex].size() / 4) {
			attributes_.emplace_back(Attribute{
				std::string(ATTRIBUTE_COLOR) + indexCharacter,
				VertexAttribDataType::VAT_Float4,
				colorBuffers[colorChannelIndex]});
		}
	}

	for (size_t index = 0; index < weightBuffers.size(); ++index) {
		if (vertexBuffer.size() / 3 == weightBuffers[index].size() / 4) {
			attributes_.emplace_back(Attribute{std::string(ATTRIBUTE_WEIGHTS) + std::to_string(index), VertexAttribDataType::VAT_Float4, weightBuffers[index]});
		}
	}

	for (size_t index = 0; index < jointBuffers.size(); ++index) {
		if (vertexBuffer.size() / 3 == jointBuffers[index].size() / 4) {
			attributes_.emplace_back(Attribute{std::string(ATTRIBUTE_JOINTS) + std::to_string(index), VertexAttribDataType::VAT_Float4, jointBuffers[index]});
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

std::map<std::string, std::string> glTFMesh::getMetadata() const {
	return metadata_;
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

void convertVectorWithTransformation(std::vector<float> vector, std::vector<float> &buffer, glm::dmat4 *trafoMatrix, double component_4) {
	if (trafoMatrix) {
		auto transformed = *trafoMatrix * glm::dvec4(vector[0], vector[1], vector[2], component_4);
		buffer.insert(buffer.end(), {static_cast<float>(transformed.x), static_cast<float>(transformed.y), static_cast<float>(transformed.z)});
	} else {
		buffer.insert(buffer.end(), {vector[0], vector[1], vector[2]});
	}
}

void convertPositionData(const glTFBufferData &data, std::vector<float> &buffer, glm::dmat4 *rootTrafoMatrix = nullptr) {
	for (size_t vertexIndex = 0; vertexIndex < data.accessor_.count; vertexIndex++) {
		convertVectorWithTransformation(data.getDataAt<float>(vertexIndex), buffer, rootTrafoMatrix, 1.0);
	}
}

void convertAttributeSet(const tinygltf::Primitive &primitive, const tinygltf::Model &scene, std::vector<std::vector<float>> &buffers, const std::string &attributeBaseName, const std::set<int> &allowedComponentTypes, const std::set<int> &allowedTypes, bool normalize, int numVertices) {
	for (auto channel = 0; channel < std::numeric_limits<int>::max(); ++channel) {
		auto attribName = fmt::format("{}_{}", attributeBaseName, channel);
		if (primitive.attributes.find(attribName) != primitive.attributes.end()) {
			glTFBufferData bufferData(scene, primitive.attributes.at(attribName), allowedComponentTypes, allowedTypes);

			if (bufferData.accessor_.count == numVertices) {
				buffers.resize(channel + 1);
				std::vector<float> &buffer{buffers[channel]};
				for (size_t vertexIndex = 0; vertexIndex < bufferData.accessor_.count; vertexIndex++) {
					auto elementData = normalize ? bufferData.getNormalizedData(vertexIndex) : bufferData.getConvertedData<float>(vertexIndex);
					buffer.insert(buffer.end(), elementData.begin(), elementData.end());
				}
			} else {
				LOG_WARNING(log_system::MESH_LOADER, "Attribute '{}' has different size than vertex buffer, ignoring it.", attribName);
			}
		} else {
			break;
		}
	}
}

void glTFMesh::loadPrimitiveData(const tinygltf::Primitive &primitive, const tinygltf::Model &scene,
	std::vector<float> &vertexBuffer,
	std::vector<std::vector<float>> &morphVertexBuffers,
	std::vector<float> &normalBuffer,
	std::vector<std::vector<float>> &morphNormalBuffers,
	std::vector<float> &tangentBuffer,
	std::vector<float> &bitangentBuffer,
	std::vector<std::vector<float>> &uvBuffers,
	std::vector<std::vector<float>> &colorBuffers,
	std::vector<std::vector<float>> &weightBuffers,
	std::vector<std::vector<float>> &jointBuffers,
	glm::dmat4 *globalModelMatrix,
	glm::dmat4 *globalNormalMatrix) {
	if (primitive.attributes.find("POSITION") == primitive.attributes.end()) {
		LOG_ERROR(log_system::MESH_LOADER, "primitive has no position attributes defined");
		return;
	}

	glTFBufferData posData(scene, primitive.attributes.at("POSITION"), std::set<int>{TINYGLTF_COMPONENT_TYPE_FLOAT}, std ::set<int>{TINYGLTF_TYPE_VEC3});
	convertPositionData(posData, vertexBuffer, globalModelMatrix);
	auto numVertices = posData.accessor_.count;

	morphVertexBuffers.resize(primitive.targets.size());
	for (size_t index = 0; index < primitive.targets.size(); index++) {
		auto it = primitive.targets[index].find("POSITION");
		if (it != primitive.targets[index].end()) {
			glTFBufferData data(scene, it->second, std::set<int>{TINYGLTF_COMPONENT_TYPE_FLOAT}, std ::set<int>{TINYGLTF_TYPE_VEC3});
			if (data.accessor_.count == numVertices) {
				convertPositionData(data, morphVertexBuffers[index], globalModelMatrix);
			} else {
				LOG_WARNING(log_system::MESH_LOADER, "Morph position attribute has different size than vertex buffer, ignoring it.");
			}
		}
	}

	std::optional<glTFBufferData> normalData;
	std::optional<glTFBufferData> tangentData;

	if (primitive.attributes.find("NORMAL") != primitive.attributes.end()) {
		normalData.emplace(scene, primitive.attributes.at("NORMAL"), std::set<int>{TINYGLTF_COMPONENT_TYPE_FLOAT}, std ::set<int>{TINYGLTF_TYPE_VEC3});

		if (primitive.attributes.find("TANGENT") != primitive.attributes.end()) {
			tangentData.emplace(scene, primitive.attributes.at("TANGENT"), std::set<int>{TINYGLTF_COMPONENT_TYPE_FLOAT}, std ::set<int>{TINYGLTF_TYPE_VEC4});
		}
	}

	if (normalData && normalData->accessor_.count != numVertices) {
		LOG_WARNING(log_system::MESH_LOADER, "Normal attribute buffer has different size than vertex buffer, ignoring it.");
		normalData.reset();
	}

	if (tangentData && tangentData->accessor_.count != numVertices) {
		LOG_WARNING(log_system::MESH_LOADER, "Tangent attribute buffer has different size than vertex buffer, ignoring it.");
		tangentData.reset();
	}

	if (normalData) {
		std::vector<float> normalScalingFactors;
		for (size_t vertexIndex = 0; vertexIndex < normalData->accessor_.count; vertexIndex++) {
			auto normal = normalData->getDataAt<float>(vertexIndex);
			if (globalNormalMatrix) {
				// The transformation of the normals changes the length so we have to normalize them again afterwards:
				// The scaling factor calculated here also needs to be applied to the morph target normals with the same vertex index
				// to assure that the direction of the weighted morphed normals do not change due to normalization.
				auto transformed = glm::vec3(*globalNormalMatrix * glm::dvec4(normal[0], normal[1], normal[2], 0.0));
				float normalScalingFactor = 1.0 / sqrt(glm::dot(transformed, transformed));
				//float normalScalingFactor = 1.0;
				normalScalingFactors.emplace_back(normalScalingFactor);
				auto normalized = normalScalingFactor * transformed;
				normalBuffer.insert(normalBuffer.end(), {static_cast<float>(normalized.x), static_cast<float>(normalized.y), static_cast<float>(normalized.z)});
			} else {
				normalBuffer.insert(normalBuffer.end(), {normal[0], normal[1], normal[2]});
			}

			if (tangentData) {
				auto tangent = tangentData->getDataAt<float>(vertexIndex);
				// Even though the GLTF file contains VEC4 tangents we convert to VEC3 in ramses since the 4th component is only
				// needed for the bitangent calculation below:
				convertVectorWithTransformation(tangent, tangentBuffer, globalModelMatrix, 0.0);

				auto bitangent = glm::cross(glm::vec3{normal[0], normal[1], normal[2]}, glm::vec3{tangent[0], tangent[1], tangent[2]}) * tangent[3];
				convertVectorWithTransformation({bitangent.x, bitangent.y, bitangent.z}, bitangentBuffer, globalModelMatrix, 0.0);
			}
		}

		morphNormalBuffers.resize(primitive.targets.size());
		for (size_t targetIndex = 0; targetIndex < primitive.targets.size(); targetIndex++) {
			auto it = primitive.targets[targetIndex].find("NORMAL");
			if (it != primitive.targets[targetIndex].end()) {
				glTFBufferData data(scene, it->second, std::set<int>{TINYGLTF_COMPONENT_TYPE_FLOAT}, std ::set<int>{TINYGLTF_TYPE_VEC3});

				if (data.accessor_.count == numVertices) {
					for (size_t index = 0; index < data.accessor_.count; index++) {
						auto normal = data.getDataAt<float>(index);
						if (globalNormalMatrix) {
							auto transformed = glm::vec3(*globalNormalMatrix * glm::dvec4(normal[0], normal[1], normal[2], 0.0));
							// Use the same scaling factor for the morph target normals as for the base normals to make sure the direction
							// of the morphed normals is not changed by normalization.
							auto normalized = normalScalingFactors[index] * transformed;
							morphNormalBuffers[targetIndex].insert(morphNormalBuffers[targetIndex].end(), {static_cast<float>(normalized.x), static_cast<float>(normalized.y), static_cast<float>(normalized.z)});
						} else {
							morphNormalBuffers[targetIndex].insert(morphNormalBuffers[targetIndex].end(), {normal[0], normal[1], normal[2]});
						}


					}
				} else {
					LOG_WARNING(log_system::MESH_LOADER, "Morph normal attribute has different size than vertex buffer, ignoring it.");
				}
			}
		}
	}

	convertAttributeSet(primitive, scene, uvBuffers, "TEXCOORD",
		std::set<int>{TINYGLTF_COMPONENT_TYPE_FLOAT, TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE, TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT},
		std::set<int>{TINYGLTF_TYPE_VEC2}, true, numVertices);

	convertAttributeSet(primitive, scene, colorBuffers, "COLOR",
		std::set<int>{TINYGLTF_COMPONENT_TYPE_FLOAT, TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE, TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT},
		std::set<int>{TINYGLTF_TYPE_VEC3, TINYGLTF_TYPE_VEC4}, true, numVertices);

	convertAttributeSet(primitive, scene, jointBuffers, "JOINTS",
		std::set<int>{TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE, TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT},
		std::set<int>{TINYGLTF_TYPE_VEC4}, false, numVertices);

	convertAttributeSet(primitive, scene, weightBuffers, "WEIGHTS",
		std::set<int>{TINYGLTF_COMPONENT_TYPE_FLOAT, TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE, TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT},
		std::set<int>{TINYGLTF_TYPE_VEC4}, true, numVertices);

	// Collect our faces/indexes
	// Note: we build the correct submesh ranges here in anticipation of submesh support in the meshnode.
	IndexBufferRangeInfo bufferRange = {static_cast<uint32_t>(indexBuffer_.size()), 0};

	if (primitive.indices > -1) {
		glTFBufferData indexBufferData(scene, primitive.indices, {TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT, TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT, TINYGLTF_TEXTURE_TYPE_UNSIGNED_BYTE}, std::set<int>{TINYGLTF_TYPE_SCALAR});
		auto indexAccessorCount = indexBufferData.accessor_.count;

		for (size_t index = 0; index < indexAccessorCount; index++) {
			auto data = indexBufferData.getConvertedData<uint32_t>(index, false).front();
			indexBuffer_.emplace_back(data + numVertices_);
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
