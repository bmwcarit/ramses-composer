/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define GLM_FORCE_XYZW_ONLY

#include "mesh_loader/glTFFileLoader.h"

#include "mesh_loader/glTFBufferData.h"
#include "mesh_loader/glTFMesh.h"
#include "utils/FileUtils.h"
#include "utils/MathUtils.h"
#include "utils/u8path.h"

#include <glm/ext/quaternion_double.hpp>
#include <glm/gtc/epsilon.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/mat4x4.hpp>
#include <log_system/log.h>

namespace {

std::array<std::array<double, 3>, 3> tinyglTFtrafoMatrixToXYZTrafos(const std::vector<double>& tinyMatrix) {
	assert(tinyMatrix.size() == 16);

	std::array<std::array<double, 3>, 3> trafos;

	glm::dmat4 trafoMatrix(tinyMatrix[0], tinyMatrix[1], tinyMatrix[2], tinyMatrix[3],
		tinyMatrix[4], tinyMatrix[5], tinyMatrix[6], tinyMatrix[7],
		tinyMatrix[8], tinyMatrix[9], tinyMatrix[10], tinyMatrix[11],
		tinyMatrix[12], tinyMatrix[13], tinyMatrix[14], tinyMatrix[15]);
	glm::dvec3 scale;
	glm::dquat rotation{0, 0, 0, 0};
	glm::dvec3 translation;
	glm::dvec3 skew;
	glm::dvec4 perspective;
	glm::decompose(trafoMatrix, scale, rotation, translation, skew, perspective);

	trafos[0] = {translation.x, translation.y, translation.z};
	trafos[1] = {scale.x, scale.y, scale.z};
	trafos[2] = raco::utils::math::quaternionToXYZDegrees(rotation.x, rotation.y, rotation.z, rotation.w);

	return trafos;
}
}


namespace raco::mesh_loader {

glTFFileLoader::glTFFileLoader(std::string absPath)
	: path_(absPath),
	  scene_(new tinygltf::Model),
	  importer_(nullptr) {
}

glTFFileLoader::~glTFFileLoader() {
	reset();
}

void glTFFileLoader::reset() {
	error_.clear();
	sceneGraph_.reset();
	importer_.reset();
	scene_.reset(new tinygltf::Model);
}

bool glTFFileLoader::buildglTFScenegraph() {
	sceneGraph_.reset(new raco::core::MeshScenegraph);

	// import nodes
	std::vector<int> totalMeshPrimitiveSums(scene_->meshes.size());
	for (auto meshIndex = 0; meshIndex < scene_->meshes.size(); ++meshIndex) {
		const auto& mesh = scene_->meshes[meshIndex];
		auto meshName = mesh.name.empty() ? fmt::format("mesh_{}", meshIndex) : mesh.name;

		totalMeshPrimitiveSums[meshIndex] = meshIndex == 0 ? mesh.primitives.size() : mesh.primitives.size() + totalMeshPrimitiveSums[meshIndex - 1];

		for (auto primitiveIndex = 0; primitiveIndex < mesh.primitives.size(); ++primitiveIndex) {
			const auto& primitive = mesh.primitives[primitiveIndex];
			if (mesh.primitives.size() == 1) {
				sceneGraph_->meshes.emplace_back(meshName);
			} else {
				sceneGraph_->meshes.emplace_back(fmt::format("{}.{}", meshName, primitiveIndex));
			}
			if (primitive.material >= 0) {
				auto meshMaterial = scene_->materials[primitive.material];

				auto meshMaterialName = meshMaterial.name.empty() ? fmt::format("material_{}", primitive.material) : meshMaterial.name;
				sceneGraph_->materials.emplace_back(meshMaterialName);
			} else {
				sceneGraph_->materials.emplace_back();
			}
		}
	}

	// import meshes - we are currently replicating Assimp's primitive-> mesh behavior
	auto& nodes = scene_->nodes;
	sceneGraph_->nodes = std::vector<std::optional<raco::core::MeshScenegraphNode>>(nodes.size(), raco::core::MeshScenegraphNode());
	std::vector<std::string> nodesAffectedByRamsesTrafo;

	for (auto nodeIndex = 0; nodeIndex < nodes.size(); ++nodeIndex) {
		auto& newNode = sceneGraph_->nodes[nodeIndex].value();
		auto& tinyNode = nodes[nodeIndex];

		newNode.name = tinyNode.name.empty() ? fmt::format("nodes_{}", nodeIndex) : tinyNode.name;
		if (tinyNode.mesh >= 0) {
			auto totalPrimAmountUpToThisPrim = tinyNode.mesh == 0 ? 0 : totalMeshPrimitiveSums[tinyNode.mesh - 1];
			for (auto currentMeshPrimIndex = 0; currentMeshPrimIndex < scene_->meshes[tinyNode.mesh].primitives.size(); ++currentMeshPrimIndex) {
				newNode.subMeshIndeces.emplace_back(totalPrimAmountUpToThisPrim + currentMeshPrimIndex);
			}
		}

		for (const auto& child : tinyNode.children) {
			sceneGraph_->nodes[child]->parentIndex = nodeIndex;
		}

		auto transferNodeTransformations = [](auto& newNodeTrafoArray, const auto& tinyNodeTrafoVec, const auto defaultValue) {
			if (!tinyNodeTrafoVec.empty()) {
				newNodeTrafoArray = {tinyNodeTrafoVec[0], tinyNodeTrafoVec[1], tinyNodeTrafoVec[2]};
			} else {
				newNodeTrafoArray = {defaultValue, defaultValue, defaultValue};
			}
		};
		if (!tinyNode.matrix.empty()) {
			auto trafos = tinyglTFtrafoMatrixToXYZTrafos(tinyNode.matrix);

			newNode.transformations.translation = trafos[0];
			newNode.transformations.scale = trafos[1];
			newNode.transformations.rotation = trafos[2];
		} else {
			transferNodeTransformations(newNode.transformations.translation, tinyNode.translation, 0.0);
			auto eulerRotation = tinyNode.rotation.empty() ? std::array<double, 3>{} : raco::utils::math::quaternionToXYZDegrees(tinyNode.rotation[0], tinyNode.rotation[1], tinyNode.rotation[2], tinyNode.rotation[3]);
			transferNodeTransformations(newNode.transformations.rotation, eulerRotation, 0.0);
			transferNodeTransformations(newNode.transformations.scale, tinyNode.scale, 1.0);
		}
	}

	importAnimations();

	return true;
}

bool glTFFileLoader::importglTFScene(const std::string& absPath) {
	error_.clear();

	if (!importer_) {
		LOG_DEBUG(log_system::MESH_LOADER, "Create importer for: {}", absPath);
		importer_ = std::make_unique<tinygltf::TinyGLTF>();
		std::string err;
		std::string warn;

		if (raco::utils::u8path(absPath).extension() == ".glb") {
			importer_->LoadBinaryFromFile(&*scene_, &err, &warn, absPath);
		} else {
			importer_->LoadASCIIFromFile(&*scene_, &err, &warn, absPath);
		}
		if (!warn.empty()) {
			LOG_WARNING(log_system::MESH_LOADER, "Encountered warnings while loading glTF mesh {}: {}", absPath, warn);
		}
		if (!err.empty()) {
			error_ = err;
			if (error_.find("parse_error") != std::string::npos || error_ == "Invalid magic.") {
				if (raco::utils::file::isGitLfsPlaceholderFile(absPath)) {
					error_ = "Git LFS placeholder file detected.";
				}
			}
			LOG_ERROR(log_system::MESH_LOADER, "Encountered an error while loading glTF mesh {}\n\tError: {}", absPath, error_);
			importer_.reset();
			return false;
		}

		if (!buildglTFScenegraph()) {
			LOG_ERROR(log_system::MESH_LOADER, "Encountered an error while loading glTF mesh {}\n\tError: {}", absPath, error_);
			importer_.reset();
			sceneGraph_.reset();
			return false;
		}
	}
	return true;
}

void glTFFileLoader::importAnimations() {
	sceneGraph_->animations.resize(scene_->animations.size(), raco::core::MeshAnimation{});
	sceneGraph_->animationSamplers.resize(scene_->animations.size());

	for (auto animIndex = 0; animIndex < scene_->animations.size(); ++animIndex) {
		auto tinyAnim = scene_->animations[animIndex];
		auto& anim = sceneGraph_->animations[animIndex];
		auto& animSamplers = sceneGraph_->animationSamplers[animIndex];
		animSamplers.resize(tinyAnim.samplers.size());
		anim->name = tinyAnim.name.empty() ? fmt::format("animation_{}", animIndex) : tinyAnim.name;

		for (auto samplerIndex = 0; samplerIndex < tinyAnim.samplers.size(); ++samplerIndex) {
			const auto& sampler = tinyAnim.samplers[samplerIndex];
			auto& newSampler = animSamplers[samplerIndex];

			newSampler = fmt::format("{}.ch{}", anim->name, samplerIndex);
		}

		for (const auto& channel : tinyAnim.channels) {
			auto& newChannel = anim->channels.emplace_back();
			newChannel.targetPath = channel.target_path;
			newChannel.samplerIndex = channel.sampler;
			newChannel.nodeIndex = channel.target_node;
		}
	}
}

raco::core::MeshScenegraph* glTFFileLoader::getScenegraph(const std::string& absPath) {
	if (!importglTFScene(absPath)) {
		return nullptr;
	}
	return sceneGraph_.get();
}

int glTFFileLoader::getTotalMeshCount() {
	auto primitiveCount = 0;
	for (const auto& mesh : scene_->meshes) {
		primitiveCount += mesh.primitives.size();
	}

	return primitiveCount;
}

std::shared_ptr<raco::core::MeshAnimationSamplerData> glTFFileLoader::getAnimationSamplerData(const std::string& absPath, int animIndex, int samplerIndex) {
	if (!importglTFScene(absPath)) {
		return {};
	}

	if (animIndex < 0 || animIndex >= sceneGraph_->animations.size()) {
		return {};
	}

	if (samplerIndex < 0 || samplerIndex >= sceneGraph_->animationSamplers[animIndex].size()) {
		return {};
	}

	raco::core::MeshAnimationInterpolation interpolation = raco::core::MeshAnimationInterpolation::Linear;
	std::vector<float> input;
	std::vector<std::vector<float>> output;

	const auto& tinyAnim = scene_->animations[animIndex];
	const auto& sampler = tinyAnim.samplers[samplerIndex];

	if (sampler.interpolation == "CUBICSPLINE") {
		interpolation = raco::core::MeshAnimationInterpolation::CubicSpline;
	} else if (sampler.interpolation == "STEP") {
		interpolation = raco::core::MeshAnimationInterpolation::Step;
	} else if (sampler.interpolation != "LINEAR") {
		LOG_ERROR(log_system::MESH_LOADER, "animation sampler at index {}.{} has no valid interpolation type. Using linear interpolation as a fallback.", animIndex, samplerIndex);
	}

	auto inputData = glTFBufferData(*scene_, sampler.input, {TINYGLTF_COMPONENT_TYPE_FLOAT});
	for (auto count = 0; count < inputData.accessor_.count; ++count) {
		input.emplace_back(inputData.getDataAt<float>(count).front());
	}

	auto outputData = glTFBufferData(*scene_, sampler.output, {TINYGLTF_COMPONENT_TYPE_FLOAT, TINYGLTF_COMPONENT_TYPE_BYTE, TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE, TINYGLTF_COMPONENT_TYPE_SHORT, TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT});
	for (auto count = 0; count < outputData.accessor_.count; ++count) {
		switch (outputData.accessor_.componentType) {
			case TINYGLTF_COMPONENT_TYPE_FLOAT: {
				auto data = outputData.getDataAt<float>(count);

				output.emplace_back(data);
				break;
			}
			// use int-to-float conversion from glTF spec
			// https://github.com/KhronosGroup/glTF/blob/main/specification/2.0/Specification.adoc#311-animations
			case TINYGLTF_COMPONENT_TYPE_BYTE: {
				auto data = outputData.getDataAt<char>(count);
				std::vector<float> convertedData(data.size());
				for (auto i = 0; i < data.size(); ++i) {
					convertedData[i] = std::max(data[i] / 127.0F, -1.0F);
				}

				output.emplace_back(convertedData);
				break;
			}
			case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE: {
				auto data = outputData.getDataAt<unsigned char>(count);
				std::vector<float> convertedData(data.size());
				for (auto i = 0; i < data.size(); ++i) {
					convertedData[i] = data[i] / 255.0F;
				}

				output.emplace_back(convertedData);
				break;
			}
			case TINYGLTF_COMPONENT_TYPE_SHORT: {
				auto data = outputData.getDataAt<short>(count);
				std::vector<float> convertedData(data.size());
				for (auto i = 0; i < data.size(); ++i) {
					convertedData[i] = std::max(data[i] / 32767.0F, -1.0F);
				}

				output.emplace_back(convertedData);
				break;
			}
			case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: {
				auto data = outputData.getDataAt<unsigned short>(count);
				std::vector<float> convertedData(data.size());
				for (auto i = 0; i < data.size(); ++i) {
					convertedData[i] = data[i] / 65535.0F;
				}

				output.emplace_back(convertedData);
				break;
			}
			default:
				LOG_ERROR(log_system::MESH_LOADER, "animation sampler at index {}.{} has invalid component type", animIndex, samplerIndex);
				continue;
		}
	}

	return std::make_shared<raco::core::MeshAnimationSamplerData>(raco::core::MeshAnimationSamplerData{interpolation, input, output});
}

raco::core::SharedMeshData glTFFileLoader::loadMesh(const core::MeshDescriptor& descriptor) {
	if (!importglTFScene(descriptor.absPath)) {
		return raco::core::SharedMeshData();
	}
	auto meshCount = getTotalMeshCount();
	if (meshCount == 0) {
		error_ = "Mesh file contains no valid submeshes to select";
		return raco::core::SharedMeshData();
	} else if (descriptor.bakeAllSubmeshes) {
		if (std::all_of(sceneGraph_->nodes.begin(), sceneGraph_->nodes.end(), [](const auto& node) { return node->subMeshIndeces.empty(); })) {
			error_ = "Mesh file contains no bakeable submeshes.\nSubmeshes should be referenced by nodes to be bakeable.";
			return raco::core::SharedMeshData();
		}
	}
	if (!descriptor.bakeAllSubmeshes && (descriptor.submeshIndex < 0 || descriptor.submeshIndex >= meshCount)) {
		error_ = "Selected submesh index is out of valid submesh index range [0," + std::to_string(meshCount - 1) + "]";
		return raco::core::SharedMeshData();
	}

	return std::make_shared<glTFMesh>(*scene_, *sceneGraph_, descriptor);
}

std::string glTFFileLoader::getError() {
	return error_;
}

} // namespace raco::mesh_loader
