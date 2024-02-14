/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "RamsesBaseFixture.h"
#include "ramses_adaptor/DefaultRamsesObjects.h"
#include "user_types/Material.h"
#include "user_types/Mesh.h"
#include "user_types/MeshNode.h"
#include "user_types/Node.h"

#include <algorithm>
#include <array>
#include <gtest/gtest.h>

using ramses_adaptor::SceneAdaptor;
using user_types::Material;
using user_types::Mesh;
using user_types::MeshNode;
using user_types::Node;

class SceneContextTest : public RamsesBaseFixture<> {};

TEST_F(SceneContextTest, construction_createsSceneWithGivenId) {
	EXPECT_TRUE(sceneContext.scene() != nullptr);
	EXPECT_EQ(sceneContext.scene()->getSceneId(), ramses::sceneId_t{1u});
}

TEST_F(SceneContextTest, construction_doesNotCreateDefaultMeshInfo) {
	auto meshStuff{select<ramses::ArrayResource>(*sceneContext.scene(), ramses::ERamsesObjectType::ArrayResource)};
	ASSERT_TRUE(meshStuff.empty());
}

TEST_F(SceneContextTest, construction_createSceneWithOneNode) {
	context.createObject(Node::typeDescription.typeName);
	dispatch();

	auto sceneNodes{select(*sceneContext.scene(), ramses::ERamsesObjectType::Node)};

	EXPECT_EQ(sceneNodes.size(), 1);
}

TEST_F(SceneContextTest, construction_createSceneWithOneMeshNode) {
	context.createObject(MeshNode::typeDescription.typeName, "MeshName");
	dispatch();

	auto sceneMeshNodes{select(*sceneContext.scene(), ramses::ERamsesObjectType::MeshNode)};
	auto meshStuff{select<ramses::ArrayResource>(*sceneContext.scene(), ramses::ERamsesObjectType::ArrayResource)};
	auto materialStuff{select<ramses::Effect>(*sceneContext.scene(), ramses::ERamsesObjectType::Effect)};

	EXPECT_EQ(sceneMeshNodes.size(), 1);
	EXPECT_EQ(meshStuff.size(), 3);
	EXPECT_EQ(materialStuff.size(), 1);
	EXPECT_TRUE(isRamsesNameInArray(ramses_adaptor::defaultIndexDataBufferName, meshStuff));
	EXPECT_TRUE(isRamsesNameInArray(ramses_adaptor::defaultVertexDataBufferName, meshStuff));
	EXPECT_TRUE(isRamsesNameInArray(ramses_adaptor::defaultEffectWithNormalsName, materialStuff));
}

TEST_F(SceneContextTest, construction_createThenDeleteOneMeshNode) {
	auto meshNode = context.createObject(MeshNode::typeDescription.typeName, "MeshNode");
	dispatch();

	context.deleteObjects({meshNode});
	dispatch();

	auto meshStuff{select<ramses::ArrayResource>(*sceneContext.scene(), ramses::ERamsesObjectType::ArrayResource)};

	ASSERT_TRUE(meshStuff.empty());
}

TEST_F(SceneContextTest, construction_createMeshNodeWithMesh) {
	auto meshNode = context.createObject(MeshNode::typeDescription.typeName, "Mesh Node");
	auto node = context.createObject(Node::typeDescription.typeName);
	auto mesh = context.createObject(Mesh::typeDescription.typeName, "Mesh");
	auto material = context.createObject(Material::typeDescription.typeName, "Material");

	context.set(core::ValueHandle{mesh, {"uri"}}, (test_path() / "meshes/Duck.glb").string());
	context.set(core::ValueHandle{material, {"uriVertex"}}, (test_path() / "shaders/simple_texture.vert").string());
	context.set(core::ValueHandle{material, {"uriFragment"}}, (test_path() / "shaders/simple_texture.frag").string());

	context.moveScenegraphChildren({meshNode}, node);
	context.set(core::ValueHandle{meshNode, {"mesh"}}, mesh);
	context.set(core::ValueHandle{meshNode, {"materials", "material", "material"}}, material);

	dispatch();

	auto meshStuff{select<ramses::ArrayResource>(*sceneContext.scene(), ramses::ERamsesObjectType::ArrayResource)};
	auto materialStuff{select<ramses::Effect>(*sceneContext.scene(), ramses::ERamsesObjectType::Effect)};
	auto appearances{select<ramses::Appearance>(*sceneContext.scene(), ramses::ERamsesObjectType::Appearance)};

	ASSERT_EQ(meshStuff.size(), 4);
	ASSERT_TRUE(isRamsesNameInArray("Mesh_MeshIndexData", meshStuff));
	ASSERT_TRUE(isRamsesNameInArray("Mesh_MeshVertexData_a_Position", meshStuff));
	ASSERT_TRUE(isRamsesNameInArray("Mesh_MeshVertexData_a_Normal", meshStuff));
	ASSERT_TRUE(isRamsesNameInArray("Mesh_MeshVertexData_a_TextureCoordinate", meshStuff));

	ASSERT_EQ(materialStuff.size(), 1);
	ASSERT_TRUE(isRamsesNameInArray("Material", materialStuff));

	ASSERT_EQ(appearances.size(), 1);
	ASSERT_TRUE(isRamsesNameInArray("Material_Appearance", appearances));
}

TEST_F(SceneContextTest, construction_createOneMeshNodeWithMeshAndOneMeshNodeWithoutMesh) {
	auto meshNode = context.createObject(MeshNode::typeDescription.typeName, "Mesh Node");
	auto meshNode2 = context.createObject(MeshNode::typeDescription.typeName, "Mesh Node2");
	auto mesh = context.createObject(Mesh::typeDescription.typeName, "Mesh");

	context.set(core::ValueHandle{mesh, {"uri"}}, (test_path() / "meshes/Duck.glb").string());

	context.set(core::ValueHandle{meshNode, {"mesh"}}, mesh);

	dispatch();
	auto meshStuff{select<ramses::ArrayResource>(*sceneContext.scene(), ramses::ERamsesObjectType::ArrayResource)};
	auto materialStuff{select<ramses::Effect>(*sceneContext.scene(), ramses::ERamsesObjectType::Effect)};

	ASSERT_EQ(meshStuff.size(), 7);	 // duck: position, normal, tex_coord, inex; default: position, normal, index
	ASSERT_TRUE(isRamsesNameInArray(ramses_adaptor::defaultIndexDataBufferName, meshStuff));
	ASSERT_TRUE(isRamsesNameInArray(ramses_adaptor::defaultVertexDataBufferName, meshStuff));
	ASSERT_TRUE(isRamsesNameInArray(ramses_adaptor::defaultNormalDataBufferName, meshStuff));
	ASSERT_TRUE(isRamsesNameInArray("Mesh_MeshIndexData", meshStuff));
	ASSERT_TRUE(isRamsesNameInArray("Mesh_MeshVertexData_a_Position", meshStuff));
	ASSERT_TRUE(isRamsesNameInArray("Mesh_MeshVertexData_a_Normal", meshStuff));
	ASSERT_TRUE(isRamsesNameInArray("Mesh_MeshVertexData_a_TextureCoordinate", meshStuff));
	ASSERT_EQ(materialStuff.size(), 1);
	ASSERT_TRUE(isRamsesNameInArray(ramses_adaptor::defaultEffectWithNormalsName, materialStuff));
}

TEST_F(SceneContextTest, construction_createMeshNodeWithMeshThenUnassignMesh) {
	auto meshNode = context.createObject(MeshNode::typeDescription.typeName, "MeshNode");
	auto node = context.createObject(Node::typeDescription.typeName);
	auto mesh = context.createObject(Mesh::typeDescription.typeName, "Mesh");
	auto material = context.createObject(Material::typeDescription.typeName, "Material");

	context.set(core::ValueHandle{mesh, {"uri"}}, (test_path() / "meshes/Duck.glb").string());

	context.moveScenegraphChildren({meshNode}, node);
	context.set(core::ValueHandle{meshNode, {"mesh"}}, mesh);
	context.set(core::ValueHandle{meshNode, {"materials", "material", "material"}}, material);
	dispatch();

	context.set(core::ValueHandle{meshNode, {"mesh"}}, core::SEditorObject{nullptr});
	dispatch();

	auto meshStuff{select<ramses::ArrayResource>(*sceneContext.scene(), ramses::ERamsesObjectType::ArrayResource)};
	auto materialStuff{select<ramses::Effect>(*sceneContext.scene(), ramses::ERamsesObjectType::Effect)};

	ASSERT_EQ(meshStuff.size(), 7);
	ASSERT_TRUE(isRamsesNameInArray(ramses_adaptor::defaultIndexDataBufferName, meshStuff));
	ASSERT_TRUE(isRamsesNameInArray(ramses_adaptor::defaultVertexDataBufferName, meshStuff));
	ASSERT_TRUE(isRamsesNameInArray(ramses_adaptor::defaultNormalDataBufferName, meshStuff));
	ASSERT_TRUE(isRamsesNameInArray("Mesh_MeshIndexData", meshStuff));
	ASSERT_TRUE(isRamsesNameInArray("Mesh_MeshVertexData_a_Position", meshStuff));
	ASSERT_TRUE(isRamsesNameInArray("Mesh_MeshVertexData_a_Normal", meshStuff));
	ASSERT_TRUE(isRamsesNameInArray("Mesh_MeshVertexData_a_TextureCoordinate", meshStuff));

	ASSERT_EQ(materialStuff.size(), 2);
	ASSERT_TRUE(isRamsesNameInArray(ramses_adaptor::defaultEffectWithNormalsName, materialStuff));
	ASSERT_TRUE(isRamsesNameInArray("Material", materialStuff));
}

TEST_F(SceneContextTest, construction_createSceneWithSimpleHierarchy) {
	auto parent = context.createObject(Node::typeDescription.typeName);
	auto child = context.createObject(MeshNode::typeDescription.typeName);
	context.moveScenegraphChildren({child}, {parent});
	dispatch();

	auto sceneNodes{select(*sceneContext.scene(), ramses::ERamsesObjectType::Node)};
	auto sceneMeshNodes{select<ramses::MeshNode>(*sceneContext.scene(), ramses::ERamsesObjectType::MeshNode)};

	EXPECT_EQ(sceneNodes.size(), 1);
	EXPECT_EQ(sceneMeshNodes.size(), 1);
	EXPECT_EQ(static_cast<const ramses::MeshNode*>(sceneMeshNodes.at(0))->getParent(), sceneNodes.at(0));
}

TEST_F(SceneContextTest, construction_createSceneWithSimpleHierarchy_reverseNodeCreation) {
	auto child = context.createObject(MeshNode::typeDescription.typeName);
	auto parent = context.createObject(Node::typeDescription.typeName);
	context.moveScenegraphChildren({child}, {parent});
	dispatch();

	auto nodeSceneElements(select(*sceneContext.scene(), ramses::ERamsesObjectType::Node));
	auto meshNodeSceneElements(select<ramses::MeshNode>(*sceneContext.scene(), ramses::ERamsesObjectType::MeshNode));

	EXPECT_EQ(nodeSceneElements.size(), 1);
	EXPECT_EQ(meshNodeSceneElements.size(), 1);
	EXPECT_EQ(static_cast<const ramses::MeshNode*>(meshNodeSceneElements.at(0))->getParent(), nodeSceneElements.at(0));
}

TEST_F(SceneContextTest, construction_createSceneWithDeeperHierarchy) {
	auto root = context.createObject(Node::typeDescription.typeName, "root");
	auto child1 = context.createObject(Node::typeDescription.typeName, "child1");
	auto child2 = context.createObject(MeshNode::typeDescription.typeName, "child2");
	context.moveScenegraphChildren({child1}, {root});
	context.moveScenegraphChildren({child2}, {child1});
	dispatch();

	auto nodeSceneElements(select<ramses::Node>(*sceneContext.scene(), ramses::ERamsesObjectType::Node));
	auto meshNodeSceneElements(select<ramses::MeshNode>(*sceneContext.scene(), ramses::ERamsesObjectType::MeshNode));

	EXPECT_EQ(nodeSceneElements.size(), 2);
	EXPECT_EQ(meshNodeSceneElements.size(), 1);

	// Check and retrive proper ramses elements
	auto ramsesChild1 = select<ramses::Node>(*sceneContext.scene(), "child1");
	EXPECT_TRUE(ramsesChild1);
	auto ramsesRoot = select<ramses::Node>(*sceneContext.scene(), "root");
	EXPECT_TRUE(ramsesRoot);

	// Check hierarchy
	EXPECT_EQ(static_cast<const ramses::Node*>(ramsesRoot)->getParent(), nullptr);
	EXPECT_EQ(static_cast<const ramses::Node*>(ramsesChild1)->getParent(), ramsesRoot);
	EXPECT_EQ(static_cast<const ramses::Node*>(meshNodeSceneElements[0])->getParent(), ramsesChild1);
}

TEST_F(SceneContextTest, construction_createSceneWithDeeperHierarchy_reverseNodeCreation) {
	auto child2 = context.createObject(MeshNode::typeDescription.typeName, "child2");
	auto child1 = context.createObject(Node::typeDescription.typeName, "child1");
	auto root = context.createObject(Node::typeDescription.typeName, "root");
	context.moveScenegraphChildren({child1}, {root});
	context.moveScenegraphChildren({child2}, {child1});
	dispatch();

	auto nodeSceneElements(select<ramses::Node>(*sceneContext.scene(), ramses::ERamsesObjectType::Node));
	auto meshNodeSceneElements(select<ramses::MeshNode>(*sceneContext.scene(), ramses::ERamsesObjectType::MeshNode));

	EXPECT_EQ(nodeSceneElements.size(), 2);
	EXPECT_EQ(meshNodeSceneElements.size(), 1);

	// Check and retrive proper ramses elements
	auto ramsesChild1 = select<ramses::Node>(*sceneContext.scene(), "child1");
	EXPECT_TRUE(ramsesChild1);
	auto ramsesRoot = select<ramses::Node>(*sceneContext.scene(), "root");
	EXPECT_TRUE(ramsesRoot);

	// Expectation depends on order of ramses::SceneObjectIterator, if this fails also check if this order has not changed
	EXPECT_EQ(static_cast<const ramses::Node*>(ramsesRoot)->getParent(), nullptr);
	EXPECT_EQ(static_cast<const ramses::Node*>(ramsesChild1)->getParent(), ramsesRoot);
	EXPECT_EQ(static_cast<const ramses::Node*>(meshNodeSceneElements[0])->getParent(), ramsesChild1);
}

TEST_F(SceneContextTest, dataChange_dynamicInsert_rootNode) {
	auto root = context.createObject(Node::typeDescription.typeName);
	dispatch();

	auto nodeSceneElements(select(*sceneContext.scene(), ramses::ERamsesObjectType::Node));
	EXPECT_EQ(nodeSceneElements.size(), 1);
}

TEST_F(SceneContextTest, dataChange_dynamicInsert_childNode) {
	auto root = context.createObject(Node::typeDescription.typeName);
	dispatch();

	auto child = context.createObject(MeshNode::typeDescription.typeName);
	context.moveScenegraphChildren({child}, {root});
	dispatch();

	auto nodeSceneElements(select(*sceneContext.scene(), ramses::ERamsesObjectType::Node));
	auto meshNodeSceneElements(select(*sceneContext.scene(), ramses::ERamsesObjectType::MeshNode));

	EXPECT_EQ(nodeSceneElements.size(), 1);
	EXPECT_EQ(meshNodeSceneElements.size(), 1);
	EXPECT_EQ(static_cast<const ramses::MeshNode*>(meshNodeSceneElements.at(0))->getParent(), nodeSceneElements.at(0));
}

TEST_F(SceneContextTest, dataChange_dynamicDelete_rootNode) {
	auto root = context.createObject(Node::typeDescription.typeName);
	dispatch();

	context.deleteObjects({root});
	dispatch();

	auto nodeSceneElements(select<ramses::Node>(*sceneContext.scene()));
	EXPECT_EQ(nodeSceneElements.size(), 0);
}

TEST_F(SceneContextTest, dataChange_dynamicReparenting_move) {
	auto parent_1 = context.createObject(Node::typeDescription.typeName, "Parent 1");
	auto parent_2 = context.createObject(Node::typeDescription.typeName, "Parent 2");
	auto child = context.createObject(MeshNode::typeDescription.typeName, "Child");
	context.moveScenegraphChildren({child}, {parent_1});
	dispatch();

	context.moveScenegraphChildren({child}, {parent_2});
	dispatch();

	auto nodeSceneElements(select(*sceneContext.scene(), ramses::ERamsesObjectType::Node));
	auto meshNodeSceneElements(select(*sceneContext.scene(), ramses::ERamsesObjectType::MeshNode));

	EXPECT_EQ(nodeSceneElements.size(), 2);
	EXPECT_EQ(meshNodeSceneElements.size(), 1);

	auto ramsesParent2 = select<ramses::Node>(*sceneContext.scene(), "Parent 2");
	EXPECT_TRUE(ramsesParent2->getName() == "Parent 2");
	EXPECT_EQ(static_cast<const ramses::MeshNode*>(meshNodeSceneElements.at(0))->getParent(), ramsesParent2);
}

TEST_F(SceneContextTest, dataChange_dynamicReparenting_noParent) {
	auto parent_1 = context.createObject(Node::typeDescription.typeName, "Parent 1");
	auto child = context.createObject(MeshNode::typeDescription.typeName, "Child");
	context.moveScenegraphChildren({child}, {parent_1});
	dispatch();

	context.moveScenegraphChildren({child}, {});
	dispatch();

	auto nodeSceneElements(select(*sceneContext.scene(), ramses::ERamsesObjectType::Node));
	auto meshNodeSceneElements(select(*sceneContext.scene(), ramses::ERamsesObjectType::MeshNode));

	EXPECT_EQ(nodeSceneElements.size(), 1);
	EXPECT_EQ(meshNodeSceneElements.size(), 1);

	EXPECT_EQ(static_cast<const ramses::MeshNode*>(meshNodeSceneElements.at(0))->getParent(), nullptr);
}

TEST_F(SceneContextTest, construction_createSceneWithDeeperHierarchy_reverseNodeCreation2) {
	auto rootNode = context.createObject(Node::typeDescription.typeName, "Root", "root1");
	auto childNode = context.createObject(Node::typeDescription.typeName, "Child1", "child1");
	auto childNode2 = context.createObject(Node::typeDescription.typeName, "Child2", "child2");
	auto child21Node = context.createObject(Node::typeDescription.typeName, "Child2_1", "child2_1");
	context.moveScenegraphChildren({child21Node}, {childNode2});
	context.moveScenegraphChildren({childNode2}, {rootNode});
	context.moveScenegraphChildren({childNode}, {rootNode});
	dispatch();
}

// TODO: this seems a little bit of an overkill to get all permutations of an array of size 4, find an easier way to tell INSTANTIATE_TEST_SUITE_P to do it for all permutations of the array
struct CreationOrder {
	CreationOrder(const std::array<std::string, 4>& a) : order{a} {};

	const std::string& typeName(size_t i) const {
		return order[i];
	}

	CreationOrder operator+(const CreationOrder& other) {
		std::next_permutation(order.begin(), order.end());
		return *this;
	}

	bool operator<(const CreationOrder& other) {
		return order < other.order;
	}

	CreationOrder first() {
		auto copy{order};
		std::sort(copy.begin(), copy.end());
		return CreationOrder{copy};
	}

	CreationOrder last() {
		auto copy{order};
		std::sort(copy.begin(), copy.end());
		std::prev_permutation(copy.begin(), copy.end());
		return copy;
	}

	std::array<std::string, 4> order;
};
struct SceneContextParamTestFixture : public RamsesBaseFixture<::testing::TestWithParam<CreationOrder>> {
	struct PrintToStringParamName {
		template <class ParamType>
		std::string operator()(const testing::TestParamInfo<ParamType>& info) const {
			auto location = static_cast<CreationOrder>(info.param);
			return location.typeName(0) + "_" + location.typeName(1) + "_" + location.typeName(2) + "_" + location.typeName(3);
		}
	};
};

TEST_P(SceneContextParamTestFixture, contextCreationOrder_init) {
	std::map<std::string, core::SEditorObject> objects{};

	objects[GetParam().typeName(0)] = context.createObject(GetParam().typeName(0));
	objects[GetParam().typeName(1)] = context.createObject(GetParam().typeName(1));
	objects[GetParam().typeName(2)] = context.createObject(GetParam().typeName(2));
	objects[GetParam().typeName(3)] = context.createObject(GetParam().typeName(3));

	auto meshNode = objects.at(MeshNode::typeDescription.typeName);
	auto node = objects.at(Node::typeDescription.typeName);
	auto mesh = objects.at(Mesh::typeDescription.typeName);
	auto material = objects.at(Material::typeDescription.typeName);

	context.set(core::ValueHandle{mesh, {"uri"}}, (test_path() / "meshes/Duck.glb").string());

	context.moveScenegraphChildren({meshNode}, node);
	context.set(core::ValueHandle{meshNode, {"mesh"}}, mesh);
	context.set(core::ValueHandle{meshNode, {"materials", "material", "material"}}, material);

	// should not explode
	SceneAdaptor sceneContext{&backend.client(), ramses::sceneId_t{2u}, &project, dataChangeDispatcher, &errors};
}

TEST_P(SceneContextParamTestFixture, contextCreationOrder_dispatch) {
	std::map<std::string, core::SEditorObject> objects{};

	objects[GetParam().typeName(0)] = context.createObject(GetParam().typeName(0));
	objects[GetParam().typeName(1)] = context.createObject(GetParam().typeName(1));
	objects[GetParam().typeName(2)] = context.createObject(GetParam().typeName(2));
	objects[GetParam().typeName(3)] = context.createObject(GetParam().typeName(3));

	auto meshNode = objects.at(MeshNode::typeDescription.typeName);
	auto node = objects.at(Node::typeDescription.typeName);
	auto mesh = objects.at(Mesh::typeDescription.typeName);
	auto material = objects.at(Material::typeDescription.typeName);

	context.set(core::ValueHandle{mesh, {"uri"}}, (test_path() / "meshes/Duck.glb").string());

	context.moveScenegraphChildren({meshNode}, node);
	context.set(core::ValueHandle{meshNode, {"mesh"}}, mesh);
	context.set(core::ValueHandle{meshNode, {"materials", "material", "material"}}, material);

	// should not explode
	dispatch();
}

CreationOrder creationOrder{{Node::typeDescription.typeName, MeshNode::typeDescription.typeName, Material::typeDescription.typeName, Mesh::typeDescription.typeName}};
INSTANTIATE_TEST_SUITE_P(
	SceneContext,
	SceneContextParamTestFixture,
	::testing::Range(creationOrder.first(), creationOrder.last(), creationOrder),
	SceneContextParamTestFixture::PrintToStringParamName());
