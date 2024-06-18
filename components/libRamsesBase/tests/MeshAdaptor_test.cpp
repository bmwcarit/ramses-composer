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

#include "RamsesBaseFixture.h"
#include "ramses_adaptor/MeshAdaptor.h"

class MeshAdaptorTest : public RamsesBaseFixture<> {};


TEST_F(MeshAdaptorTest, context_mesh_name_change) {
	auto node = context.createObject(user_types::Mesh::typeDescription.typeName, "Mesh Name");
	context.set({node, {"uri"}}, test_path().append("meshes/Duck.glb").string());

	dispatch();

	auto meshStuff{select<ramses::ArrayResource>(*sceneContext.scene(), ramses::ERamsesObjectType::ArrayResource)};
	EXPECT_EQ(meshStuff.size(), 4);
	ASSERT_TRUE(isRamsesNameInArray("Mesh Name_MeshIndexData", meshStuff));
	ASSERT_TRUE(isRamsesNameInArray("Mesh Name_MeshVertexData_a_Position", meshStuff));
	ASSERT_TRUE(isRamsesNameInArray("Mesh Name_MeshVertexData_a_Normal", meshStuff));
	ASSERT_TRUE(isRamsesNameInArray("Mesh Name_MeshVertexData_a_TextureCoordinate", meshStuff));

	context.set({node, {"objectName"}}, std::string("Changed"));
	dispatch();

	meshStuff = select<ramses::ArrayResource>(*sceneContext.scene(), ramses::ERamsesObjectType::ArrayResource);
	EXPECT_EQ(meshStuff.size(), 4);
	ASSERT_TRUE(isRamsesNameInArray("Changed_MeshIndexData", meshStuff));
	ASSERT_TRUE(isRamsesNameInArray("Changed_MeshVertexData_a_Position", meshStuff));
	ASSERT_TRUE(isRamsesNameInArray("Changed_MeshVertexData_a_Normal", meshStuff));
	ASSERT_TRUE(isRamsesNameInArray("Changed_MeshVertexData_a_TextureCoordinate", meshStuff));
}

TEST_F(MeshAdaptorTest, gltf_without_meshes) {
	auto mesh = context.createObject(user_types::Mesh::typeDescription.typeName, "Mesh Name");
	context.set({mesh, &user_types::Mesh::uri_}, test_path().append("meshes/meshless.gltf").string());

	dispatch();

	auto meshStuff{select<ramses::ArrayResource>(*sceneContext.scene(), ramses::ERamsesObjectType::ArrayResource)};
	EXPECT_EQ(meshStuff.size(), 0);
}


TEST_F(MeshAdaptorTest, gltf_with_meshes_but_no_mesh_refs_baked) {
	auto mesh = context.createObject(user_types::Mesh::typeDescription.typeName, "Mesh Name");
	context.set({mesh, &user_types::Mesh::bakeMeshes_}, true);
	context.set({mesh, &user_types::Mesh::uri_}, test_path().append("meshes/meshrefless.gltf").string());

	dispatch();

	auto meshStuff{select<ramses::ArrayResource>(*sceneContext.scene(), ramses::ERamsesObjectType::ArrayResource)};
	EXPECT_EQ(meshStuff.size(), 0);
	ASSERT_EQ(context.errors().getError(mesh).level(), core::ErrorLevel::ERROR);
}

TEST_F(MeshAdaptorTest, gltf_with_meshes_but_no_mesh_refs_unbaked) {
	auto mesh = context.createObject(user_types::Mesh::typeDescription.typeName, "Mesh Name");
	context.set({mesh, &user_types::Mesh::bakeMeshes_}, false);
	context.set({mesh, &user_types::Mesh::uri_}, test_path().append("meshes/meshrefless.gltf").string());

	dispatch();

	auto meshStuff{select<ramses::ArrayResource>(*sceneContext.scene(), ramses::ERamsesObjectType::ArrayResource)};
	EXPECT_EQ(meshStuff.size(), 4);
	ASSERT_TRUE(isRamsesNameInArray("Mesh Name_MeshIndexData", meshStuff));
	ASSERT_TRUE(isRamsesNameInArray("Mesh Name_MeshVertexData_a_Position", meshStuff));
	ASSERT_TRUE(isRamsesNameInArray("Mesh Name_MeshVertexData_a_Normal", meshStuff));
	ASSERT_TRUE(isRamsesNameInArray("Mesh Name_MeshVertexData_a_TextureCoordinate", meshStuff));
	ASSERT_EQ(context.errors().getError(mesh).level(), core::ErrorLevel::INFORMATION);
}

TEST_F(MeshAdaptorTest, gltf_color_attribute_vec3_no_underscore) {
	auto mesh = create<user_types::Mesh>("mesh");
	context.set({mesh, &user_types::Mesh::bakeMeshes_}, false);
	context.set({mesh, &user_types::Mesh::uri_}, test_path().append("meshes/cube_color_attr_no_underscore_vec3.gltf").string());

	dispatch();

	auto data = mesh->meshData();

	auto color_idx = data->attribIndex("a_Color");
	ASSERT_TRUE(color_idx >= 0);
	EXPECT_EQ(data->attribDataType(color_idx), core::MeshData::VertexAttribDataType::VAT_Float4);

	auto meshStuff{select<ramses::ArrayResource>(*sceneContext.scene(), ramses::ERamsesObjectType::ArrayResource)};
	EXPECT_EQ(meshStuff.size(), 7);
	ASSERT_TRUE(isRamsesNameInArray("mesh_MeshIndexData", meshStuff));
	ASSERT_TRUE(isRamsesNameInArray("mesh_MeshVertexData_a_Position", meshStuff));
	ASSERT_TRUE(isRamsesNameInArray("mesh_MeshVertexData_a_Normal", meshStuff));
	ASSERT_TRUE(isRamsesNameInArray("mesh_MeshVertexData_a_TextureCoordinate", meshStuff));
	ASSERT_TRUE(isRamsesNameInArray("mesh_MeshVertexData_a_Tangent", meshStuff));
	ASSERT_TRUE(isRamsesNameInArray("mesh_MeshVertexData_a_Bitangent", meshStuff));
	ASSERT_TRUE(isRamsesNameInArray("mesh_MeshVertexData_a_Color", meshStuff));
	ASSERT_EQ(context.errors().getError({mesh}).level(), core::ErrorLevel::INFORMATION);
}

TEST_F(MeshAdaptorTest, gltf_color_attribute_vec4_with_underscore) {
	auto mesh = create<user_types::Mesh>("mesh");
	context.set({mesh, &user_types::Mesh::bakeMeshes_}, false);
	context.set({mesh, &user_types::Mesh::uri_}, test_path().append("meshes/cube_color_attr_with_underscore_vec4.gltf").string());

	dispatch();

	auto data = mesh->meshData();

	auto color_idx = data->attribIndex("a_Color");
	ASSERT_TRUE(color_idx >= 0);
	EXPECT_EQ(data->attribDataType(color_idx), core::MeshData::VertexAttribDataType::VAT_Float4);

	auto meshStuff{select<ramses::ArrayResource>(*sceneContext.scene(), ramses::ERamsesObjectType::ArrayResource)};
	EXPECT_EQ(meshStuff.size(), 7);
	ASSERT_TRUE(isRamsesNameInArray("mesh_MeshIndexData", meshStuff));
	ASSERT_TRUE(isRamsesNameInArray("mesh_MeshVertexData_a_Position", meshStuff));
	ASSERT_TRUE(isRamsesNameInArray("mesh_MeshVertexData_a_Normal", meshStuff));
	ASSERT_TRUE(isRamsesNameInArray("mesh_MeshVertexData_a_TextureCoordinate", meshStuff));
	ASSERT_TRUE(isRamsesNameInArray("mesh_MeshVertexData_a_Tangent", meshStuff));
	ASSERT_TRUE(isRamsesNameInArray("mesh_MeshVertexData_a_Bitangent", meshStuff));
	ASSERT_TRUE(isRamsesNameInArray("mesh_MeshVertexData_a_Color", meshStuff));
	ASSERT_EQ(context.errors().getError({mesh}).level(), core::ErrorLevel::INFORMATION);
}
