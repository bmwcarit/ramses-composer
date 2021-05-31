/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include <gtest/gtest.h>

#include "mesh_loader/glTFFileLoader.h"
#include "testing/RacoBaseTest.h"
#include "testing/TestEnvironmentCore.h"

#include <assimp/Importer.hpp>

using namespace raco;

class MeshLoaderTest : public TestEnvironmentCore {};

TEST_F(MeshLoaderTest, glTFLoadBaked) {
	core::MeshDescriptor desc;
	desc.absPath = cwd_path().append("meshes/CesiumMilkTruck/CesiumMilkTruck.gltf").string();
	desc.bakeAllSubmeshes = true;

	mesh_loader::glTFFileLoader fileloader(desc.absPath);
	auto loadedMesh = fileloader.loadMesh(desc);

	auto submeshIndexBufferRanges = loadedMesh->submeshIndexBufferRanges();
	auto totalSubmeshIndexBufferRangeCount = std::accumulate(submeshIndexBufferRanges.begin(), submeshIndexBufferRanges.end(), 0, [](int sum, auto &info) { return sum + info.count; });
	ASSERT_EQ(loadedMesh->getIndices().size(), totalSubmeshIndexBufferRangeCount);
	ASSERT_EQ(loadedMesh->numSubmeshes(), 1);  // TODO should be 4 with full submesh support
	ASSERT_EQ(fileloader.getTotalMeshCount(desc.bakeAllSubmeshes), 4);
}

TEST_F(MeshLoaderTest, glTFLoadUnbaked) {
	core::MeshDescriptor desc;
	desc.absPath = cwd_path().append("meshes/CesiumMilkTruck/CesiumMilkTruck.gltf").string();
	desc.bakeAllSubmeshes = false;
	desc.submeshIndex = 1;

	mesh_loader::glTFFileLoader fileloader(desc.absPath);
	auto loadedMesh = fileloader.loadMesh(desc);

	ASSERT_EQ(loadedMesh->getIndices().size(), loadedMesh->submeshIndexBufferRanges().front().count);
	ASSERT_EQ(loadedMesh->numSubmeshes(), 1);
	ASSERT_EQ(fileloader.getTotalMeshCount(desc.bakeAllSubmeshes), 4);
}

TEST_F(MeshLoaderTest, glTFLoadUnbakedThenBaked) {
	core::MeshDescriptor desc;
	desc.absPath = cwd_path().append("meshes/CesiumMilkTruck/CesiumMilkTruck.gltf").string();
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
	ASSERT_EQ(fileloader.getTotalMeshCount(desc.bakeAllSubmeshes), 4);
}