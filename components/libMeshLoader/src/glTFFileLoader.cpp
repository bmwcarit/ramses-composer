/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define GLM_FORCE_XYZW_ONLY

#include "mesh_loader/glTFFileLoader.h"

#include "mesh_loader/glTFMesh.h"
#include "utils/stdfilesystem.h"

#include <log_system/log.h>
#include <tiny_gltf.h>
#include <glm/gtc/quaternion.hpp>
#include <glm/ext/quaternion_double.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/epsilon.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/quaternion.hpp>


namespace {

constexpr auto PI = 3.14159265358979323846;

std::array<double, 3> quaternionToXYZDegrees(double qX, double qY, double qZ, double qW) {
	auto mat = glm::toMat3(glm::normalize(glm::dquat(qW, qX, qY, qZ)));
	std::array<double, 3> degrees;
	auto& [degreeX, degreeY, degreeZ] = degrees;

	degreeY = std::asin(std::clamp(static_cast<double>(mat[2].x), -1.0, 1.0)) / PI * 180;
	if (std::abs(mat[2][0]) < 0.99999) {
		degreeX = std::atan2(-mat[2].y, mat[2].z) / PI * 180;
		degreeZ = std::atan2(-mat[1].x, mat[0].x) / PI * 180;

	} else {
		degreeX = std::atan2(mat[1].z, mat[1].y) / PI * 180;
		degreeZ = 0;
	}


	return {degreeX, degreeY, degreeZ};
}

// see: https://github.com/KhronosGroup/glTF-Tutorials/blob/master/gltfTutorial/gltfTutorial_004_ScenesNodes.md#local-and-global-transforms
raco::core::MeshScenegraphNode::Transformations trafoMatrixToXYZTrafos(const std::vector<double>& tinyMatrix) {
	assert(tinyMatrix.size() == 16);

	raco::core::MeshScenegraphNode::Transformations trafos;

	glm::dmat4 trafoMatrix(tinyMatrix[0], tinyMatrix[1], tinyMatrix[2], tinyMatrix[3],
		tinyMatrix[4], tinyMatrix[5], tinyMatrix[6], tinyMatrix[7],
		tinyMatrix[8], tinyMatrix[9], tinyMatrix[10], tinyMatrix[11],
		tinyMatrix[12], tinyMatrix[13], tinyMatrix[14], tinyMatrix[15]);
	glm::dvec3 scale;
	glm::dquat rotation{0,0,0,0};
	glm::dvec3 translation;
	glm::dvec3 skew;
	glm::dvec4 perspective;
	glm::decompose(trafoMatrix, scale, rotation, translation, skew, perspective);

	trafos.translation = {translation.x, translation.y, translation.z};
	trafos.scale = {scale.x, scale.y, scale.z};
	trafos.rotation = quaternionToXYZDegrees(rotation.x, rotation.y, rotation.z, rotation.w);

	return trafos;
}

}  // namespace

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
	sceneGraph_.clear();
	importer_.reset();
}

bool glTFFileLoader::buildglTFScenegraph(const std::vector<int>& totalMeshPrimitiveSums) {
	constexpr auto EPSILON = 0.0001;

	auto& nodes = scene_->nodes;
	sceneGraph_.nodes = std::vector<std::optional<raco::core::MeshScenegraphNode>>(nodes.size(), raco::core::MeshScenegraphNode());
	std::vector<std::string> nodesAffectedByRamsesTrafo;

	for (auto nodeIndex = 0; nodeIndex < nodes.size(); ++nodeIndex) {
		auto& newNode = sceneGraph_.nodes[nodeIndex].value();
		auto& tinyNode = nodes[nodeIndex];

		newNode.name = tinyNode.name.empty() ? fmt::format("nodes_{}", nodeIndex) : tinyNode.name;
		if (tinyNode.mesh >= 0) {
			auto totalPrimAmountUpToThisPrim = tinyNode.mesh == 0 ? 0 : totalMeshPrimitiveSums[tinyNode.mesh - 1];
			for (auto currentMeshPrimIndex = 0; currentMeshPrimIndex < scene_->meshes[tinyNode.mesh].primitives.size(); ++currentMeshPrimIndex) {
				newNode.subMeshIndeces.emplace_back(totalPrimAmountUpToThisPrim + currentMeshPrimIndex);
			}
		}

		for (const auto& child : tinyNode.children) {
			sceneGraph_.nodes[child]->parentIndex = nodeIndex;
		}

		auto transferNodeTransformations = [](auto& newNodeTrafoArray, const auto& tinyNodeTrafoVec, const auto defaultValue) {
			if (!tinyNodeTrafoVec.empty()) {
				newNodeTrafoArray = {tinyNodeTrafoVec[0], tinyNodeTrafoVec[1], tinyNodeTrafoVec[2]};
			} else {
				newNodeTrafoArray = {defaultValue, defaultValue, defaultValue};
			}
		};
		if (!tinyNode.matrix.empty()) {
			newNode.transformations = trafoMatrixToXYZTrafos(tinyNode.matrix);
		} else {
			transferNodeTransformations(newNode.transformations.translation, tinyNode.translation, 0.0);
			auto eulerRotation = tinyNode.rotation.empty() ? std::array<double, 3>{} : quaternionToXYZDegrees(tinyNode.rotation[0], tinyNode.rotation[1], tinyNode.rotation[2], tinyNode.rotation[3]);
			transferNodeTransformations(newNode.transformations.rotation, eulerRotation, 0.0);
			transferNodeTransformations(newNode.transformations.scale, tinyNode.scale, 1.0);
		}
	}

	return true;
}

bool glTFFileLoader::importglTFScene(const core::MeshDescriptor& descriptor) {
	error_.clear();

	if (!importer_) {
		LOG_DEBUG(log_system::MESH_LOADER, "Create importer for: {}", descriptor.absPath);
		importer_ = std::make_unique<tinygltf::TinyGLTF>();
		std::string err;
		std::string warn;
		LOG_DEBUG(log_system::MESH_LOADER, "Import baked glTF scene: {}", descriptor.bakeAllSubmeshes);

		if (std::filesystem::path(descriptor.absPath).extension() == ".glb") {
			importer_->LoadBinaryFromFile(&*scene_, &err, &warn, descriptor.absPath);
		} else {
			importer_->LoadASCIIFromFile(&*scene_, &err, &warn, descriptor.absPath);
		}
		if (!warn.empty()) {
			LOG_WARNING(log_system::MESH_LOADER, "Encountered warnings while loading glTF mesh {}: {}", descriptor.absPath, warn);
		}
		if (!err.empty()) {
			error_ = err;
		}
	}

	if (!error_.empty() || scene_->meshes.empty()) {
		LOG_ERROR(log_system::MESH_LOADER, "Encountered an error while loading glTF mesh {}\n\tError: {}", descriptor.absPath, error_);		
		return false;
	}

	sceneGraph_.clear();
	std::vector<int> totalMeshPrimitiveSums(scene_->meshes.size());
	for (auto meshIndex = 0; meshIndex < scene_->meshes.size(); ++meshIndex) {
		const auto& mesh = scene_->meshes[meshIndex];
		auto meshName = mesh.name.empty() ? fmt::format("mesh_{}", meshIndex) : mesh.name;

		totalMeshPrimitiveSums[meshIndex] = meshIndex == 0 ? mesh.primitives.size() : mesh.primitives.size() + totalMeshPrimitiveSums[meshIndex - 1];

		for (auto primitiveIndex = 0; primitiveIndex < mesh.primitives.size(); ++primitiveIndex) {
			const auto& primitive = mesh.primitives[primitiveIndex];
			if (mesh.primitives.size() == 1) {
				sceneGraph_.meshes.emplace_back(meshName);
			} else {
				sceneGraph_.meshes.emplace_back(fmt::format("{}.{}", meshName, primitiveIndex));
			}
			if (primitive.material >= 0) {
				auto meshMaterial = scene_->materials[primitive.material];

				auto meshMaterialName = meshMaterial.name.empty() ? fmt::format("material_{}", primitive.material) : meshMaterial.name;
				sceneGraph_.materials.emplace_back(meshMaterialName);
			} else {
				sceneGraph_.materials.emplace_back();
			}
		}
	}

	if (!buildglTFScenegraph(totalMeshPrimitiveSums)) {
		LOG_ERROR(log_system::MESH_LOADER, "Encountered an error while loading glTF mesh {}\n\tError: {}", descriptor.absPath, error_);
		sceneGraph_.clear();
		return false;
	}

	if (descriptor.bakeAllSubmeshes) {
		return true;
	}

	for (auto animIndex = 0; animIndex < scene_->animations.size(); ++animIndex) {
		auto tinyAnim = scene_->animations[animIndex];
		auto& anim = sceneGraph_.animations.emplace_back();
		anim.name = tinyAnim.name.empty() ? fmt::format("animation_{}", animIndex) : tinyAnim.name;

		for (const auto& sampler : tinyAnim.samplers) {
			auto& newSampler = anim.samplers.emplace_back();
			newSampler.interpolation = sampler.interpolation;
			newSampler.inputIndex = sampler.input;
			newSampler.outputIndex = sampler.output;
		}

		for (const auto& channel : tinyAnim.channels) {
			auto& newChannel = anim.channels.emplace_back();
			newChannel.targetPath = channel.target_path;
			newChannel.samplerIndex = channel.sampler;
			newChannel.nodeIndex = channel.target_node;
		}
	}

	return true;
}

raco::core::MeshScenegraph glTFFileLoader::getScenegraph() {
	return sceneGraph_;
}

int glTFFileLoader::getTotalMeshCount() {
	auto primitiveCount = 0;
	for (const auto& mesh : scene_->meshes) {
		primitiveCount += mesh.primitives.size();
	}

	return primitiveCount;
}

raco::core::SharedMeshData glTFFileLoader::loadMesh(const core::MeshDescriptor& descriptor) {
	if (!importglTFScene(descriptor)) {
		return raco::core::SharedMeshData();
	}
	if (!descriptor.bakeAllSubmeshes && (descriptor.submeshIndex < 0 || descriptor.submeshIndex >= getTotalMeshCount())) {
		error_ = "Selected submesh index is out of valid submesh index range [0," + std::to_string(getTotalMeshCount() - 1) + "]";
		return raco::core::SharedMeshData();
	}
	return std::make_shared<glTFMesh>(*scene_, sceneGraph_, descriptor);
}

std::string glTFFileLoader::getError() {
	return error_;
}

std::string glTFFileLoader::getWarning() {
	return warning_;
}

} // namespace raco::mesh_loader
