/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include <gtest/gtest.h>

#include "mesh_loader/CTMFileLoader.h"
#include "mesh_loader/glTFFileLoader.h"
#include "testing/RacoBaseTest.h"
#include "testing/TestEnvironmentCore.h"
#include "testing/TestUtil.h"

using namespace raco;

class MeshLoaderTest : public TestEnvironmentCore {
public:
	core::SharedMeshData loadMesh(const std::string &relPath, bool bake = true, int submeshIndex = 0) {
		core::MeshDescriptor desc{test_path().append(relPath).string(), submeshIndex, bake};
		mesh_loader::glTFFileLoader fileloader(desc.absPath);
		return fileloader.loadMesh(desc);
	}

	static void check_attribute(core::SharedMeshData mesh, const std::string &name, core::MeshData::VertexAttribDataType type, int elemCount) {
		int index = mesh->attribIndex(name);
		ASSERT_TRUE(index >= 0);

		ASSERT_EQ(mesh->attribDataType(index), type);
		ASSERT_EQ(mesh->attribElementCount(index), elemCount);
	}

protected:
	std::vector<float> getPositionData(const core::SharedMeshData mesh) {
		auto posIndex = mesh->attribIndex(mesh->ATTRIBUTE_POSITION);
		auto firstPos = mesh->attribBuffer(posIndex);

		auto posElementAmount = mesh->attribElementCount(posIndex);
		std::vector<float> posData(posElementAmount * 3);
		std::memcpy(&posData[0], firstPos, posElementAmount * 3 * sizeof(float));

		return posData;
	};
};

TEST_F(MeshLoaderTest, glTFLoadBaked) {
	core::MeshDescriptor desc;
	desc.absPath = test_path().append("meshes/CesiumMilkTruck/CesiumMilkTruck.gltf").string();
	desc.bakeAllSubmeshes = true;

	mesh_loader::glTFFileLoader fileloader(desc.absPath);
	auto loadedMesh = fileloader.loadMesh(desc);

	auto submeshIndexBufferRanges = loadedMesh->submeshIndexBufferRanges();
	auto totalSubmeshIndexBufferRangeCount = std::accumulate(submeshIndexBufferRanges.begin(), submeshIndexBufferRanges.end(), 0, [](int sum, auto &info) { return sum + info.count; });
	ASSERT_EQ(loadedMesh->getIndices().size(), totalSubmeshIndexBufferRangeCount);
	ASSERT_EQ(loadedMesh->numSubmeshes(), 1);  // TODO should be 4 with full submesh support
	ASSERT_EQ(fileloader.getTotalMeshCount(), 4);
}

TEST_F(MeshLoaderTest, glTFLoadUnbaked) {
	core::MeshDescriptor desc;
	desc.absPath = test_path().append("meshes/CesiumMilkTruck/CesiumMilkTruck.gltf").string();
	desc.bakeAllSubmeshes = false;
	desc.submeshIndex = 1;

	mesh_loader::glTFFileLoader fileloader(desc.absPath);
	auto loadedMesh = fileloader.loadMesh(desc);

	ASSERT_EQ(loadedMesh->getIndices().size(), loadedMesh->submeshIndexBufferRanges().front().count);
	ASSERT_EQ(loadedMesh->numSubmeshes(), 1);
	ASSERT_EQ(fileloader.getTotalMeshCount(), 4);
}

TEST_F(MeshLoaderTest, glTFLoadUnbakedThenBaked) {
	core::MeshDescriptor desc;
	desc.absPath = test_path().append("meshes/CesiumMilkTruck/CesiumMilkTruck.gltf").string();
	desc.bakeAllSubmeshes = false;
	desc.submeshIndex = 1;

	mesh_loader::glTFFileLoader fileloader(desc.absPath);
	auto unbakedMesh = fileloader.loadMesh(desc);

	desc.bakeAllSubmeshes = true;
	auto bakedMesh = fileloader.loadMesh(desc);
	auto submeshIndexBufferRanges = bakedMesh->submeshIndexBufferRanges();
	auto totalSubmeshIndexBufferRangeCount = std::accumulate(submeshIndexBufferRanges.begin(), submeshIndexBufferRanges.end(), 0, [](int sum, auto &info) { return sum + info.count; });

	ASSERT_EQ(unbakedMesh->getIndices().size(), unbakedMesh->submeshIndexBufferRanges().front().count);
	ASSERT_EQ(bakedMesh->getIndices().size(), totalSubmeshIndexBufferRangeCount);
	ASSERT_EQ(unbakedMesh->numSubmeshes(), 1);
	ASSERT_EQ(bakedMesh->numSubmeshes(), 1);  // TODO should be 4 with full submesh support
	ASSERT_EQ(fileloader.getTotalMeshCount(), 4);
}

TEST_F(MeshLoaderTest, glTFLoadBakedThenUnbakedThenBakedCorrectPositionData) {
	core::MeshDescriptor desc;
	desc.absPath = test_path().append("meshes/CesiumMilkTruck/CesiumMilkTruck.gltf").string();
	desc.bakeAllSubmeshes = true;
	desc.submeshIndex = 0;

	mesh_loader::glTFFileLoader fileloader(desc.absPath);
	auto firstUnbakedMesh = fileloader.loadMesh(desc);
	auto firstUnbakedPosData = getPositionData(firstUnbakedMesh);

	desc.bakeAllSubmeshes = false;
	auto bakedMesh = fileloader.loadMesh(desc);

	desc.bakeAllSubmeshes = true;
	auto secondUnbakedMesh = fileloader.loadMesh(desc);
	auto secondUnbakedPosData = getPositionData(secondUnbakedMesh);

	ASSERT_EQ(firstUnbakedPosData, secondUnbakedPosData);
}

TEST_F(MeshLoaderTest, glTFLoadUnbakedThenBakedThenUnbakedCorrectPositionData) {
	core::MeshDescriptor desc;
	desc.absPath = test_path().append("meshes/CesiumMilkTruck/CesiumMilkTruck.gltf").string();
	desc.bakeAllSubmeshes = false;
	desc.submeshIndex = 0;

	mesh_loader::glTFFileLoader fileloader(desc.absPath);
	auto firstBakedMesh = fileloader.loadMesh(desc);
	auto firstBakedPosData = getPositionData(firstBakedMesh);

	desc.bakeAllSubmeshes = true;
	auto unbakedMesh = fileloader.loadMesh(desc);

	desc.bakeAllSubmeshes = false;
	auto secondBakedMesh = fileloader.loadMesh(desc);
	auto secondBakedPosData = getPositionData(secondBakedMesh);

	ASSERT_EQ(firstBakedPosData, secondBakedPosData);
}

TEST_F(MeshLoaderTest, glTFLoadInvalidThenValid) {
	auto meshFile = makeFile("mesh.gltf", "");

	core::MeshDescriptor desc;
	desc.absPath = meshFile;
	desc.bakeAllSubmeshes = true;
	desc.submeshIndex = 0;

	mesh_loader::glTFFileLoader fileloader(desc.absPath);
	auto mesh = fileloader.loadMesh(desc);
	ASSERT_EQ(mesh, nullptr);

	desc.absPath = test_path().append("meshes/CesiumMilkTruck/CesiumMilkTruck.gltf").string();
	mesh = fileloader.loadMesh(desc);
	ASSERT_NE(mesh, nullptr);
}

TEST_F(MeshLoaderTest, glTFWithTangentsAndBitangents) {
	auto mesh = loadMesh("meshes/AnimatedMorphCube/AnimatedMorphCube.gltf", false, 0);

	ASSERT_EQ(mesh->numAttributes(), 8);
	check_attribute(mesh, core::MeshData::ATTRIBUTE_POSITION, core::MeshData::VertexAttribDataType::VAT_Float3, 24);
	check_attribute(mesh, fmt::format("{}_Morph_{}", core::MeshData::ATTRIBUTE_POSITION, 0), core::MeshData::VertexAttribDataType::VAT_Float3, 24);
	check_attribute(mesh, fmt::format("{}_Morph_{}", core::MeshData::ATTRIBUTE_POSITION, 1), core::MeshData::VertexAttribDataType::VAT_Float3, 24);
	check_attribute(mesh, core::MeshData::ATTRIBUTE_NORMAL, core::MeshData::VertexAttribDataType::VAT_Float3, 24);
	check_attribute(mesh, fmt::format("{}_Morph_{}", core::MeshData::ATTRIBUTE_NORMAL, 0), core::MeshData::VertexAttribDataType::VAT_Float3, 24);
	check_attribute(mesh, fmt::format("{}_Morph_{}", core::MeshData::ATTRIBUTE_NORMAL, 1), core::MeshData::VertexAttribDataType::VAT_Float3, 24);
	check_attribute(mesh, core::MeshData::ATTRIBUTE_TANGENT, core::MeshData::VertexAttribDataType::VAT_Float3, 24);
	check_attribute(mesh, core::MeshData::ATTRIBUTE_BITANGENT, core::MeshData::VertexAttribDataType::VAT_Float3, 24);
}

TEST_F(MeshLoaderTest, glTFWithMultipleTexCoords) {
	auto mesh = loadMesh("meshes/MosquitoInAmber/MosquitoInAmber.gltf", false, 1);

	ASSERT_EQ(mesh->numAttributes(), 5);
	check_attribute(mesh, core::MeshData::ATTRIBUTE_POSITION, core::MeshData::VertexAttribDataType::VAT_Float3, 3057);
	check_attribute(mesh, core::MeshData::ATTRIBUTE_NORMAL, core::MeshData::VertexAttribDataType::VAT_Float3, 3057);
	check_attribute(mesh, core::MeshData::ATTRIBUTE_UVMAP, core::MeshData::VertexAttribDataType::VAT_Float2, 3057);
	check_attribute(mesh, core::MeshData::ATTRIBUTE_UVMAP + std::to_string(1), core::MeshData::VertexAttribDataType::VAT_Float2, 3057);
	check_attribute(mesh, core::MeshData::ATTRIBUTE_UVMAP + std::to_string(2), core::MeshData::VertexAttribDataType::VAT_Float2, 3057);
}

TEST_F(MeshLoaderTest, glTFWithMultipleColors) {
	auto mesh = loadMesh("meshes/MultipleVCols/multiple_VCols.gltf");

	ASSERT_EQ(mesh->numAttributes(), 6);
	check_attribute(mesh, core::MeshData::ATTRIBUTE_POSITION, core::MeshData::VertexAttribDataType::VAT_Float3, 625);
	check_attribute(mesh, core::MeshData::ATTRIBUTE_NORMAL, core::MeshData::VertexAttribDataType::VAT_Float3, 625);
	check_attribute(mesh, core::MeshData::ATTRIBUTE_UVMAP, core::MeshData::VertexAttribDataType::VAT_Float2, 625);
	check_attribute(mesh, core::MeshData::ATTRIBUTE_COLOR, core::MeshData::VertexAttribDataType::VAT_Float4, 625);
	check_attribute(mesh, core::MeshData::ATTRIBUTE_COLOR + std::to_string(1), core::MeshData::VertexAttribDataType::VAT_Float4, 625);
	check_attribute(mesh, core::MeshData::ATTRIBUTE_COLOR + std::to_string(2), core::MeshData::VertexAttribDataType::VAT_Float4, 625);
}

TEST_F(MeshLoaderTest, glTFWithSingleJointWeightSet) {
	auto mesh = loadMesh("meshes/SimpleSkin/SimpleSkin.gltf");

	ASSERT_EQ(mesh->numAttributes(), 3);
	check_attribute(mesh, core::MeshData::ATTRIBUTE_POSITION, core::MeshData::VertexAttribDataType::VAT_Float3, 10);
	check_attribute(mesh, core::MeshData::ATTRIBUTE_JOINTS + std::to_string(0), core::MeshData::VertexAttribDataType::VAT_Float4, 10);
	check_attribute(mesh, core::MeshData::ATTRIBUTE_WEIGHTS + std::to_string(0), core::MeshData::VertexAttribDataType::VAT_Float4, 10);
}

TEST_F(MeshLoaderTest, glTFWithMultipleJointWeightSets) {
	auto mesh = loadMesh("meshes/SimpleSkin/SimpleSkin-multi-joint-set.gltf");

	ASSERT_EQ(mesh->numAttributes(), 5);
	check_attribute(mesh, core::MeshData::ATTRIBUTE_POSITION, core::MeshData::VertexAttribDataType::VAT_Float3, 10);
	check_attribute(mesh, core::MeshData::ATTRIBUTE_JOINTS + std::to_string(0), core::MeshData::VertexAttribDataType::VAT_Float4, 10);
	check_attribute(mesh, core::MeshData::ATTRIBUTE_JOINTS + std::to_string(1), core::MeshData::VertexAttribDataType::VAT_Float4, 10);
	check_attribute(mesh, core::MeshData::ATTRIBUTE_WEIGHTS + std::to_string(0), core::MeshData::VertexAttribDataType::VAT_Float4, 10);
	check_attribute(mesh, core::MeshData::ATTRIBUTE_WEIGHTS + std::to_string(1), core::MeshData::VertexAttribDataType::VAT_Float4, 10);
}

TEST_F(MeshLoaderTest, gltFCheckSkinData) {
	auto absPath = test_path().append("meshes/SimpleSkin/SimpleSkin.gltf").string();
	mesh_loader::glTFFileLoader fileloader(absPath);
	std::string error;
	auto skin = fileloader.loadSkin(absPath, 0, error);

	ASSERT_TRUE(skin != nullptr);
	ASSERT_EQ(skin->inverseBindMatrices.size(), 2);
}

TEST_F(MeshLoaderTest, ctmWithGitLfsPlaceholderFile) {
	std::string path = test_path().append("meshes/gitLfsPlaceholderFile.ctm").string();
	raco::createGitLfsPlaceholderFile(path);

	core::MeshDescriptor desc;
	desc.absPath = path;
	auto fileLoader = std::unique_ptr<core::MeshCacheEntry>(new mesh_loader::CTMFileLoader(path));
	auto mesh = fileLoader.get()->loadMesh(desc);

	ASSERT_EQ(fileLoader->getError(), "Git LFS placeholder file detected.");
	ASSERT_EQ(mesh.get(), nullptr);
}

TEST_F(MeshLoaderTest, glTFWithGitLfsPlaceholderFile) {
	std::string path = test_path().append("meshes/gitLfsPlaceholderFile.gltf").string();
	raco::createGitLfsPlaceholderFile(path);

	core::MeshDescriptor desc;
	desc.absPath = path;
	mesh_loader::glTFFileLoader fileLoader(desc.absPath);
	auto mesh = fileLoader.loadMesh(desc);

	ASSERT_EQ(fileLoader.getError(), "Git LFS placeholder file detected.");
	ASSERT_EQ(mesh.get(), nullptr);
}

TEST_F(MeshLoaderTest, glbWithGitLfsPlaceholderFile) {
	std::string path = test_path().append("meshes/gitLfsPlaceholderFile.glb").string();
	raco::createGitLfsPlaceholderFile(path);

	core::MeshDescriptor desc;
	desc.absPath = path;
	mesh_loader::glTFFileLoader fileLoader(desc.absPath);
	auto mesh = fileLoader.loadMesh(desc);

	ASSERT_EQ(fileLoader.getError(), "Git LFS placeholder file detected.");
	ASSERT_EQ(mesh.get(), nullptr);
}