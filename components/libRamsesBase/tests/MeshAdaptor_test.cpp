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

#include "RamsesBaseFixture.h"
#include "ramses_adaptor/MeshAdaptor.h"

class MeshAdaptorTest : public RamsesBaseFixture<> {};


TEST_F(MeshAdaptorTest, context_mesh_name_change) {
	auto node = context.createObject(raco::user_types::Mesh::typeDescription.typeName, "Mesh Name");
	context.set({node, {"uri"}}, test_path().append("meshes/Duck.glb").string());

	dispatch();

	auto meshStuff{select<ramses::ArrayResource>(*sceneContext.scene(), ramses::ERamsesObjectType::ERamsesObjectType_ArrayResource)};
	EXPECT_EQ(meshStuff.size(), 4);
	ASSERT_TRUE(isRamsesNameInArray("Mesh Name_MeshIndexData", meshStuff));
	ASSERT_TRUE(isRamsesNameInArray("Mesh Name_MeshVertexData_a_Position", meshStuff));
	ASSERT_TRUE(isRamsesNameInArray("Mesh Name_MeshVertexData_a_Normal", meshStuff));
	ASSERT_TRUE(isRamsesNameInArray("Mesh Name_MeshVertexData_a_TextureCoordinate", meshStuff));

	context.set({node, {"objectName"}}, std::string("Changed"));
	dispatch();

	meshStuff = select<ramses::ArrayResource>(*sceneContext.scene(), ramses::ERamsesObjectType::ERamsesObjectType_ArrayResource);
	EXPECT_EQ(meshStuff.size(), 4);
	ASSERT_TRUE(isRamsesNameInArray("Changed_MeshIndexData", meshStuff));
	ASSERT_TRUE(isRamsesNameInArray("Changed_MeshVertexData_a_Position", meshStuff));
	ASSERT_TRUE(isRamsesNameInArray("Changed_MeshVertexData_a_Normal", meshStuff));
	ASSERT_TRUE(isRamsesNameInArray("Changed_MeshVertexData_a_TextureCoordinate", meshStuff));
}

TEST_F(MeshAdaptorTest, gltf_without_meshes) {
	auto mesh = context.createObject(raco::user_types::Mesh::typeDescription.typeName, "Mesh Name");
	context.set({mesh, &raco::user_types::Mesh::uri_}, test_path().append("meshes/meshless.gltf").string());

	dispatch();

	auto meshStuff{select<ramses::ArrayResource>(*sceneContext.scene(), ramses::ERamsesObjectType::ERamsesObjectType_ArrayResource)};
	EXPECT_EQ(meshStuff.size(), 0);
}


TEST_F(MeshAdaptorTest, gltf_with_meshes_but_no_mesh_refs_baked) {
	auto mesh = context.createObject(raco::user_types::Mesh::typeDescription.typeName, "Mesh Name");
	context.set({mesh, &raco::user_types::Mesh::bakeMeshes_}, true);
	context.set({mesh, &raco::user_types::Mesh::uri_}, test_path().append("meshes/meshrefless.gltf").string());

	dispatch();

	auto meshStuff{select<ramses::ArrayResource>(*sceneContext.scene(), ramses::ERamsesObjectType::ERamsesObjectType_ArrayResource)};
	EXPECT_EQ(meshStuff.size(), 0);
	ASSERT_EQ(context.errors().getError(mesh).level(), raco::core::ErrorLevel::ERROR);
}

TEST_F(MeshAdaptorTest, gltf_with_meshes_but_no_mesh_refs_unbaked) {
	auto mesh = context.createObject(raco::user_types::Mesh::typeDescription.typeName, "Mesh Name");
	context.set({mesh, &raco::user_types::Mesh::bakeMeshes_}, false);
	context.set({mesh, &raco::user_types::Mesh::uri_}, test_path().append("meshes/meshrefless.gltf").string());

	dispatch();

	auto meshStuff{select<ramses::ArrayResource>(*sceneContext.scene(), ramses::ERamsesObjectType::ERamsesObjectType_ArrayResource)};
	EXPECT_EQ(meshStuff.size(), 4);
	ASSERT_TRUE(isRamsesNameInArray("Mesh Name_MeshIndexData", meshStuff));
	ASSERT_TRUE(isRamsesNameInArray("Mesh Name_MeshVertexData_a_Position", meshStuff));
	ASSERT_TRUE(isRamsesNameInArray("Mesh Name_MeshVertexData_a_Normal", meshStuff));
	ASSERT_TRUE(isRamsesNameInArray("Mesh Name_MeshVertexData_a_TextureCoordinate", meshStuff));
	ASSERT_EQ(context.errors().getError(mesh).level(), raco::core::ErrorLevel::INFORMATION);
}