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

#include "AdaptorTestUtils.h"
#include "MaterialAdaptorTestBase.h"

#include "ramses_adaptor/MeshNodeAdaptor.h"
#include "ramses_adaptor/SceneAdaptor.h"
#include "ramses_adaptor/utilities.h"
#include "user_types/RenderBufferMS.h"

using namespace raco;
using raco::ramses_adaptor::MeshNodeAdaptor;
using raco::ramses_adaptor::SceneAdaptor;
using raco::user_types::Material;
using raco::user_types::Mesh;
using raco::user_types::MeshNode;
using raco::user_types::RenderBufferMS;
using raco::user_types::SMeshNode;
using raco::user_types::ValueHandle;

class MeshNodeAdaptorFixture : public MaterialAdaptorTestBase {
protected:
	template <int MESH_NODE_AMOUNT>
	void runMeshNodeConstructionRoutine(bool private_material) {
		auto mesh = create_mesh("Mesh", "meshes/Duck.glb");
		auto material = create_material("Material", "shaders/basic.vert", "shaders/basic.frag");

		std::array<SMeshNode, MESH_NODE_AMOUNT> meshNodes;
		for (int i = 0; i < MESH_NODE_AMOUNT; ++i) {
			meshNodes[i] = create_meshnode(std::to_string(i), mesh, material);
			context.set(meshNodes[i]->getMaterialPrivateHandle(0), private_material);
		}
		dispatch();

		auto selectedMeshNodes{select<ramses::MeshNode>(*sceneContext.scene(), ramses::ERamsesObjectType::ERamsesObjectType_MeshNode)};
		auto geometryBindings{select<ramses::GeometryBinding>(*sceneContext.scene(), ramses::ERamsesObjectType::ERamsesObjectType_GeometryBinding)};
		auto effects{select<ramses::Effect>(*sceneContext.scene(), ramses::ERamsesObjectType::ERamsesObjectType_Effect)};
		auto appearances{select<ramses::Appearance>(*sceneContext.scene(), ramses::ERamsesObjectType::ERamsesObjectType_Appearance)};
		EXPECT_EQ(selectedMeshNodes.size(), MESH_NODE_AMOUNT);
		EXPECT_EQ(geometryBindings.size(), MESH_NODE_AMOUNT);
		EXPECT_EQ(effects.size(), 1);
		EXPECT_EQ(appearances.size(), private_material ? MESH_NODE_AMOUNT + 1 : 1);

		for (int i = 0; i < MESH_NODE_AMOUNT; ++i) {
			auto ramsesMeshNode = select<ramses::MeshNode>(*sceneContext.scene(), std::to_string(i).c_str());
			EXPECT_STREQ(std::to_string(i).c_str(), ramsesMeshNode->getName());

			EXPECT_TRUE(ramsesMeshNode->getAppearance() != nullptr);
			if (private_material) {
				EXPECT_STREQ((std::to_string(i) + "_Appearance").c_str(), ramsesMeshNode->getAppearance()->getName());

				auto appearanceBinding = select<rlogic::RamsesAppearanceBinding>(sceneContext.logicEngine(), std::to_string(i).append("_AppearanceBinding").c_str());
				EXPECT_EQ(appearanceBinding->getUserId(), meshNodes[i]->objectIDAsRamsesLogicID());
			} else {
				EXPECT_STREQ("Material_Appearance", ramsesMeshNode->getAppearance()->getName());

				auto appearanceBinding = select<rlogic::RamsesAppearanceBinding>(sceneContext.logicEngine(), "Material_AppearanceBinding");
				EXPECT_EQ(appearanceBinding->getUserId(), material->objectIDAsRamsesLogicID());
			}
			EXPECT_STREQ("Material", ramsesMeshNode->getAppearance()->getEffect().getName());
			EXPECT_TRUE(ramsesMeshNode->getGeometryBinding() != nullptr);
		}
	}
};

TEST_F(MeshNodeAdaptorFixture, inContext_userType_MeshNode_constructs_ramsesMeshNode) {
	auto meshNode = context.createObject(MeshNode::typeDescription.typeName, "MeshNode Name");

	dispatch();
	auto meshNodes{select<ramses::MeshNode>(*sceneContext.scene(), ramses::ERamsesObjectType::ERamsesObjectType_MeshNode)};

	EXPECT_EQ(meshNodes.size(), 1);
	EXPECT_TRUE(meshNodes[0]->getAppearance() != nullptr);
	EXPECT_TRUE(meshNodes[0]->getGeometryBinding() != nullptr);
	EXPECT_STREQ("MeshNode Name", meshNodes[0]->getName());
}

TEST_F(MeshNodeAdaptorFixture, inContext_userType_MeshNode_name_change) {
	auto meshNode = context.createObject(MeshNode::typeDescription.typeName, "MeshNode Name");

	dispatch();

	auto meshNodes{select<ramses::MeshNode>(*sceneContext.scene(), ramses::ERamsesObjectType::ERamsesObjectType_MeshNode)};

	EXPECT_EQ(meshNodes.size(), 1);
	EXPECT_TRUE(meshNodes[0]->getAppearance() != nullptr);
	EXPECT_TRUE(meshNodes[0]->getGeometryBinding() != nullptr);
	EXPECT_STREQ("MeshNode Name", meshNodes[0]->getName());
	EXPECT_STREQ(raco::ramses_adaptor::defaultAppearanceName, meshNodes[0]->getAppearance()->getName());

	context.set({meshNode, {"objectName"}}, std::string("Changed"));
	dispatch();

	EXPECT_STREQ("Changed", meshNodes[0]->getName());
	EXPECT_STREQ(raco::ramses_adaptor::defaultAppearanceName, meshNodes[0]->getAppearance()->getName());
	EXPECT_STREQ("Changed_GeometryBinding", meshNodes[0]->getGeometryBinding()->getName());
}

TEST_F(MeshNodeAdaptorFixture, inContext_userType_MeshNode_withEmptyMesh_constructs_ramsesMeshNode) {
	auto mesh = context.createObject(Mesh::typeDescription.typeName, "Mesh");
	auto meshNode = context.createObject(MeshNode::typeDescription.typeName, "MeshNode Name");
	context.set(ValueHandle{meshNode}.get("mesh"), mesh);

	dispatch();
	auto meshNodes{select<ramses::MeshNode>(*sceneContext.scene(), ramses::ERamsesObjectType::ERamsesObjectType_MeshNode)};
	auto geometryBindings{select<ramses::GeometryBinding>(*sceneContext.scene(), ramses::ERamsesObjectType::ERamsesObjectType_GeometryBinding)};

	EXPECT_EQ(meshNodes.size(), 1);
	EXPECT_TRUE(meshNodes[0]->getAppearance() != nullptr);
	EXPECT_TRUE(meshNodes[0]->getGeometryBinding() != nullptr);
	EXPECT_STREQ("MeshNode Name", meshNodes[0]->getName());
}

TEST_F(MeshNodeAdaptorFixture, inContext_userType_MeshNode_withEmptyMesh_createdAfterMeshNode_constructs_ramsesMeshNode) {
	auto meshNode = context.createObject(MeshNode::typeDescription.typeName, "MeshNode Name");
	auto mesh = context.createObject(Mesh::typeDescription.typeName, "Mesh");
	context.set(ValueHandle{meshNode}.get("mesh"), mesh);

	dispatch();
	auto meshNodes{select<ramses::MeshNode>(*sceneContext.scene(), ramses::ERamsesObjectType::ERamsesObjectType_MeshNode)};
	auto geometryBindings{select<ramses::GeometryBinding>(*sceneContext.scene(), ramses::ERamsesObjectType::ERamsesObjectType_GeometryBinding)};

	EXPECT_EQ(meshNodes.size(), 1);
	EXPECT_TRUE(meshNodes[0]->getAppearance() != nullptr);
	EXPECT_TRUE(meshNodes[0]->getGeometryBinding() != nullptr);
	EXPECT_STREQ("MeshNode Name", meshNodes[0]->getName());
}

TEST_F(MeshNodeAdaptorFixture, inContext_userType_MeshNode_withEmptyMaterial_constructs_ramsesMeshNode) {
	auto mesh = create_mesh("Mesh", "meshes/Duck.glb");
	auto material = create<Material>("Material");
	auto meshnode = create_meshnode("MeshNode", mesh, material);
	dispatch();

	auto meshNodes{select<ramses::MeshNode>(*sceneContext.scene(), ramses::ERamsesObjectType::ERamsesObjectType_MeshNode)};
	auto geometryBindings{select<ramses::GeometryBinding>(*sceneContext.scene(), ramses::ERamsesObjectType::ERamsesObjectType_GeometryBinding)};
	EXPECT_EQ(meshNodes.size(), 1);
	EXPECT_EQ(geometryBindings.size(), 1);

	auto ramsesMeshNode = select<ramses::MeshNode>(*sceneContext.scene(), "MeshNode");
	EXPECT_TRUE(ramsesMeshNode->getAppearance() != nullptr);
	EXPECT_STREQ(raco::ramses_adaptor::defaultAppearanceWithNormalsName, ramsesMeshNode->getAppearance()->getName());
	EXPECT_STREQ(raco::ramses_adaptor::defaultEffectWithNormalsName, ramsesMeshNode->getAppearance()->getEffect().getName());
	EXPECT_TRUE(ramsesMeshNode->getGeometryBinding() != nullptr);
	EXPECT_STREQ("MeshNode", ramsesMeshNode->getName());
}

TEST_F(MeshNodeAdaptorFixture, inContext_userType_MeshNode_withMaterial_constructs_ramsesMeshNode) {
	runMeshNodeConstructionRoutine<1>(true);
}

TEST_F(MeshNodeAdaptorFixture, inContext_userType_MeshNode_withMaterial_constructs_ramsesMeshNode_shared) {
	runMeshNodeConstructionRoutine<1>(false);
}

TEST_F(MeshNodeAdaptorFixture, inContext_userType_ten_MeshNodes_withSameMaterial_andSameMesh_construction) {
	runMeshNodeConstructionRoutine<10>(true);
}

TEST_F(MeshNodeAdaptorFixture, inContext_userType_ten_MeshNodes_withSameMaterial_andSameMesh_construction_shared) {
	runMeshNodeConstructionRoutine<10>(false);
}

TEST_F(MeshNodeAdaptorFixture, inContext_userType_ten_MeshNodes_withSameMaterial_andSameMesh_propertyUnsetting) {
	constexpr auto MESH_NODE_AMOUNT = 10;
	std::array<raco::core::SEditorObject, MESH_NODE_AMOUNT> meshNodes;
	auto material = create<Material>("Material");
	auto mesh = create_mesh("Mesh", "meshes/Duck.glb");

	for (int i = 0; i < MESH_NODE_AMOUNT; ++i) {
		meshNodes[i] = create_meshnode(std::to_string(i), mesh, material);
	}

	context.set({material, {"uriVertex"}}, test_path().append("shaders/basic.vert").string());
	context.set({material, {"uriFragment"}}, test_path().append("shaders/basic.frag").string());
	dispatch();

	context.deleteObjects({mesh, material});
	dispatch();

	auto selectedMeshNodes{select<ramses::MeshNode>(*sceneContext.scene(), ramses::ERamsesObjectType::ERamsesObjectType_MeshNode)};
	auto geometryBindings{select<ramses::GeometryBinding>(*sceneContext.scene(), ramses::ERamsesObjectType::ERamsesObjectType_GeometryBinding)};
	EXPECT_EQ(selectedMeshNodes.size(), MESH_NODE_AMOUNT);
	EXPECT_EQ(geometryBindings.size(), MESH_NODE_AMOUNT);

	for (int i = 0; i < MESH_NODE_AMOUNT; ++i) {
		auto ramsesMeshNode = select<ramses::MeshNode>(*sceneContext.scene(), std::to_string(i).c_str());
		EXPECT_STREQ(raco::ramses_adaptor::defaultEffectName, ramsesMeshNode->getAppearance()->getEffect().getName());
		// TODO: Compare Ramses mesh with default sceneAdaptor mesh here
	}
}

TEST_F(MeshNodeAdaptorFixture, inContext_userType_MeshNode_dynamicMaterial_constructs_ramsesMeshNode) {
	auto mesh = create_mesh("Mesh", "meshes/Duck.glb");
	auto material = create<Material>("Material");
	auto meshnode = create_meshnode("MeshNode", mesh, material);

	dispatch();

	// precondition
	{
		auto ramsesMeshNode = select<ramses::MeshNode>(*sceneContext.scene(), "MeshNode");
		EXPECT_STREQ(raco::ramses_adaptor::defaultEffectWithNormalsName, ramsesMeshNode->getAppearance()->getEffect().getName());
	}

	context.set({material, {"uriVertex"}}, test_path().append("shaders/basic.vert").string());
	context.set({material, {"uriFragment"}}, test_path().append("shaders/basic.frag").string());
	dispatch();

	auto meshNodes{select<ramses::MeshNode>(*sceneContext.scene(), ramses::ERamsesObjectType::ERamsesObjectType_MeshNode)};
	auto geometryBindings{select<ramses::GeometryBinding>(*sceneContext.scene(), ramses::ERamsesObjectType::ERamsesObjectType_GeometryBinding)};
	EXPECT_EQ(meshNodes.size(), 1);
	EXPECT_EQ(geometryBindings.size(), 1);

	auto ramsesMeshNode = select<ramses::MeshNode>(*sceneContext.scene(), "MeshNode");
	EXPECT_TRUE(ramsesMeshNode->getAppearance() != nullptr);
	EXPECT_STREQ("Material", ramsesMeshNode->getAppearance()->getEffect().getName());
	EXPECT_TRUE(ramsesMeshNode->getGeometryBinding() != nullptr);
	EXPECT_STREQ("MeshNode", ramsesMeshNode->getName());
}

TEST_F(MeshNodeAdaptorFixture, meshnode_switch_mat_shared_to_private) {
	auto mesh = create_mesh("Mesh", "meshes/Duck.glb");
	auto material = create_material("Material", "shaders/basic.vert", "shaders/basic.frag");
	auto meshnode = create_meshnode("MeshNode", mesh, material);

	dispatch();

	auto ramsesMeshNode = select<ramses::MeshNode>(*sceneContext.scene(), "MeshNode");
	EXPECT_TRUE(ramsesMeshNode->getAppearance() != nullptr);
	EXPECT_STREQ("Material_Appearance", ramsesMeshNode->getAppearance()->getName());
	EXPECT_STREQ("Material", ramsesMeshNode->getAppearance()->getEffect().getName());

	context.set(meshnode->getMaterialPrivateHandle(0), true);
	dispatch();

	EXPECT_TRUE(ramsesMeshNode->getAppearance() != nullptr);
	EXPECT_STREQ("MeshNode_Appearance", ramsesMeshNode->getAppearance()->getName());
	EXPECT_STREQ("Material", ramsesMeshNode->getAppearance()->getEffect().getName());
}

TEST_F(MeshNodeAdaptorFixture, meshnode_shared_material_is_unique) {
	auto mesh = create_mesh("Mesh", "meshes/Duck.glb");
	auto material = create_material("Material", "shaders/basic.vert", "shaders/basic.frag");
	auto meshnode_a = create_meshnode("Duck A", mesh, material);
	auto meshnode_b = create_meshnode("Duck B", mesh, material);

	dispatch();

	auto ramsesMeshNode_a = select<ramses::MeshNode>(*sceneContext.scene(), "Duck A");
	EXPECT_TRUE(ramsesMeshNode_a->getAppearance() != nullptr);

	auto ramsesMeshNode_b = select<ramses::MeshNode>(*sceneContext.scene(), "Duck B");
	EXPECT_TRUE(ramsesMeshNode_b->getAppearance() != nullptr);

	EXPECT_EQ(ramsesMeshNode_a->getAppearance(), ramsesMeshNode_b->getAppearance());
}

TEST_F(MeshNodeAdaptorFixture, meshnode_set_depthwrite_mat_private) {
	auto mesh = create_mesh("Mesh", "meshes/Duck.glb");
	auto material = create_material("Material", "shaders/basic.vert", "shaders/basic.frag");
	auto meshNode = create_meshnode("MeshNode", mesh, material);

	context.set(meshNode->getMaterialPrivateHandle(0), true);
	context.set({material, {"options", "depthwrite"}}, false);
	dispatch();

	auto ramsesMeshNode = select<ramses::MeshNode>(*sceneContext.scene(), "MeshNode");
	EXPECT_TRUE(ramsesMeshNode->getAppearance() != nullptr);
	EXPECT_EQ(ramses::EDepthWrite_Enabled, raco::ramses_adaptor::getDepthWriteMode(ramsesMeshNode->getAppearance()));

	context.set(meshNode->getMaterialOptionsHandle(0).get("depthwrite"), false);
	dispatch();

	ramsesMeshNode = select<ramses::MeshNode>(*sceneContext.scene(), "MeshNode");
	EXPECT_TRUE(ramsesMeshNode->getAppearance() != nullptr);
	EXPECT_EQ(ramses::EDepthWrite_Disabled, raco::ramses_adaptor::getDepthWriteMode(ramsesMeshNode->getAppearance()));
}

TEST_F(MeshNodeAdaptorFixture, meshnode_set_depthwrite_mat_shared) {
	auto mesh = create_mesh("Mesh", "meshes/Duck.glb");
	auto material = create_material("Material", "shaders/basic.vert", "shaders/basic.frag");
	auto meshNode = create_meshnode("MeshNode", mesh, material);

	context.set(meshNode->getMaterialOptionsHandle(0).get("depthwrite"), false);
	dispatch();

	auto ramsesMeshNode = select<ramses::MeshNode>(*sceneContext.scene(), "MeshNode");
	EXPECT_TRUE(ramsesMeshNode->getAppearance() != nullptr);
	EXPECT_EQ(ramses::EDepthWrite_Enabled, raco::ramses_adaptor::getDepthWriteMode(ramsesMeshNode->getAppearance()));

	context.set({material, {"options", "depthwrite"}}, false);
	dispatch();

	ramsesMeshNode = select<ramses::MeshNode>(*sceneContext.scene(), "MeshNode");
	EXPECT_TRUE(ramsesMeshNode->getAppearance() != nullptr);
	EXPECT_EQ(ramses::EDepthWrite_Disabled, raco::ramses_adaptor::getDepthWriteMode(ramsesMeshNode->getAppearance()));
}

TEST_F(MeshNodeAdaptorFixture, inContext_userType_MeshNode_materialReset_and_depthWriteDisable) {
	auto mesh = create_mesh("Mesh", "meshes/Duck.glb");
	auto material = create_material("Material", "shaders/basic.vert", "shaders/basic.frag");
	auto meshNode = create_meshnode("MeshNode", mesh, material);

	context.set({material, {"options", "depthwrite"}}, false);
	dispatch();

	auto ramsesMeshNode = select<ramses::MeshNode>(*sceneContext.scene(), "MeshNode");
	EXPECT_TRUE(ramsesMeshNode->getAppearance() != nullptr);
	EXPECT_EQ(ramses::EDepthWrite_Disabled, raco::ramses_adaptor::getDepthWriteMode(ramsesMeshNode->getAppearance()));

	context.set(ValueHandle{meshNode}.get("materials")[0].get("material"), core::SEditorObject{});
	dispatch();

	ramsesMeshNode = select<ramses::MeshNode>(*sceneContext.scene(), "MeshNode");
	EXPECT_TRUE(ramsesMeshNode->getAppearance() != nullptr);
	EXPECT_STREQ(raco::ramses_adaptor::defaultEffectWithNormalsName, ramsesMeshNode->getAppearance()->getEffect().getName());
	EXPECT_EQ(ramses::EDepthWrite_Enabled, raco::ramses_adaptor::getDepthWriteMode(ramsesMeshNode->getAppearance()));
}

TEST_F(MeshNodeAdaptorFixture, inContext_userType_MeshNode_multiSampledMaterial_invalidNoCrash) {
	auto mesh = create_mesh("Mesh", "meshes/Duck.glb");
	auto material = create_material("Material", "shaders/multisampler.vert", "shaders/multisampler.frag");
	auto meshNode = create_meshnode("MeshNode", mesh, material);
	auto renderBufferMS = create<RenderBufferMS>("RBMS", {});

	context.set(ValueHandle{material}.get("uniforms").get("textureSampler"), renderBufferMS);
	dispatch();

	context.set({renderBufferMS, &RenderBufferMS::sampleCount_}, -1);
	dispatch();
	ASSERT_TRUE(context.errors().hasError(ValueHandle{material}.get("uniforms").get("textureSampler")));

	context.set({renderBufferMS, &RenderBufferMS::sampleCount_}, 2);
	dispatch();
	ASSERT_FALSE(context.errors().hasError(ValueHandle{material}.get("uniforms").get("textureSampler")));
}
TEST_F(MeshNodeAdaptorFixture, inContext_userType_MeshNode_dynamicCreation_meshBeforeMeshNode) {
	auto mesh = context.createObject(Mesh::typeDescription.typeName, "Mesh");
	dispatch();
	auto meshNode = context.createObject(MeshNode::typeDescription.typeName, "MeshNode");
	dispatch();
	context.set(ValueHandle{meshNode, {"mesh"}}, mesh);
	dispatch();
	context.set(ValueHandle{mesh, {"uri"}}, test_path().append("meshes/Duck.glb").string());
	dispatch();

	// TODO: Need a way to check actual mesh resources in ramses, we only check for no error
	EXPECT_TRUE(true);
}

TEST_F(MeshNodeAdaptorFixture, inContext_userType_MeshNode_dynamicCreation_meshNodeBeforeMesh) {
	auto meshNode = context.createObject(MeshNode::typeDescription.typeName, "MeshNode");
	dispatch();
	auto mesh = context.createObject(Mesh::typeDescription.typeName, "Mesh");
	dispatch();
	context.set(ValueHandle{meshNode, {"mesh"}}, mesh);
	dispatch();
	context.set(ValueHandle{mesh, {"uri"}}, test_path().append("meshes/Duck.glb").string());
	dispatch();

	// TODO: Need a way to check actual mesh resources in ramses, we only check for no error
	EXPECT_TRUE(true);
}

TEST_F(MeshNodeAdaptorFixture, inContext_user_type_MeshNode_meshDeletion_meshNodeDataIsEmpty) {
	auto meshNode = context.createObject(MeshNode::typeDescription.typeName, "MeshNode");
	dispatch();
	auto mesh = context.createObject(Mesh::typeDescription.typeName, "Mesh");
	dispatch();
	context.set(ValueHandle{meshNode, {"mesh"}}, mesh);
	dispatch();
	context.set(ValueHandle{mesh, {"uri"}}, test_path().append("meshes/Duck.glb").string());
	dispatch();

	context.deleteObjects({mesh});

	// TODO: Need a way to check actual mesh resources in ramses, we only check for no error
	EXPECT_TRUE(true);
}

TEST_F(MeshNodeAdaptorFixture, inContext_user_type_MeshNode_submeshSelection_wrongSubmeshIndexCreatesErrorTooHigh) {
	auto meshNode = context.createObject(MeshNode::typeDescription.typeName, "MeshNode");
	dispatch();
	auto mesh = context.createObject(Mesh::typeDescription.typeName, "Mesh");
	dispatch();
	context.set(ValueHandle{meshNode, {"mesh"}}, mesh);
	dispatch();
	context.set(ValueHandle{mesh, {"bakeMeshes"}}, false);
	dispatch();
	context.set(ValueHandle{mesh, {"uri"}}, test_path().append("meshes/Duck.glb").string());
	dispatch();
	context.set(ValueHandle{mesh, {"meshIndex"}}, 1);
	dispatch();

	ASSERT_EQ(context.errors().getError(ValueHandle{mesh}).level(), core::ErrorLevel::ERROR);
}

TEST_F(MeshNodeAdaptorFixture, set_get_scalar_uniforms) {
	auto mesh = create_mesh("mesh", "meshes/Duck.glb");
	auto material = create_material("mat", "shaders/uniform-scalar.vert", "shaders/uniform-scalar.frag");
	auto meshnode = create_meshnode("meshnode", mesh, material);

	context.set(meshnode->getMaterialPrivateHandle(0), true);

	setStructComponents({meshnode, {"materials", "material", "uniforms"}}, default_struct_prim_values);

	dispatch();

	auto mat_appearance{select<ramses::Appearance>(*sceneContext.scene(), "mat_Appearance")};
	auto appearance{select<ramses::Appearance>(*sceneContext.scene(), "meshnode_Appearance")};
	EXPECT_NE(mat_appearance, appearance);

	checkStructComponents(appearance, {}, default_struct_prim_values);
}

TEST_F(MeshNodeAdaptorFixture, link_get_scalar_uniforms) {
	auto mesh = create_mesh("mesh", "meshes/Duck.glb");
	auto material = create_material("mat", "shaders/uniform-scalar.vert", "shaders/uniform-scalar.frag");
	auto meshnode = create_meshnode("meshnode", mesh, material);
	auto interface = create_lua_interface("interface", "scripts/uniform-structs.lua");

	context.set(meshnode->getMaterialPrivateHandle(0), true);

	linkStructComponents({interface, {"inputs", "s_prims"}}, {meshnode, {"materials", "material", "uniforms"}});
	setStructComponents({interface, {"inputs", "s_prims"}}, default_struct_prim_values);

	dispatch();

	auto mat_appearance{select<ramses::Appearance>(*sceneContext.scene(), "mat_Appearance")};
	auto appearance{select<ramses::Appearance>(*sceneContext.scene(), "meshnode_Appearance")};
	EXPECT_NE(mat_appearance, appearance);

	checkStructComponents({meshnode, {"materials", "material", "uniforms"}}, appearance, {}, default_struct_prim_values);
}

TEST_F(MeshNodeAdaptorFixture, set_get_array_uniforms) {
	auto mesh = create_mesh("mesh", "meshes/Duck.glb");
	auto material = create_material("mat", "shaders/uniform-array.vert", "shaders/uniform-array.frag");
	auto meshnode = create_meshnode("meshnode", mesh, material);

	context.set(meshnode->getMaterialPrivateHandle(0), true);

	commandInterface.set({meshnode, {"materials", "material", "uniforms", "ivec", "2"}}, 2);
	commandInterface.set({meshnode, {"materials", "material", "uniforms", "fvec", "3"}}, 3.0);

	commandInterface.set({meshnode, {"materials", "material", "uniforms", "avec2", "4", "y"}}, 4.0);
	commandInterface.set({meshnode, {"materials", "material", "uniforms", "avec3", "5", "z"}}, 5.0);
	commandInterface.set({meshnode, {"materials", "material", "uniforms", "avec4", "6", "w"}}, 6.0);

	commandInterface.set({meshnode, {"materials", "material", "uniforms", "aivec2", "4", "i2"}}, 7);
	commandInterface.set({meshnode, {"materials", "material", "uniforms", "aivec3", "5", "i3"}}, 8);
	commandInterface.set({meshnode, {"materials", "material", "uniforms", "aivec4", "6", "i4"}}, 9);

	dispatch();

	auto mat_appearance{select<ramses::Appearance>(*sceneContext.scene(), "mat_Appearance")};
	auto appearance{select<ramses::Appearance>(*sceneContext.scene(), "meshnode_Appearance")};
	EXPECT_NE(mat_appearance, appearance);

	checkUniformVector<int32_t, 2>(appearance, "ivec", 1, 2);
	checkUniformVector<float, 5>(appearance, "fvec", 2, 3.0);

	checkUniformVector<std::array<float, 2>, 4>(appearance, "avec2", 3, {0.0, 4.0});
	checkUniformVector<std::array<float, 3>, 5>(appearance, "avec3", 4, {0.0, 0.0, 5.0});
	checkUniformVector<std::array<float, 4>, 6>(appearance, "avec4", 5, {0.0, 0.0, 0.0, 6.0});

	checkUniformVector<std::array<int32_t, 2>, 4>(appearance, "aivec2", 3, {0, 7});
	checkUniformVector<std::array<int32_t, 3>, 5>(appearance, "aivec3", 4, {0, 0, 8});
	checkUniformVector<std::array<int32_t, 4>, 6>(appearance, "aivec4", 5, {0, 0, 0, 9});
}

TEST_F(MeshNodeAdaptorFixture, set_get_struct_prim_uniforms) {
	auto mesh = create_mesh("mesh", "meshes/Duck.glb");
	auto material = create_material("mat", "shaders/uniform-struct.vert", "shaders/uniform-struct.frag");
	auto meshnode = create_meshnode("meshnode", mesh, material);

	context.set(meshnode->getMaterialPrivateHandle(0), true);

	setStructComponents({meshnode, {"materials", "material", "uniforms", "s_prims"}}, default_struct_prim_values);

	dispatch();

	auto mat_appearance{select<ramses::Appearance>(*sceneContext.scene(), "mat_Appearance")};
	auto appearance{select<ramses::Appearance>(*sceneContext.scene(), "meshnode_Appearance")};
	EXPECT_NE(mat_appearance, appearance);

	checkStructComponents(appearance, "s_prims.", default_struct_prim_values);
}

TEST_F(MeshNodeAdaptorFixture, link_get_struct_prim_uniforms_struct) {
	auto mesh = create_mesh("mesh", "meshes/Duck.glb");
	auto material = create_material("mat", "shaders/uniform-struct.vert", "shaders/uniform-struct.frag");
	auto meshnode = create_meshnode("meshnode", mesh, material);
	auto interface = create_lua_interface("interface", "scripts/uniform-structs.lua");

	context.set(meshnode->getMaterialPrivateHandle(0), true);

	link(interface, {"inputs", "s_prims"}, meshnode, {"materials", "material", "uniforms", "s_prims"});
	setStructComponents({interface, {"inputs", "s_prims"}}, default_struct_prim_values);

	dispatch();

	auto mat_appearance{select<ramses::Appearance>(*sceneContext.scene(), "mat_Appearance")};
	auto appearance{select<ramses::Appearance>(*sceneContext.scene(), "meshnode_Appearance")};
	EXPECT_NE(mat_appearance, appearance);

	checkStructComponents({meshnode, {"materials", "material", "uniforms", "s_prims"}}, appearance, "s_prims.", default_struct_prim_values);
}

TEST_F(MeshNodeAdaptorFixture, link_get_struct_prim_uniforms_members) {
	auto mesh = create_mesh("mesh", "meshes/Duck.glb");
	auto material = create_material("mat", "shaders/uniform-struct.vert", "shaders/uniform-struct.frag");
	auto meshnode = create_meshnode("meshnode", mesh, material);
	auto interface = create_lua_interface("interface", "scripts/uniform-structs.lua");

	context.set(meshnode->getMaterialPrivateHandle(0), true);

	linkStructComponents({interface, {"inputs", "s_prims"}}, {meshnode, {"materials", "material", "uniforms", "s_prims"}});
	setStructComponents({interface, {"inputs", "s_prims"}}, default_struct_prim_values);

	dispatch();

	auto mat_appearance{select<ramses::Appearance>(*sceneContext.scene(), "mat_Appearance")};
	auto appearance{select<ramses::Appearance>(*sceneContext.scene(), "meshnode_Appearance")};
	EXPECT_NE(mat_appearance, appearance);

	checkStructComponents({meshnode, {"materials", "material", "uniforms", "s_prims"}}, appearance, "s_prims.", default_struct_prim_values);
}

TEST_F(MeshNodeAdaptorFixture, set_get_array_struct_prim_uniforms) {
	auto mesh = create_mesh("mesh", "meshes/Duck.glb");
	auto material = create_material("mat", "shaders/uniform-struct.vert", "shaders/uniform-struct.frag");
	auto meshnode = create_meshnode("meshnode", mesh, material);

	context.set(meshnode->getMaterialPrivateHandle(0), true);

	setStructComponents({meshnode, {"materials", "material", "uniforms", "a_s_prims", "2"}}, default_struct_prim_values);

	dispatch();

	auto mat_appearance{select<ramses::Appearance>(*sceneContext.scene(), "mat_Appearance")};
	auto appearance{select<ramses::Appearance>(*sceneContext.scene(), "meshnode_Appearance")};
	EXPECT_NE(mat_appearance, appearance);

	checkStructComponents(appearance, "a_s_prims[1].", default_struct_prim_values);
}
