/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "mesh_loader/glTFFileLoader.h"

#include "mesh_loader/glTFMesh.h"

#include <cmath>
#include <assimp/DefaultLogger.hpp>
#include <assimp/Importer.hpp>
#include <assimp/Logger.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <log_system/log.h>

namespace {

class glTFImportLogger : public Assimp::Logger {
protected:
	bool attachStream(Assimp::LogStream*, unsigned int) override { return false; }
	bool detachStream(Assimp::LogStream*, unsigned int) override { return false; }
	void OnDebug(const char* message) override { LOG_TRACE(raco::log_system::MESH_LOADER, message); }
	void OnVerboseDebug(const char* message) override { LOG_DEBUG(raco::log_system::MESH_LOADER, message); }
	void OnError(const char* message) override { LOG_ERROR(raco::log_system::MESH_LOADER, message); }
	void OnInfo(const char* message) override { LOG_INFO(raco::log_system::MESH_LOADER, message); }
	void OnWarn(const char* message) override { LOG_WARNING(raco::log_system::MESH_LOADER, message); }
};

glTFImportLogger sImportLogger;

std::array<double, 3> rotationMatrixToXYZDegrees(aiMatrix3x3& mat) {
	// see: https://github.com/mrdoob/three.js/blob/master/src/math/Euler.js
	constexpr auto PI = 3.14159265358979323846;

	std::array<double, 3> degrees;
	auto& [degreeX, degreeY, degreeZ] = degrees;

	degreeY = std::asin(std::clamp(static_cast<double>(mat.a3), -1.0, 1.0)) / PI * 180;
	if (std::abs(mat.a3) < 0.9999999) {
		degreeX = std::atan2(-mat.b3, mat.c3) / PI * 180;
		degreeZ = std::atan2(-mat.a2, mat.a1) / PI * 180;

	} else {
		degreeX = std::atan2(mat.c2, mat.b2) / PI * 180;
		degreeZ = 0;
	}

	return degrees;
}

}  // namespace

namespace raco::mesh_loader {

glTFFileLoader::glTFFileLoader(std::string absPath) : path_(absPath) {
	if (&sImportLogger != Assimp::DefaultLogger::get()) {
		Assimp::DefaultLogger::set(&sImportLogger);
	}
}

void glTFFileLoader::reset() {
	error_.clear();
	bakedSceneImporter_.reset();
	unbakedSceneImporter_.reset();
	bakedScenegraph_.clear();
	unbakedScenegraph_.clear();
}

bool glTFFileLoader::buildglTFScenegraph(std::vector<raco::core::MeshScenegraphNode>& sceneGraph, int parentIndex, aiNode* child) {
	auto &newNode = sceneGraph.emplace_back();
	auto newNodeIndex = static_cast<int>(sceneGraph.size()) - 1;

	for (unsigned i = 0; i < child->mNumMeshes; ++i) {
		newNode.subMeshIndeces.emplace_back(child->mMeshes[i]);
	}
	newNode.parentIndex = parentIndex;
	newNode.name = child->mName.data;

	aiVector3t<float> scale;
	aiQuaterniont<float> rotation;
	aiVector3t<float> position;
	child->mTransformation.Decompose(scale, rotation, position);
	auto [scaleX, scaleY, scaleZ] = scale;
	if (scaleX < 0 || scaleY < 0 || scaleZ < 0) {
		error_ = fmt::format(
			"The imported node {} contains negative scale values [{},{},{}].\n\n"
			"When encountering negative scale values during a node import, Assimp flips all other positive scale values of that node and compensates with respective 180 degree rotations.\n"
			"This approach does not harmonize with how Ramses handles node scaling and rotation.\n\n"
			"As a preventive measure, importing nodes with negative scale values is temporarily prohibited.\n\n"
			"For a current status on this Assimp issue, see: {}",
			newNode.name, scaleX, scaleY, scaleZ, "https://github.com/assimp/assimp/issues/3784");
		return false;
	}

	rotation.Normalize();
	auto rotationMatrix = rotation.GetMatrix();

	auto rotationDegrees = ::rotationMatrixToXYZDegrees(rotationMatrix);
	newNode.transformations.scale = {scale.x, scale.y, scale.z};
	newNode.transformations.rotation = rotationDegrees;
	newNode.transformations.translation = {position.x, position.y, position.z};

	for (size_t i = 0; i < child->mNumChildren; ++i) {
		if (!buildglTFScenegraph(sceneGraph, newNodeIndex, child->mChildren[i])) {
			return false;
		}
	}

	return true;
}

bool glTFFileLoader::importglTFScene(const core::MeshDescriptor& descriptor, std::unique_ptr<Assimp::Importer>& importer, const aiScene*& scene, raco::core::MeshScenegraph& sceneGraph, unsigned flags) {
	if (!importer) {
		LOG_DEBUG(log_system::MESH_LOADER, "Create importer for: {}", descriptor.absPath);
		importer = std::make_unique<Assimp::Importer>();
		LOG_DEBUG(log_system::MESH_LOADER, "Import baked glTF scene: {}", descriptor.bakeAllSubmeshes);
		scene = importer->ReadFile(descriptor.absPath.c_str(), flags);
	}

	if (scene == nullptr || scene->mNumMeshes < 1) {
		error_ = importer->GetErrorString();
		if (error_.empty()) {
			LOG_ERROR(log_system::MESH_LOADER, "Encountered an error while loading glTF mesh {}", descriptor.absPath);
		} else {
			LOG_ERROR(log_system::MESH_LOADER, "Encountered an error while loading glTF mesh {}\n\tError: {}", descriptor.absPath, error_);
		}

		return false;
	}

	sceneGraph.clear();
	for (unsigned meshIndex = 0; meshIndex < scene->mNumMeshes; ++meshIndex) {
		sceneGraph.meshes.emplace_back(scene->mMeshes[meshIndex]->mName.C_Str());

		auto meshMaterialIndex = scene->mMeshes[meshIndex]->mMaterialIndex;
		sceneGraph.materials.emplace_back(scene->mMaterials[meshMaterialIndex]->GetName().C_Str());
	}

	if (!buildglTFScenegraph(sceneGraph.nodes, -1, scene->mRootNode)) {
		LOG_ERROR(log_system::MESH_LOADER, "Encountered an error while loading glTF mesh {}\n\tError: {}", descriptor.absPath, error_);
		sceneGraph.clear();
		return false;
	}

	return true;
}

raco::core::MeshScenegraph glTFFileLoader::getScenegraph(bool bakeAllSubmeshes) {
	return bakeAllSubmeshes ? bakedScenegraph_ : unbakedScenegraph_;
}

int glTFFileLoader::getTotalMeshCount(bool bakeAllSubmeshes) {
	if (auto* sceneToCheck = bakeAllSubmeshes ? bakedScene_ : unbakedScene_) {
		return static_cast<int>(sceneToCheck->mNumMeshes);
	}
	return 0;
}

raco::core::SharedMeshData glTFFileLoader::loadMesh(const core::MeshDescriptor& descriptor) {
	auto &activeImporter = descriptor.bakeAllSubmeshes ? bakedSceneImporter_ : unbakedSceneImporter_;
	auto &sceneToImport = descriptor.bakeAllSubmeshes ? bakedScene_ : unbakedScene_;
	auto &sceneGraph = descriptor.bakeAllSubmeshes ? bakedScenegraph_ : unbakedScenegraph_;
	auto activeFlags = aiPostProcessSteps::aiProcess_Triangulate | aiPostProcessSteps::aiProcess_GenNormals;
	if (descriptor.bakeAllSubmeshes) {
		activeFlags |= aiPostProcessSteps::aiProcess_PreTransformVertices;
	}

	if (!importglTFScene(descriptor, activeImporter, sceneToImport, sceneGraph, activeFlags)) {
		return raco::core::SharedMeshData();
	}
	if (!descriptor.bakeAllSubmeshes && (descriptor.submeshIndex < 0 || descriptor.submeshIndex >= static_cast<int>(sceneToImport->mNumMeshes))) {
		error_ = "Selected submesh index is out of valid submesh index range [0," + std::to_string(sceneToImport->mNumMeshes - 1) + "]";
		return raco::core::SharedMeshData();
	}
	return std::make_shared<glTFMesh>(*sceneToImport, descriptor);
}

std::string glTFFileLoader::getError() {
	return error_;
}

} // namespace raco::mesh_loader
