/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "testing/TestEnvironmentCore.h"
#include "user_types/Mesh.h"
#include <gtest/gtest.h>

using namespace raco::core;
using namespace raco::user_types;


class MeshTest : public TestEnvironmentCore {};

TEST_F(MeshTest, submeshSelection_wrongSubmeshIndexCreatesErrorTooLow) {
	auto mesh = commandInterface.createObject(Mesh::typeDescription.typeName, "Mesh");

	commandInterface.set(ValueHandle{mesh, {"bakeMeshes"}}, false);
	commandInterface.set(ValueHandle{mesh, {"uri"}}, test_path().append("meshes/Duck.glb").string());
	commandInterface.set(ValueHandle{mesh, {"meshIndex"}}, -1);
	ASSERT_EQ(commandInterface.errors().getError(ValueHandle{mesh}).level(), raco::core::ErrorLevel::ERROR);
}

TEST_F(MeshTest, submeshSelection_correctSubmeshIndexFixesError) {
	auto mesh = commandInterface.createObject(Mesh::typeDescription.typeName, "Mesh");

	commandInterface.set(ValueHandle{mesh, {"bakeMeshes"}}, false);
	commandInterface.set(ValueHandle{mesh, {"uri"}}, test_path().append("meshes/Duck.glb").string());
	commandInterface.set(ValueHandle{mesh, {"meshIndex"}}, 1);
	commandInterface.set(ValueHandle{mesh, {"meshIndex"}}, 0);

	ASSERT_EQ(commandInterface.errors().getError(ValueHandle{mesh}).level(), raco::core::ErrorLevel::INFORMATION);
}

TEST_F(MeshTest, meshInfo_showMetaDataWithUnbakedMeshes) {
	auto mesh = commandInterface.createObject(Mesh::typeDescription.typeName, "Mesh");

	commandInterface.set(ValueHandle{mesh, &raco::user_types::Mesh::bakeMeshes_}, false);
	commandInterface.set(ValueHandle{mesh, &raco::user_types::Mesh::uri_}, test_path().append("meshes/CesiumMilkTruck/CesiumMilkTruck.gltf").string());

	ASSERT_EQ(commandInterface.errors().getError(ValueHandle{mesh}).message(),
		R"(Mesh information

Triangles: 768
Vertices: 828
Total Asset File Meshes: 4

Attributes:
in vec3 a_Position;
in vec3 a_Normal;
in vec2 a_TextureCoordinate;)");

	commandInterface.set(ValueHandle{mesh, &raco::user_types::Mesh::meshIndex_}, 1);
	ASSERT_EQ(commandInterface.errors().getError(ValueHandle{mesh}).message(),
		R"(Mesh information

Triangles: 1744
Vertices: 2366
Total Asset File Meshes: 4

Attributes:
in vec3 a_Position;
in vec3 a_Normal;
in vec2 a_TextureCoordinate;

Metadata:
prop1: truck mesh property)");

	commandInterface.set(ValueHandle{mesh, &raco::user_types::Mesh::meshIndex_}, 2);
	ASSERT_EQ(commandInterface.errors().getError(ValueHandle{mesh}).message(),
		R"(Mesh information

Triangles: 56
Vertices: 151
Total Asset File Meshes: 4

Attributes:
in vec3 a_Position;
in vec3 a_Normal;
in vec2 a_TextureCoordinate;

Metadata:
prop1: truck mesh property)");

	commandInterface.set(ValueHandle{mesh, &raco::user_types::Mesh::meshIndex_}, 3);
	ASSERT_EQ(commandInterface.errors().getError(ValueHandle{mesh}).message(),
		R"(Mesh information

Triangles: 288
Vertices: 650
Total Asset File Meshes: 4

Attributes:
in vec3 a_Position;
in vec3 a_Normal;
in vec2 a_TextureCoordinate;

Metadata:
prop1: truck mesh property)");
}

TEST_F(MeshTest, meshInfo_dontShowMetaDataWithBakedMeshes) {
	auto mesh = commandInterface.createObject(Mesh::typeDescription.typeName, "Mesh");

	commandInterface.set(ValueHandle{mesh, &raco::user_types::Mesh::bakeMeshes_}, false);
	commandInterface.set(ValueHandle{mesh, &raco::user_types::Mesh::uri_}, test_path().append("meshes/CesiumMilkTruck/CesiumMilkTruck.gltf").string());
	commandInterface.set(ValueHandle{mesh, &raco::user_types::Mesh::meshIndex_}, 1);
	commandInterface.set(ValueHandle{mesh, &raco::user_types::Mesh::bakeMeshes_}, true);

	ASSERT_EQ(commandInterface.errors().getError(ValueHandle{mesh}).message(),
		R"(Mesh information

Triangles: 3624
Vertices: 4823
Total Asset File Meshes: 4

Attributes:
in vec3 a_Position;
in vec3 a_Normal;
in vec2 a_TextureCoordinate;)");
}

TEST_F(MeshTest, meshInfo_updateMetaDataAfterInvalidUri) {
	auto mesh = commandInterface.createObject(Mesh::typeDescription.typeName, "Mesh");

	commandInterface.set(ValueHandle{mesh, &raco::user_types::Mesh::bakeMeshes_}, false);
	commandInterface.set(ValueHandle{mesh, &raco::user_types::Mesh::uri_}, test_path().append("meshes/CesiumMilkTruck/CesiumMilkTruck.gltf").string());
	commandInterface.set(ValueHandle{mesh, &raco::user_types::Mesh::meshIndex_}, 1);
	commandInterface.set(ValueHandle{mesh, &raco::user_types::Mesh::uri_}, std::string("invalid"));

	ASSERT_TRUE(mesh->as<raco::user_types::Mesh>()->metaData_.empty());

	commandInterface.set(ValueHandle{mesh, &raco::user_types::Mesh::uri_}, test_path().append("meshes/CesiumMilkTruck/CesiumMilkTruck.gltf").string());
	ASSERT_FALSE(mesh->as<raco::user_types::Mesh>()->metaData_.empty());
}

TEST_F(MeshTest, valid_file_with_no_meshes_unbaked_submesh_error) {
	auto mesh = commandInterface.createObject(Mesh::typeDescription.typeName, "Mesh");

	commandInterface.set(ValueHandle{mesh, &raco::user_types::Mesh::bakeMeshes_}, false);
	commandInterface.set(ValueHandle{mesh, &raco::user_types::Mesh::uri_}, test_path().append("meshes/meshless.gltf").string());

	ASSERT_TRUE(commandInterface.errors().hasError({mesh}));
	ASSERT_EQ(commandInterface.errors().getError(ValueHandle{mesh}).level(), raco::core::ErrorLevel::ERROR);
}

TEST_F(MeshTest, valid_file_with_no_meshes_baked_no_submesh_error) {
	auto mesh = commandInterface.createObject(Mesh::typeDescription.typeName, "Mesh");

	commandInterface.set(ValueHandle{mesh, &raco::user_types::Mesh::bakeMeshes_}, false);
	commandInterface.set(ValueHandle{mesh, &raco::user_types::Mesh::uri_}, test_path().append("meshes/meshless.gltf").string());
	commandInterface.set(ValueHandle{mesh, &raco::user_types::Mesh::bakeMeshes_}, true);

	ASSERT_TRUE(commandInterface.errors().hasError({mesh}));
	ASSERT_EQ(commandInterface.errors().getError(ValueHandle{mesh}).level(), raco::core::ErrorLevel::ERROR);
}