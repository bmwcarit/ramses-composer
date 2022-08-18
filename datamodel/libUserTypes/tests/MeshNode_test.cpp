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
#include "user_types/MeshNode.h"
#include <gtest/gtest.h>

using namespace raco::core;
using namespace raco::user_types;

class MeshNodeTest : public TestEnvironmentCore {
protected:
	void setMaterialOptions(raco::user_types::SMaterial& material) {
		commandInterface.set({material, {"options", "blendOperationColor"}}, 3);
		commandInterface.set({material, {"options", "blendOperationAlpha"}}, 2);
		commandInterface.set({material, {"options", "blendFactorSrcColor"}}, 8);
		commandInterface.set({material, {"options", "blendFactorDestColor"}}, 10);
		commandInterface.set({material, {"options", "blendFactorSrcAlpha"}}, 5);
		commandInterface.set({material, {"options", "blendFactorDestAlpha"}}, 4);
		commandInterface.set({material, {"options", "blendColor", "x"}}, 0.2);
		commandInterface.set({material, {"options", "blendColor", "y"}}, 0.4);
		commandInterface.set({material, {"options", "blendColor", "z"}}, 0.6);
		commandInterface.set({material, {"options", "blendColor", "w"}}, 0.8);
		commandInterface.set({material, {"options", "depthwrite"}}, false);
		commandInterface.set({material, {"options", "depthFunction"}}, 0);
		commandInterface.set({material, {"options", "cullmode"}}, 1);
	}
};

TEST_F(MeshNodeTest, submeshSelection_wrongSubmeshIndexCreatesErrorTooLow) {
	auto meshNode = commandInterface.createObject(MeshNode::typeDescription.typeName, "MeshNode");
	auto mesh = commandInterface.createObject(Mesh::typeDescription.typeName, "Mesh");

	commandInterface.set(ValueHandle{meshNode, {"mesh"}}, mesh);
	commandInterface.set(ValueHandle{mesh, {"bakeMeshes"}}, false);
	commandInterface.set(ValueHandle{mesh, {"uri"}}, test_path().append("meshes/Duck.glb").string());
	commandInterface.set(ValueHandle{mesh, {"meshIndex"}}, -1);
	ASSERT_EQ(commandInterface.errors().getError(ValueHandle{mesh}).level(), raco::core::ErrorLevel::ERROR);
}

TEST_F(MeshNodeTest, submeshSelection_correctSubmeshIndexFixesError) {
	auto meshNode = commandInterface.createObject(MeshNode::typeDescription.typeName, "MeshNode");
	auto mesh = commandInterface.createObject(Mesh::typeDescription.typeName, "Mesh");

	commandInterface.set(ValueHandle{meshNode, {"mesh"}}, mesh);
	commandInterface.set(ValueHandle{mesh, {"bakeMeshes"}}, false);
	commandInterface.set(ValueHandle{mesh, {"uri"}}, test_path().append("meshes/Duck.glb").string());
	commandInterface.set(ValueHandle{mesh, {"meshIndex"}}, 1);
	commandInterface.set(ValueHandle{mesh, {"meshIndex"}}, 0);

	ASSERT_EQ(commandInterface.errors().getError(ValueHandle{mesh}).level(), raco::core::ErrorLevel::INFORMATION);
}

TEST_F(MeshNodeTest, valid_file_with_no_meshes_unbaked_submesh_error) {
	auto meshNode = commandInterface.createObject(MeshNode::typeDescription.typeName, "MeshNode");
	auto mesh = commandInterface.createObject(Mesh::typeDescription.typeName, "Mesh");

	commandInterface.set(ValueHandle{meshNode, &raco::user_types::MeshNode::mesh_}, mesh);
	commandInterface.set(ValueHandle{mesh, &raco::user_types::Mesh::bakeMeshes_}, false);
	commandInterface.set(ValueHandle{mesh, &raco::user_types::Mesh::uri_}, test_path().append("meshes/meshless.gltf").string());

	ASSERT_TRUE(commandInterface.errors().hasError({mesh}));
	ASSERT_EQ(commandInterface.errors().getError(ValueHandle{mesh}).level(), raco::core::ErrorLevel::ERROR);
}

TEST_F(MeshNodeTest, valid_file_with_no_meshes_baked_no_submesh_error) {
	auto meshNode = commandInterface.createObject(MeshNode::typeDescription.typeName, "MeshNode");
	auto mesh = commandInterface.createObject(Mesh::typeDescription.typeName, "Mesh");

	commandInterface.set(ValueHandle{meshNode, &raco::user_types::MeshNode::mesh_}, mesh);
	commandInterface.set(ValueHandle{mesh, &raco::user_types::Mesh::bakeMeshes_}, false);
	commandInterface.set(ValueHandle{mesh, &raco::user_types::Mesh::uri_}, test_path().append("meshes/meshless.gltf").string());
	commandInterface.set(ValueHandle{mesh, &raco::user_types::Mesh::bakeMeshes_}, true);

	ASSERT_TRUE(commandInterface.errors().hasError({mesh}));
	ASSERT_EQ(commandInterface.errors().getError(ValueHandle{mesh}).level(), raco::core::ErrorLevel::ERROR);
}

TEST_F(MeshNodeTest, private_material_options_get_taken_over_from_shared_material) {
	auto material = create_material("Material", "shaders/basic.vert", "shaders/basic.frag");
	auto mesh = create_mesh("Mesh", "meshes/Duck.glb");
	auto meshNode = create_meshnode("MeshNode", mesh, material);

	setMaterialOptions(material);

	commandInterface.set(meshNode->getMaterialPrivateHandle(0), true);

	auto privateMaterialOptionsHandle = meshNode->getMaterialOptionsHandle(0);
	ASSERT_EQ(privateMaterialOptionsHandle.get("blendOperationColor").asInt(), 3);
	ASSERT_EQ(privateMaterialOptionsHandle.get("blendOperationAlpha").asInt(), 2);
	ASSERT_EQ(privateMaterialOptionsHandle.get("blendFactorSrcColor").asInt(), 8);
	ASSERT_EQ(privateMaterialOptionsHandle.get("blendFactorDestColor").asInt(), 10);
	ASSERT_EQ(privateMaterialOptionsHandle.get("blendFactorSrcAlpha").asInt(), 5);
	ASSERT_EQ(privateMaterialOptionsHandle.get("blendFactorDestAlpha").asInt(), 4);
	ASSERT_DOUBLE_EQ(privateMaterialOptionsHandle.get("blendColor").get("x").asDouble(), 0.2);
	ASSERT_DOUBLE_EQ(privateMaterialOptionsHandle.get("blendColor").get("y").asDouble(), 0.4);
	ASSERT_DOUBLE_EQ(privateMaterialOptionsHandle.get("blendColor").get("z").asDouble(), 0.6);
	ASSERT_DOUBLE_EQ(privateMaterialOptionsHandle.get("blendColor").get("w").asDouble(), 0.8);
	ASSERT_EQ(privateMaterialOptionsHandle.get("depthwrite").asBool(), false);
	ASSERT_EQ(privateMaterialOptionsHandle.get("depthFunction").asInt(), 0);
	ASSERT_EQ(privateMaterialOptionsHandle.get("cullmode").asInt(), 1);
}

TEST_F(MeshNodeTest, private_material_new_options_get_taken_from_shared_material_after_reprivatizing) {
	auto material = create_material("Material", "shaders/basic.vert", "shaders/basic.frag");
	auto mesh = create_mesh("Mesh", "meshes/Duck.glb");
	auto meshNode = create_meshnode("MeshNode", mesh, material);

	setMaterialOptions(material);

	commandInterface.set(meshNode->getMaterialPrivateHandle(0), true);

	auto privateMaterialOptionsHandle = meshNode->getMaterialOptionsHandle(0);
	ASSERT_EQ(privateMaterialOptionsHandle.get("blendOperationColor").asInt(), 3);
	ASSERT_EQ(privateMaterialOptionsHandle.get("blendOperationAlpha").asInt(), 2);
	ASSERT_EQ(privateMaterialOptionsHandle.get("blendFactorSrcColor").asInt(), 8);
	ASSERT_EQ(privateMaterialOptionsHandle.get("blendFactorDestColor").asInt(), 10);
	ASSERT_EQ(privateMaterialOptionsHandle.get("blendFactorSrcAlpha").asInt(), 5);
	ASSERT_EQ(privateMaterialOptionsHandle.get("blendFactorDestAlpha").asInt(), 4);
	ASSERT_DOUBLE_EQ(privateMaterialOptionsHandle.get("blendColor").get("x").asDouble(), 0.2);
	ASSERT_DOUBLE_EQ(privateMaterialOptionsHandle.get("blendColor").get("y").asDouble(), 0.4);
	ASSERT_DOUBLE_EQ(privateMaterialOptionsHandle.get("blendColor").get("z").asDouble(), 0.6);
	ASSERT_DOUBLE_EQ(privateMaterialOptionsHandle.get("blendColor").get("w").asDouble(), 0.8);
	ASSERT_EQ(privateMaterialOptionsHandle.get("depthwrite").asBool(), false);
	ASSERT_EQ(privateMaterialOptionsHandle.get("depthFunction").asInt(), 0);
	ASSERT_EQ(privateMaterialOptionsHandle.get("cullmode").asInt(), 1);


	commandInterface.set(meshNode->getMaterialPrivateHandle(0), false);

	commandInterface.set({material, {"options", "blendOperationColor"}}, 4);
	commandInterface.set({material, {"options", "blendOperationAlpha"}}, 1);
	commandInterface.set({material, {"options", "blendFactorSrcColor"}}, 3);
	commandInterface.set({material, {"options", "blendFactorDestColor"}}, 5);
	commandInterface.set({material, {"options", "blendFactorSrcAlpha"}}, 1);
	commandInterface.set({material, {"options", "blendFactorDestAlpha"}}, 0);
	commandInterface.set({material, {"options", "blendColor", "x"}}, 0.1);
	commandInterface.set({material, {"options", "blendColor", "y"}}, 0.3);
	commandInterface.set({material, {"options", "blendColor", "z"}}, 0.5);
	commandInterface.set({material, {"options", "blendColor", "w"}}, 0.6);
	commandInterface.set({material, {"options", "depthwrite"}}, true);
	commandInterface.set({material, {"options", "depthFunction"}}, 1);
	commandInterface.set({material, {"options", "cullmode"}}, 0);

	commandInterface.set(meshNode->getMaterialPrivateHandle(0), true);

	privateMaterialOptionsHandle = meshNode->getMaterialOptionsHandle(0);
	ASSERT_EQ(privateMaterialOptionsHandle.get("blendOperationColor").asInt(), 4);
	ASSERT_EQ(privateMaterialOptionsHandle.get("blendOperationAlpha").asInt(), 1);
	ASSERT_EQ(privateMaterialOptionsHandle.get("blendFactorSrcColor").asInt(), 3);
	ASSERT_EQ(privateMaterialOptionsHandle.get("blendFactorDestColor").asInt(), 5);
	ASSERT_EQ(privateMaterialOptionsHandle.get("blendFactorSrcAlpha").asInt(), 1);
	ASSERT_EQ(privateMaterialOptionsHandle.get("blendFactorDestAlpha").asInt(), 0);
	ASSERT_DOUBLE_EQ(privateMaterialOptionsHandle.get("blendColor").get("x").asDouble(), 0.1);
	ASSERT_DOUBLE_EQ(privateMaterialOptionsHandle.get("blendColor").get("y").asDouble(), 0.3);
	ASSERT_DOUBLE_EQ(privateMaterialOptionsHandle.get("blendColor").get("z").asDouble(), 0.5);
	ASSERT_DOUBLE_EQ(privateMaterialOptionsHandle.get("blendColor").get("w").asDouble(), 0.6);
	ASSERT_EQ(privateMaterialOptionsHandle.get("depthwrite").asBool(), true);
	ASSERT_EQ(privateMaterialOptionsHandle.get("depthFunction").asInt(), 1);
	ASSERT_EQ(privateMaterialOptionsHandle.get("cullmode").asInt(), 0);
}

TEST_F(MeshNodeTest, private_material_options_get_overwritten_from_shared_material_after_reprivatizing) {
	auto material = create_material("Material", "shaders/basic.vert", "shaders/basic.frag");
	auto mesh = create_mesh("Mesh", "meshes/Duck.glb");
	auto meshNode = create_meshnode("MeshNode", mesh, material);

	setMaterialOptions(material);

	commandInterface.set(meshNode->getMaterialPrivateHandle(0), true);

	auto privateMaterialOptionsHandle = meshNode->getMaterialOptionsHandle(0);

	commandInterface.set(privateMaterialOptionsHandle.get("blendOperationColor"), 4);
	commandInterface.set(privateMaterialOptionsHandle.get("blendOperationAlpha"), 1);
	commandInterface.set(privateMaterialOptionsHandle.get("blendFactorSrcColor"), 3);
	commandInterface.set(privateMaterialOptionsHandle.get("blendFactorDestColor"), 5);
	commandInterface.set(privateMaterialOptionsHandle.get("blendFactorSrcAlpha"), 1);
	commandInterface.set(privateMaterialOptionsHandle.get("blendFactorDestAlpha"), 0);
	commandInterface.set(privateMaterialOptionsHandle.get("blendColor").get("x"), 0.1);
	commandInterface.set(privateMaterialOptionsHandle.get("blendColor").get("y"), 0.3);
	commandInterface.set(privateMaterialOptionsHandle.get("blendColor").get("z"), 0.5);
	commandInterface.set(privateMaterialOptionsHandle.get("blendColor").get("w"), 0.6);
	commandInterface.set(privateMaterialOptionsHandle.get("depthwrite"), true);
	commandInterface.set(privateMaterialOptionsHandle.get("depthFunction"), 1);
	commandInterface.set(privateMaterialOptionsHandle.get("cullmode"), 0);

	commandInterface.set(meshNode->getMaterialPrivateHandle(0), false);
	commandInterface.set(meshNode->getMaterialPrivateHandle(0), true);

	privateMaterialOptionsHandle = meshNode->getMaterialOptionsHandle(0);
	ASSERT_EQ(privateMaterialOptionsHandle.get("blendOperationColor").asInt(), 3);
	ASSERT_EQ(privateMaterialOptionsHandle.get("blendOperationAlpha").asInt(), 2);
	ASSERT_EQ(privateMaterialOptionsHandle.get("blendFactorSrcColor").asInt(), 8);
	ASSERT_EQ(privateMaterialOptionsHandle.get("blendFactorDestColor").asInt(), 10);
	ASSERT_EQ(privateMaterialOptionsHandle.get("blendFactorSrcAlpha").asInt(), 5);
	ASSERT_EQ(privateMaterialOptionsHandle.get("blendFactorDestAlpha").asInt(), 4);
	ASSERT_DOUBLE_EQ(privateMaterialOptionsHandle.get("blendColor").get("x").asDouble(), 0.2);
	ASSERT_DOUBLE_EQ(privateMaterialOptionsHandle.get("blendColor").get("y").asDouble(), 0.4);
	ASSERT_DOUBLE_EQ(privateMaterialOptionsHandle.get("blendColor").get("z").asDouble(), 0.6);
	ASSERT_DOUBLE_EQ(privateMaterialOptionsHandle.get("blendColor").get("w").asDouble(), 0.8);
	ASSERT_EQ(privateMaterialOptionsHandle.get("depthwrite").asBool(), false);
	ASSERT_EQ(privateMaterialOptionsHandle.get("depthFunction").asInt(), 0);
	ASSERT_EQ(privateMaterialOptionsHandle.get("cullmode").asInt(), 1);
}

TEST_F(MeshNodeTest, private_material_options_do_not_get_updated_from_shared_material_to_active_private_material) {
	auto material = create_material("Material", "shaders/basic.vert", "shaders/basic.frag");
	auto mesh = create_mesh("Mesh", "meshes/Duck.glb");
	auto meshNode = create_meshnode("MeshNode", mesh, material);

	setMaterialOptions(material);

	commandInterface.set(meshNode->getMaterialPrivateHandle(0), true);

	commandInterface.set({material, {"options", "blendOperationColor"}}, 4);
	commandInterface.set({material, {"options", "blendOperationAlpha"}}, 1);
	commandInterface.set({material, {"options", "blendFactorSrcColor"}}, 3);
	commandInterface.set({material, {"options", "blendFactorDestColor"}}, 5);
	commandInterface.set({material, {"options", "blendFactorSrcAlpha"}}, 1);
	commandInterface.set({material, {"options", "blendFactorDestAlpha"}}, 0);
	commandInterface.set({material, {"options", "blendColor", "x"}}, 0.1);
	commandInterface.set({material, {"options", "blendColor", "y"}}, 0.3);
	commandInterface.set({material, {"options", "blendColor", "z"}}, 0.5);
	commandInterface.set({material, {"options", "blendColor", "w"}}, 0.6);
	commandInterface.set({material, {"options", "depthwrite"}}, true);
	commandInterface.set({material, {"options", "depthFunction"}}, 1);
	commandInterface.set({material, {"options", "cullmode"}}, 0);

	auto privateMaterialOptionsHandle = meshNode->getMaterialOptionsHandle(0);
	ASSERT_EQ(privateMaterialOptionsHandle.get("blendOperationColor").asInt(), 3);
	ASSERT_EQ(privateMaterialOptionsHandle.get("blendOperationAlpha").asInt(), 2);
	ASSERT_EQ(privateMaterialOptionsHandle.get("blendFactorSrcColor").asInt(), 8);
	ASSERT_EQ(privateMaterialOptionsHandle.get("blendFactorDestColor").asInt(), 10);
	ASSERT_EQ(privateMaterialOptionsHandle.get("blendFactorSrcAlpha").asInt(), 5);
	ASSERT_EQ(privateMaterialOptionsHandle.get("blendFactorDestAlpha").asInt(), 4);
	ASSERT_DOUBLE_EQ(privateMaterialOptionsHandle.get("blendColor").get("x").asDouble(), 0.2);
	ASSERT_DOUBLE_EQ(privateMaterialOptionsHandle.get("blendColor").get("y").asDouble(), 0.4);
	ASSERT_DOUBLE_EQ(privateMaterialOptionsHandle.get("blendColor").get("z").asDouble(), 0.6);
	ASSERT_DOUBLE_EQ(privateMaterialOptionsHandle.get("blendColor").get("w").asDouble(), 0.8);
	ASSERT_EQ(privateMaterialOptionsHandle.get("depthwrite").asBool(), false);
	ASSERT_EQ(privateMaterialOptionsHandle.get("depthFunction").asInt(), 0);
	ASSERT_EQ(privateMaterialOptionsHandle.get("cullmode").asInt(), 1);
}