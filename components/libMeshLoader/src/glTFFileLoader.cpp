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

using namespace raco;

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
	trafos[2] = utils::math::quaternionToXYZDegrees(rotation.x, rotation.y, rotation.z, rotation.w);

	return trafos;
}

void unpackAnimationData(const std::vector<std::vector<float>>& data,
	size_t numKeyFrames,
	core::MeshAnimationInterpolation interpolation,
	core::EnginePrimitive componentType,
	std::vector<std::vector<float>>& keyFrames,
	std::vector<std::vector<float>>& tangentsIn,
	std::vector<std::vector<float>>& tangentsOut) {
	auto animInterpolationIsCubic = (interpolation == core::MeshAnimationInterpolation::CubicSpline) || (interpolation == core::MeshAnimationInterpolation::CubicSpline_Quaternion);

	if (!animInterpolationIsCubic) {
		if (componentType == core::EnginePrimitive::Array) {
			// Morph targets:
			// data buffer has numKeyFrames * number(morph targets) element vectors of size 1
			// we need to change this in numKeyFrames vector of length number(morph targets)
			auto numTargets = data.size() / numKeyFrames;
			assert(data.size() % numKeyFrames == 0);
			for (size_t i = 0; i < numKeyFrames; i++) {
				std::vector<float> vec;
				for (size_t target = 0; target < numTargets; target++) {
					vec.emplace_back(data[i * numTargets + target][0]);
				}
				keyFrames.emplace_back(vec);
			}
		} else {
			for (const auto& vecfKeyframe : data) {
				if (componentType == core::EnginePrimitive::Vec3f) {
					keyFrames.push_back({vecfKeyframe[0], vecfKeyframe[1], vecfKeyframe[2]});
				} else if (componentType == core::EnginePrimitive::Vec4f) {
					keyFrames.push_back({vecfKeyframe[0], vecfKeyframe[1], vecfKeyframe[2], vecfKeyframe[3]});
				}
			}
		}
	} else {
		if (componentType == core::EnginePrimitive::Array) {
			// Morph targets with cubic interpolation
			// buffer structure: each keyframe described by a_1 ... a_k v_1 ... v_k b_1 ... b_k
			// where 1...k are the morph targets,
			// a/b are the in/out tangents, and v are the values

			auto numTargets = data.size() / (3 * numKeyFrames);
			assert(data.size() % (3 * numKeyFrames) == 0);

			for (size_t i = 0; i < numKeyFrames; i++) {
				std::vector<float> tangentIn;
				std::vector<float> value;
				std::vector<float> tangentOut;

				for (size_t target = 0; target < numTargets; target++) {
					tangentIn.emplace_back(data[(3 * i + 0) * numTargets + target][0]);
					value.emplace_back(data[(3 * i + 1) * numTargets + target][0]);
					tangentOut.emplace_back(data[(3 * i + 2) * numTargets + target][0]);
				}

				tangentsIn.push_back(tangentIn);
				keyFrames.emplace_back(value);
				tangentsOut.push_back(tangentOut);
			}

		} else {
			for (auto i = 0; i < data.size(); i += 3) {
				auto& tangentIn = data[i];
				auto& vecfKeyframe = data[i + 1];
				auto& tangentOut = data[i + 2];

				if (componentType == core::EnginePrimitive::Vec3f) {
					tangentsIn.push_back({tangentIn[0], tangentIn[1], tangentIn[2]});
					keyFrames.push_back({vecfKeyframe[0], vecfKeyframe[1], vecfKeyframe[2]});
					tangentsOut.push_back({tangentOut[0], tangentOut[1], tangentOut[2]});
				} else if (componentType == core::EnginePrimitive::Vec4f) {
					tangentsIn.push_back({tangentIn[0], tangentIn[1], tangentIn[2], tangentIn[3]});
					keyFrames.push_back({vecfKeyframe[0], vecfKeyframe[1], vecfKeyframe[2], vecfKeyframe[3]});
					tangentsOut.push_back({tangentOut[0], tangentOut[1], tangentOut[2], tangentOut[3]});
				}
			}
		}
	}
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
	sceneGraph_.reset();
	importer_.reset();
	scene_.reset(new tinygltf::Model);
}

bool glTFFileLoader::buildglTFScenegraph() {
	sceneGraph_.reset(new core::MeshScenegraph);

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
	sceneGraph_->nodes = std::vector<std::optional<core::MeshScenegraphNode>>(nodes.size(), core::MeshScenegraphNode());
	std::vector<std::string> nodesAffectedByRamsesTrafo;

	for (auto nodeIndex = 0; nodeIndex < nodes.size(); ++nodeIndex) {
		auto& newNode = sceneGraph_->nodes[nodeIndex].value();
		auto& tinyNode = nodes[nodeIndex];

		newNode.name = tinyNode.name.empty() ? fmt::format("nodes_{}", nodeIndex) : tinyNode.name;
		if (tinyNode.mesh >= 0) {
			auto totalPrimAmountUpToThisPrim = tinyNode.mesh == 0 ? 0 : totalMeshPrimitiveSums[tinyNode.mesh - 1];
			for (auto currentMeshPrimIndex = 0; currentMeshPrimIndex < scene_->meshes[tinyNode.mesh].primitives.size(); ++currentMeshPrimIndex) {
				newNode.subMeshIndices.emplace_back(totalPrimAmountUpToThisPrim + currentMeshPrimIndex);
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
			auto eulerRotation = tinyNode.rotation.empty() ? std::array<double, 3>{} : utils::math::quaternionToXYZDegrees(tinyNode.rotation[0], tinyNode.rotation[1], tinyNode.rotation[2], tinyNode.rotation[3]);
			transferNodeTransformations(newNode.transformations.rotation, eulerRotation, 0.0);
			transferNodeTransformations(newNode.transformations.scale, tinyNode.scale, 1.0);
		}
	}

	importAnimations();
	importSkins();

	return true;
}

bool glTFFileLoader::importglTFScene(const std::string& absPath) {
	error_.clear();

	if (!importer_) {
		LOG_DEBUG(log_system::MESH_LOADER, "Create importer for: {}", absPath);
		importer_ = std::make_unique<tinygltf::TinyGLTF>();
		std::string err;
		std::string warn;

		if (utils::u8path(absPath).extension() == ".glb") {
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
				if (utils::file::isGitLfsPlaceholderFile(absPath)) {
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
	sceneGraph_->animations.resize(scene_->animations.size(), core::MeshAnimation{});
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

void glTFFileLoader::importSkins() {
	for (auto index = 0; index < scene_->skins.size(); index++) {
		const auto& skin = scene_->skins[index];
		std::string name = skin.name.empty() ? fmt::format("skin_{}", index) : skin.name;

		// Find node by searching through all nodes in scene
		auto it = std::find_if(scene_->nodes.begin(), scene_->nodes.end(), [index](const tinygltf::Node& node) {
			return index == node.skin;
		});
		if (it != scene_->nodes.end()) {
			int nodeIndex = it - scene_->nodes.begin();
			sceneGraph_->skins.emplace_back(core::SkinDescription{name, nodeIndex, skin.joints});
		}
	}
}

const core::MeshScenegraph* glTFFileLoader::getScenegraph(const std::string& absPath) {
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

core::SharedAnimationSamplerData glTFFileLoader::getAnimationSamplerData(const std::string& absPath, int animIndex, int samplerIndex) {
	if (!importglTFScene(absPath)) {
		return {};
	}

	if (animIndex < 0 || animIndex >= sceneGraph_->animations.size()) {
		return {};
	}

	if (samplerIndex < 0 || samplerIndex >= sceneGraph_->animationSamplers[animIndex].size()) {
		return {};
	}

	const auto& tinyAnim = scene_->animations[animIndex];
	const auto& sampler = tinyAnim.samplers[samplerIndex];

	core::MeshAnimationInterpolation interpolation = core::MeshAnimationInterpolation::Linear;
	if (sampler.interpolation == "LINEAR") {
		if (scene_->accessors[sampler.output].type == TINYGLTF_TYPE_VEC4) {
			// VEC4 must be a rotation animation in glTF so this must be a quaternion interpolation type:
			interpolation = core::MeshAnimationInterpolation::Linear_Quaternion;
		} else {
			interpolation = core::MeshAnimationInterpolation::Linear;
		}
	} else if (sampler.interpolation == "CUBICSPLINE") {
		if (scene_->accessors[sampler.output].type == TINYGLTF_TYPE_VEC4) {
			// VEC4 must be a rotation animation in glTF so this must be a quaternion interpolation type:
			interpolation = core::MeshAnimationInterpolation::CubicSpline_Quaternion;
		} else {
			interpolation = core::MeshAnimationInterpolation::CubicSpline;
		}
	} else if (sampler.interpolation == "STEP") {
		interpolation = core::MeshAnimationInterpolation::Step;
	} else {
		LOG_ERROR(log_system::MESH_LOADER, "animation sampler at index {}.{} has no valid interpolation type. Using linear interpolation as a fallback.", animIndex, samplerIndex);
	}

	auto inputData = glTFBufferData(*scene_, sampler.input, {TINYGLTF_COMPONENT_TYPE_FLOAT}, {TINYGLTF_TYPE_SCALAR});
	std::vector<float> input;
	for (auto count = 0; count < inputData.accessor_.count; ++count) {
		input.emplace_back(inputData.getDataAt<float>(count).front());
	}

	auto outputData = glTFBufferData(*scene_, sampler.output, {TINYGLTF_COMPONENT_TYPE_FLOAT, TINYGLTF_COMPONENT_TYPE_BYTE, TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE, TINYGLTF_COMPONENT_TYPE_SHORT, TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT}, {TINYGLTF_TYPE_SCALAR, TINYGLTF_TYPE_VEC3, TINYGLTF_TYPE_VEC4});
	std::vector<std::vector<float>> output;
	for (auto count = 0; count < outputData.accessor_.count; ++count) {
		output.emplace_back(outputData.getNormalizedData(count));
	}

	core::EnginePrimitive componentType;
	switch (output.front().size()) {
		case 1:
			componentType = core::EnginePrimitive::Array;
			break;
		case 3:
			componentType = core::EnginePrimitive::Vec3f;
			break;
		case 4:
			componentType = core::EnginePrimitive::Vec4f;
			break;
		default:
			assert(false);
	}

	std::vector<std::vector<float>> keyFrames;
	std::vector<std::vector<float>> tangentsIn;
	std::vector<std::vector<float>> tangentsOut;

	unpackAnimationData(output, input.size(), interpolation, componentType, keyFrames, tangentsIn, tangentsOut);

	return std::make_shared<core::AnimationSamplerData>(core::AnimationSamplerData{interpolation, componentType, input, keyFrames, tangentsIn, tangentsOut});
}

core::SharedMeshData glTFFileLoader::loadMesh(const core::MeshDescriptor& descriptor) {
	if (!importglTFScene(descriptor.absPath)) {
		return core::SharedMeshData();
	}
	auto meshCount = getTotalMeshCount();
	if (meshCount == 0) {
		error_ = "Mesh file contains no valid submeshes to select";
		return core::SharedMeshData();
	} else if (descriptor.bakeAllSubmeshes) {
		if (std::all_of(sceneGraph_->nodes.begin(), sceneGraph_->nodes.end(), [](const auto& node) { return node->subMeshIndices.empty(); })) {
			error_ = "Mesh file contains no bakeable submeshes.\nSubmeshes should be referenced by nodes to be bakeable.";
			return core::SharedMeshData();
		}
	}
	if (!descriptor.bakeAllSubmeshes && (descriptor.submeshIndex < 0 || descriptor.submeshIndex >= meshCount)) {
		error_ = "Selected submesh index is out of valid submesh index range [0," + std::to_string(meshCount - 1) + "]";
		return core::SharedMeshData();
	}

	return std::make_shared<glTFMesh>(*scene_, *sceneGraph_, descriptor);
}

core::SharedSkinData glTFFileLoader::loadSkin(const std::string& absPath, int skinIndex, std::string& outError) {
	if (!importglTFScene(absPath)) {
		outError = error_.empty() ? "Invalid GLTF file." : error_;
		return {};
	}

	if (scene_->skins.empty()) {
		outError = "GLTF file contains no skins.";
		return {};
	}

	if (skinIndex < 0 || skinIndex >= scene_->skins.size()) {
		outError = fmt::format("Skin index out of valid range [0,{}].", scene_->skins.size() - 1);
		return {};
	}

	auto& skin = scene_->skins[skinIndex];
	auto matrixData = glTFBufferData(*scene_, skin.inverseBindMatrices, {TINYGLTF_COMPONENT_TYPE_FLOAT}, {TINYGLTF_TYPE_MAT4});

	std::vector<glm::mat4x4> matrixBuffer;

	for (auto index = 0; index < matrixData.accessor_.count; index++) {
		auto m = matrixData.getDataArray<std::array<float, 16>>(index);
		matrixBuffer.emplace_back(glm::mat4x4(
			m[0], m[1], m[2], m[3],
			m[4], m[5], m[6], m[7],
			m[8], m[9], m[10], m[11],
			m[12], m[13], m[14], m[15]));
	}

	return std::make_shared<core::SkinData>(core::SkinData{matrixBuffer, scene_->skins.size()});
}

std::string glTFFileLoader::getError() {
	return error_;
}

}  // namespace raco::mesh_loader
