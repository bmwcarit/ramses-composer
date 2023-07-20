/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "gtest/gtest.h"

#include "ObjectTreeViewDefaultModel_test.h"


#include "core/ExternalReferenceAnnotation.h"
#include "core/Queries.h"
#include "object_tree_view_model/ObjectTreeNode.h"
#include "user_types/Animation.h"
#include "user_types/LuaScript.h"
#include "user_types/LuaInterface.h"
#include "user_types/Material.h"
#include "user_types/Mesh.h"
#include "user_types/MeshNode.h"
#include "user_types/Node.h"
#include "user_types/PrefabInstance.h"
#include "user_types/Skin.h"

#include <QApplication>
#include <core/PrefabOperations.h>

using namespace raco::core;
using namespace raco::object_tree::model;
using namespace raco::user_types;

ObjectTreeViewDefaultModelTest::ObjectTreeViewDefaultModelTest()
	: viewModel_{new raco::object_tree::model::ObjectTreeViewDefaultModel(&commandInterface(), application.dataChangeDispatcher(), externalProjectStore(),
		  {raco::user_types::Animation::typeDescription.typeName,
		raco::user_types::Node::typeDescription.typeName,
		raco::user_types::MeshNode::typeDescription.typeName,
		raco::user_types::PrefabInstance::typeDescription.typeName,
		raco::user_types::OrthographicCamera::typeDescription.typeName,
		raco::user_types::PerspectiveCamera::typeDescription.typeName,
		raco::user_types::LuaScript::typeDescription.typeName,
		raco::user_types::LuaInterface::typeDescription.typeName,
		raco::user_types::Skin::typeDescription.typeName})} {
}


void ObjectTreeViewDefaultModelTest::compareValuesInTree(const SEditorObject &obj, const QModelIndex &objIndex, const ObjectTreeViewDefaultModel &viewModel_) {
	auto *objTreeNode = viewModel_.indexToTreeNode(objIndex);
	std::string treeValue;
	std::string objValue;

	for (int i = 0; i < ObjectTreeViewDefaultModel::COLUMNINDEX_COLUMN_COUNT; ++i) {
		switch (i) {
			case (ObjectTreeViewDefaultModel::COLUMNINDEX_NAME): {
				treeValue = objTreeNode->getRepresentedObject()->objectName();
				objValue = obj->objectName();
				break;
			}
			case ObjectTreeViewDefaultModel::COLUMNINDEX_VISIBILITY: {
				auto visibilityTreeValue = objTreeNode->getVisibility();
				auto visibilityObjValue = obj->get("visibility")->asBool() ? VisibilityState::Visible : VisibilityState::Invisible;
				ASSERT_EQ(visibilityTreeValue, visibilityObjValue);	
			} break;
			case (ObjectTreeViewDefaultModel::COLUMNINDEX_TYPE): {
				treeValue = objTreeNode->getRepresentedObject()->getTypeDescription().typeName;
				objValue = obj->getTypeDescription().typeName;
			}
			break;
			case ObjectTreeViewDefaultModel::COLUMNINDEX_PROJECT: {
				treeValue = project().getProjectNameForObject(objTreeNode->getRepresentedObject());
				objValue = project().getProjectNameForObject(obj);
			}
			break;
			case ObjectTreeViewDefaultModel::COLUMNINDEX_ID: {
				treeValue = objTreeNode->getRepresentedObject()->objectID();
				objValue = obj->objectID();
			}
		    break;
			case ObjectTreeViewDefaultModel::COLUMNINDEX_USERTAGS: {
				auto treeValue = objTreeNode->getUserTags();
				auto objValue = obj->userTags_->asVector<std::string>();
				ASSERT_EQ(treeValue, objValue);
			} 
			break;
			default: {
				FAIL() << "Need to check value equivalence for new ObjectTreeViewDefaultModel column enum value";
			}
		}

		ASSERT_EQ(treeValue, objValue);	
	}
}

std::vector<raco::core::SEditorObject> ObjectTreeViewDefaultModelTest::createAllSceneGraphObjects() {
	std::vector<SEditorObject> allSceneGraphNodes;
	for (const auto &typeName : getTypes()) {
		auto newNode = createNodes(typeName, {typeName});
		if (Queries::isNotResource(newNode.front()) && typeName != Prefab::typeDescription.typeName) {
			allSceneGraphNodes.emplace_back(newNode.front());
		}
	}

	return allSceneGraphNodes;
}

std::vector<raco::core::SEditorObject> ObjectTreeViewDefaultModelTest::createAllResources() {
	std::vector<raco::core::SEditorObject> allResources;
	for (const auto &typeName : getTypes()) {
		auto newNode = createNodes(typeName, {typeName});
		if (raco::core::Queries::isResource(newNode.front())) {
			allResources.emplace_back(newNode.front());
		}
	}

	return allResources;
}


TEST_F(ObjectTreeViewDefaultModelTest, TreeBuildingSimple) {
	auto singleNodeName = "Test";
	nodeNames_ = {singleNodeName};

	auto singleNode = createNodes(MeshNode::typeDescription.typeName, nodeNames_).front();

	auto singleNodeIndex = viewModel_->index(0, 0);
	ASSERT_FALSE(viewModel_->parent(singleNodeIndex).isValid());
	compareValuesInTree(singleNode, singleNodeIndex, *viewModel_);
}


TEST_F(ObjectTreeViewDefaultModelTest, TreeBuildingThreeRootNodes) {
	nodeNames_ = {"Test1", "Test2", "Test3"};

	auto createdNodes = createNodes(Node::typeDescription.typeName, nodeNames_);

	for (size_t i = 0; i < nodeNames_.size(); ++i) {
		auto currentNodeModel = viewModel_->index(i, 0);

		ASSERT_FALSE(viewModel_->parent(currentNodeModel).isValid());
		compareValuesInTree(createdNodes[i], currentNodeModel, *viewModel_);
	}
}


// Scene Graph structure:
// rootNode : Node
// - childNode : Node
TEST_F(ObjectTreeViewDefaultModelTest, TreeBuildingOneParentOneChild) {
	nodeNames_ = {"rootNode", "childNode"};

	auto createdNodes = createNodes(Node::typeDescription.typeName, nodeNames_);
	auto rootNode = createdNodes.front();
	auto childNode = createdNodes.back();
	moveScenegraphChildren({childNode}, rootNode);

	auto rootNodeModelIndex = viewModel_->index(0, ObjectTreeViewDefaultModel::COLUMNINDEX_NAME);
	auto childNodeModelIndex = viewModel_->index(0, ObjectTreeViewDefaultModel::COLUMNINDEX_NAME, rootNodeModelIndex);

	auto compareParentRelationshipPerColumn = [this, &rootNodeModelIndex, &childNodeModelIndex](auto column) {
		auto parent = viewModel_->indexToTreeNode(rootNodeModelIndex);
		auto child = viewModel_->indexToTreeNode(childNodeModelIndex);
		auto invisibleRootNode = viewModel_->indexToTreeNode({});
		
		ASSERT_EQ(parent->getParent()->getRepresentedObject(), invisibleRootNode->getRepresentedObject());
		ASSERT_EQ(parent->getChildren().size(), 1);
		ASSERT_EQ(parent->getChild(0), child);
		ASSERT_EQ(child->getParent(), parent);		
	};

	for (auto i = 0; i < ObjectTreeViewDefaultModel::COLUMNINDEX_COLUMN_COUNT; ++i) {
		compareParentRelationshipPerColumn(i);
	}

	compareValuesInTree(rootNode, rootNodeModelIndex, *viewModel_);
	compareValuesInTree(childNode, childNodeModelIndex, *viewModel_);
}


// Scene Graph structure:
// rootNode1 : Node
// - childNode1 : MeshNode
// - childNode2 : MeshNode
// rootNode2 : Node
TEST_F(ObjectTreeViewDefaultModelTest, TreeBuildingOneRootHasTwoChildrenOtherRootDoesNot) {
	nodeNames_ = {"rootNode1"};
	auto rootNode1 = createNodes(Node::typeDescription.typeName, nodeNames_).front();

	nodeNames_ = {"childNode1"};
	auto childNode1 = createNodes(MeshNode::typeDescription.typeName, nodeNames_).front();
	moveScenegraphChildren({childNode1}, rootNode1);

	nodeNames_ = {"childNode2"};
	auto childNode2 = createNodes(MeshNode::typeDescription.typeName, nodeNames_).front();
	moveScenegraphChildren({childNode2}, rootNode1);

	nodeNames_ = {"rootNode2"};
	auto rootNode2 = createNodes(Node::typeDescription.typeName, nodeNames_).front();

	auto invisibleRootNode = viewModel_->indexToTreeNode({});
	ASSERT_EQ(invisibleRootNode->childCount(), 2);	
	
	auto firstRootLeaf = invisibleRootNode->getChild(0);
	ASSERT_EQ(rootNode1, firstRootLeaf->getRepresentedObject());
	ASSERT_EQ(firstRootLeaf->childCount(), 2);

	auto firstChildNode = firstRootLeaf->getChild(0);
	ASSERT_EQ(childNode1, firstChildNode->getRepresentedObject());

	auto secondChildNode = firstRootLeaf->getChild(1);
	ASSERT_EQ(childNode2, secondChildNode->getRepresentedObject());

	auto secondRootLeaf = invisibleRootNode->getChild(1);
	ASSERT_EQ(rootNode2, secondRootLeaf->getRepresentedObject());
	ASSERT_EQ(secondRootLeaf->childCount(), 0);
}


// Scene Graph structure:
// node1 : Node
// - node2 : Node
// -- node3 : Node
// --- node4 : Node
// ---- node5 : Node
// ----- node6 : Node
// ------ node7 : Node
// ------- node8 : Node
// -------- node9 : Node
// --------- node10 : Node
TEST_F(ObjectTreeViewDefaultModelTest, TreeBuildingNastyNesting) {
	constexpr auto NODE_AMOUNT = 10;
	static_assert(NODE_AMOUNT > 1, "NODE_AMOUNT in ObjectTreeViewDefaultModelTest::NastyNesting needs to be larger than 1");

	for (int i = 1; i <= NODE_AMOUNT; ++i) {
		nodeNames_.emplace_back("node" + std::to_string(i));
	}

	auto createdNodes = createNodes(Node::typeDescription.typeName, nodeNames_);

	for (size_t i = 0; i < createdNodes.size() - 1; ++i) {
		moveScenegraphChildren({createdNodes[i + 1]}, createdNodes[i]);
	}

	QModelIndex parent;
	for (int i = 0; i < NODE_AMOUNT - 1; ++i) {
		auto currentIndex = viewModel_->index(0, ObjectTreeViewDefaultModel::COLUMNINDEX_NAME, parent);
		auto actualParent = viewModel_->parent(currentIndex);
		ASSERT_EQ(parent, actualParent);
		parent = currentIndex;
	}
}


TEST_F(ObjectTreeViewDefaultModelTest, SceneGraphMoveCreateTwoNodes) {
	nodeNames_ = {"rootNode", "childNode"};

	auto wrongItemIndex = viewModel_->indexFromTreeNodeID("nothing");
	ASSERT_FALSE(wrongItemIndex.isValid());

	auto createdNodes = createNodes(Node::typeDescription.typeName, nodeNames_);
	auto rootIndex = viewModel_->indexFromTreeNodeID(createdNodes.front()->objectID());
	auto childIndex = viewModel_->indexFromTreeNodeID(createdNodes.back()->objectID());

	ASSERT_EQ(rootIndex.row(), 0);
	ASSERT_FALSE(rootIndex.parent().isValid());
	ASSERT_EQ(childIndex.row(), 1);
	ASSERT_FALSE(childIndex.parent().isValid());
}


TEST_F(ObjectTreeViewDefaultModelTest, SceneGraphMoveCreateParentAndChild) {
	nodeNames_ = {"rootNode", "childNode"};

	auto createdNodes = createNodes(Node::typeDescription.typeName, nodeNames_);
	auto rootNode = createdNodes.front();
	auto childNode = createdNodes.back();

	moveScenegraphChildren({childNode}, rootNode);
	ASSERT_EQ(childNode->getParent(), rootNode);

	auto rootIndex = viewModel_->indexFromTreeNodeID(rootNode->objectID());
	auto childIndex = viewModel_->indexFromTreeNodeID(childNode->objectID());

	ASSERT_EQ(rootIndex.row(), 0);
	ASSERT_FALSE(rootIndex.parent().isValid());
	ASSERT_EQ(childIndex.row(), 0);
	ASSERT_EQ(childIndex.parent(), rootIndex);

	ASSERT_EQ(viewModel_->indexToTreeNode(rootIndex)->childCount(), 1);

	auto rootChildIndex = rootIndex.child(0, 0);
	ASSERT_EQ(rootChildIndex.row(), childIndex.row());
}


TEST_F(ObjectTreeViewDefaultModelTest, SceneGraphMoveDontAllowMovingObjectIntoItself) {
	nodeNames_ = {"rootNode"};

	auto createdNodes = createNodes(Node::typeDescription.typeName, nodeNames_);
	auto rootNode = createdNodes.front();

	moveScenegraphChildren({rootNode}, rootNode);
	auto rootIndex = viewModel_->indexFromTreeNodeID(createdNodes.front()->objectID());

	ASSERT_EQ(rootNode->getParent(), nullptr);
	ASSERT_EQ(rootIndex.row(), 0);
	ASSERT_FALSE(rootIndex.parent().isValid());
}


TEST_F(ObjectTreeViewDefaultModelTest, SceneGraphMoveDontAllowMovingParentIntoChild) {
	nodeNames_ = {"rootNode", "childNode"};

	auto createdNodes = createNodes(Node::typeDescription.typeName, nodeNames_);
	auto rootNode = createdNodes.front();
	auto childNode = createdNodes.back();

	moveScenegraphChildren({childNode}, rootNode);
	ASSERT_EQ(childNode->getParent(), rootNode);

	moveScenegraphChildren({rootNode}, childNode);
	auto rootIndex = viewModel_->indexFromTreeNodeID(createdNodes.front()->objectID());
	auto childIndex = viewModel_->indexFromTreeNodeID(createdNodes.back()->objectID());

	ASSERT_EQ(childNode->getParent(), rootNode);
	ASSERT_EQ(rootNode->getParent(), nullptr);
	ASSERT_EQ(rootIndex.row(), 0);
	ASSERT_FALSE(rootIndex.parent().isValid());
	ASSERT_EQ(childIndex.row(), 0);
	ASSERT_EQ(childIndex.parent(), rootIndex);
}


TEST_F(ObjectTreeViewDefaultModelTest, SceneGraphMoveParentMidAndChildProperHierarchy) {
	nodeNames_ = {
		"root",
		"mid",
		"child"};

	auto createdNodes = createNodes(Node::typeDescription.typeName, nodeNames_);
	for (size_t i = 0; i < createdNodes.size() - 1; ++i) {
		moveScenegraphChildren({createdNodes[i + 1]}, createdNodes[i]);
	}

	for (size_t i = 1; i < createdNodes.size(); ++i) {
		ASSERT_EQ(createdNodes[i]->getParent(), createdNodes[i - 1]);

		auto currentNodeIndex = viewModel_->indexFromTreeNodeID(createdNodes[i]->objectID());
		auto parentNodeIndex = viewModel_->indexFromTreeNodeID(createdNodes[i - 1]->objectID());
		ASSERT_EQ(currentNodeIndex.parent(), parentNodeIndex);
	}
}


TEST_F(ObjectTreeViewDefaultModelTest, SceneGraphMoveParentMidAndChildDontMoveParentsIntoChildren) {
	nodeNames_ = {
		"root",
		"mid",
		"child"};

	auto createdNodes = createNodes(Node::typeDescription.typeName, nodeNames_);
	for (size_t i = 0; i < createdNodes.size() - 1; ++i) {
		moveScenegraphChildren({createdNodes[i + 1]}, createdNodes[i]);
	}

	moveScenegraphChildren({createdNodes[0]}, createdNodes[1]);
	moveScenegraphChildren({createdNodes[1]}, createdNodes[2]);
	moveScenegraphChildren({createdNodes[0]}, createdNodes[2]);

	for (size_t i = 1; i < createdNodes.size(); ++i) {
		ASSERT_EQ(createdNodes[i]->getParent(), createdNodes[i - 1]);

		auto currentNodeIndex = viewModel_->indexFromTreeNodeID(createdNodes[i]->objectID());
		auto parentNodeIndex = viewModel_->indexFromTreeNodeID(createdNodes[i - 1]->objectID());
		ASSERT_EQ(currentNodeIndex.parent(), parentNodeIndex);
	}
}


TEST_F(ObjectTreeViewDefaultModelTest, SceneGraphMoveParentMidAndChildFlattenHierarchy) {
	nodeNames_ = {
		"root",
		"mid",
		"child"};

	auto createdNodes = createNodes(Node::typeDescription.typeName, nodeNames_);
	for (size_t i = 0; i < createdNodes.size() - 1; ++i) {
		moveScenegraphChildren({createdNodes[i + 1]}, createdNodes[i]);
	}

	auto rootNode = createdNodes.front();
	auto midNode = createdNodes[1];
	auto childNode = createdNodes.back();

	moveScenegraphChildren({childNode}, rootNode);
	moveScenegraphChildren({midNode}, rootNode);

	auto rootNodeChildren = rootNode->children_->asVector<SEditorObject>();
	ASSERT_EQ(rootNodeChildren.size(), 2);
	ASSERT_EQ(rootNodeChildren.front(), childNode);
	ASSERT_EQ(rootNodeChildren.back(), midNode);

	ASSERT_TRUE(midNode->children_->asVector<SEditorObject>().empty());
	ASSERT_EQ(midNode->getParent(), rootNode);

	ASSERT_TRUE(childNode->children_->asVector<SEditorObject>().empty());
	ASSERT_EQ(childNode->getParent(), rootNode);
}


TEST_F(ObjectTreeViewDefaultModelTest, SceneGraphMoveParentChildrenDifferentMovingIndex) {
	nodeNames_ = {
		"root",
		"child1", "child2", "child3"
	};

	auto createdNodes = createNodes(Node::typeDescription.typeName, nodeNames_);
	for (size_t i = 1; i < createdNodes.size(); ++i) {
		moveScenegraphChildren({createdNodes[i]}, createdNodes[0], 0);
	}

	auto rootNode = createdNodes.front();
	auto childAmount = createdNodes.size() - 1;
	
	auto rootChildren = rootNode->children_->asVector<SEditorObject>();
	ASSERT_EQ(rootChildren.size(), childAmount);

	auto rootTreeNode = viewModel_->indexToTreeNode(viewModel_->indexFromTreeNodeID(rootNode->objectID()));
	ASSERT_EQ(rootTreeNode->childCount(), childAmount);

	for (int i = 1; i <= childAmount; ++i) {
		ASSERT_EQ(rootChildren[i - 1], createdNodes[createdNodes.size() - i]);
		ASSERT_EQ(rootTreeNode->getChild(i - 1)->getRepresentedObject(), createdNodes[createdNodes.size() - i]);
	}
}


TEST_F(ObjectTreeViewDefaultModelTest, SceneGraphMoveInitiatedFromContext) {
	nodeNames_ = {"rootNode", "childNode"};

	auto createdNodes = createNodes(Node::typeDescription.typeName, nodeNames_);
	auto rootNode = createdNodes.front();
	auto childNode = createdNodes.back();

	auto rootNodeChildren = rootNode->children_->asVector<SEditorObject>();
	ASSERT_TRUE(rootNodeChildren.empty());

	commandInterface().moveScenegraphChildren({childNode}, {rootNode});
	dispatch();
	
	rootNodeChildren = rootNode->children_->asVector<SEditorObject>();
	ASSERT_EQ(rootNodeChildren.size(), 1);
	ASSERT_EQ(rootNodeChildren.front(), childNode);
}


TEST_F(ObjectTreeViewDefaultModelTest, SceneGraphMoveWrongIndex) {
	nodeNames_ = {"rootNode", "childNode"};

	auto createdNodes = createNodes(Node::typeDescription.typeName, nodeNames_);
	auto rootNode = createdNodes.front();
	auto childNode = createdNodes.back();

	ASSERT_THROW(commandInterface().moveScenegraphChildren({childNode}, {rootNode}, 1), std::runtime_error);
}


TEST_F(ObjectTreeViewDefaultModelTest, SceneGraphMoveMovingObjectsAround) {
	nodeNames_ = {
		"root",
		"child1", "child2", "child3"
	};

	auto createdNodes = createNodes(Node::typeDescription.typeName, nodeNames_);
	for (size_t i = 1; i < createdNodes.size(); ++i) {
		moveScenegraphChildren({createdNodes[i]}, createdNodes[0]);
	}

	auto rootNode = createdNodes.front();
	auto movedChild = createdNodes[1];
	
	auto checkRootChildenOrder = [&rootNode, &createdNodes](const std::vector<int> &expectedChildrenIndexes) {
		auto rootChildren = rootNode->children_->asVector<SEditorObject>();
		ASSERT_EQ(rootChildren.size(), expectedChildrenIndexes.size());

		for (size_t i = 0; i < rootChildren.size(); ++i) {
			ASSERT_EQ(rootChildren[i]->objectID(), createdNodes[expectedChildrenIndexes[i]]->objectID());
		}
	};
	
	moveScenegraphChildren({movedChild}, rootNode, 0);
	checkRootChildenOrder({1, 2, 3});

	moveScenegraphChildren({movedChild}, rootNode, 1);
	checkRootChildenOrder({1, 2, 3});

	moveScenegraphChildren({movedChild}, rootNode, 2);
	checkRootChildenOrder({2, 1, 3});

	moveScenegraphChildren({movedChild}, rootNode, 3);
	checkRootChildenOrder({2, 3, 1});

	moveScenegraphChildren({movedChild}, rootNode, 2);
	checkRootChildenOrder({2, 3, 1});

	moveScenegraphChildren({movedChild}, rootNode, 1);
	checkRootChildenOrder({2, 1, 3});

	moveScenegraphChildren({movedChild}, rootNode, 0);
	checkRootChildenOrder({1, 2, 3});

	moveScenegraphChildren({movedChild}, {});
	checkRootChildenOrder({2, 3});

	moveScenegraphChildren({movedChild}, {}, 3);
	checkRootChildenOrder({2, 3});
}


TEST_F(ObjectTreeViewDefaultModelTest, ObjectDeletionJustOneObject) {
	auto justANode = createNodes(Node::typeDescription.typeName, {"Node"}).front();

	ASSERT_EQ(project().instances().size(), 2);

	auto justAnIndex = viewModel_->indexFromTreeNodeID(justANode->objectID());

	ASSERT_EQ(deleteObjectsAtIndices({justAnIndex}), 1);
	ASSERT_EQ(project().instances().size(), 1);

	justAnIndex = viewModel_->indexFromTreeNodeID(justANode->objectID());
	ASSERT_FALSE(justAnIndex.isValid());
}


TEST_F(ObjectTreeViewDefaultModelTest, ObjectDeletionChildNode) {
	nodeNames_ = {"rootNode", "childNode"};

	auto createdNodes = createNodes(Node::typeDescription.typeName, nodeNames_);
	auto parentNode = createdNodes.front();
	auto childNode = createdNodes.back();
	moveScenegraphChildren({childNode}, parentNode);

	auto childIndex = viewModel_->indexFromTreeNodeID(childNode->objectID());
	ASSERT_EQ(deleteObjectsAtIndices({childIndex}), 1);
	ASSERT_EQ(project().instances().size(), 2);

	auto parentIndex = viewModel_->indexFromTreeNodeID(parentNode->objectID());
	ASSERT_EQ(viewModel_->indexToTreeNode(parentIndex)->childCount(), 0);

	childIndex = viewModel_->indexFromTreeNodeID(childNode->objectID());
	ASSERT_FALSE(childIndex.isValid());
}


TEST_F(ObjectTreeViewDefaultModelTest, ObjectDeletionParentNode) {
	nodeNames_ = {
		"root", 
		"child1", "child2"};

	auto createdNodes = createNodes(Node::typeDescription.typeName, nodeNames_);
	auto parentNode = createdNodes.front();
	auto childNode1 = createdNodes[1];
	auto childNode2 = createdNodes.back();
	moveScenegraphChildren({childNode1}, parentNode);
	moveScenegraphChildren({childNode2}, parentNode);

	auto parentIndex = viewModel_->indexFromTreeNodeID(parentNode->objectID());
	ASSERT_EQ(deleteObjectsAtIndices({parentIndex}), 3);
	ASSERT_EQ(project().instances().size(), 1);

	ASSERT_FALSE(viewModel_->indexFromTreeNodeID(parentNode->objectID()).isValid());
	ASSERT_FALSE(viewModel_->indexFromTreeNodeID(childNode1->objectID()).isValid());
	ASSERT_FALSE(viewModel_->indexFromTreeNodeID(childNode2->objectID()).isValid());
}

TEST_F(ObjectTreeViewDefaultModelTest, ObjectDeletionMidNode) {
	nodeNames_ = {
		"root", 
		"mid", 
		"child1", "child2"};

	auto createdNodes = createNodes(Node::typeDescription.typeName, nodeNames_);
	auto parentNode = createdNodes.front();
	auto midNode = createdNodes[1];
	auto childNode1 = createdNodes[2];
	auto childNode2 = createdNodes.back();
	moveScenegraphChildren({midNode}, parentNode);
	moveScenegraphChildren({childNode1}, midNode);
	moveScenegraphChildren({childNode2}, midNode);

	auto midIndex = viewModel_->indexFromTreeNodeID(midNode->objectID());
	ASSERT_EQ(deleteObjectsAtIndices({midIndex}), 3);
	ASSERT_EQ(project().instances().size(), 2);

	ASSERT_TRUE(viewModel_->indexFromTreeNodeID(parentNode->objectID()).isValid());
	ASSERT_FALSE(viewModel_->indexFromTreeNodeID(midNode->objectID()).isValid());
	ASSERT_FALSE(viewModel_->indexFromTreeNodeID(childNode1->objectID()).isValid());
	ASSERT_FALSE(viewModel_->indexFromTreeNodeID(childNode2->objectID()).isValid());

}

TEST_F(ObjectTreeViewDefaultModelTest, ObjectDeletionMultiSelectionIncludingParent) {
	nodeNames_ = {
		"root",
		"child1", "child2"};

	auto createdNodes = createNodes(Node::typeDescription.typeName, nodeNames_);
	auto parentNode = createdNodes.front();
	auto childNode1 = createdNodes[1];
	auto childNode2 = createdNodes.back();
	moveScenegraphChildren({childNode1}, parentNode);
	moveScenegraphChildren({childNode2}, parentNode);

	auto parentIndex = viewModel_->indexFromTreeNodeID(parentNode->objectID());
	auto child1Index = viewModel_->indexFromTreeNodeID(childNode1->objectID());
	ASSERT_EQ(deleteObjectsAtIndices({parentIndex, child1Index}), 3);
	ASSERT_EQ(project().instances().size(), 1);

	ASSERT_FALSE(viewModel_->indexFromTreeNodeID(parentNode->objectID()).isValid());
	ASSERT_FALSE(viewModel_->indexFromTreeNodeID(childNode1->objectID()).isValid());
	ASSERT_FALSE(viewModel_->indexFromTreeNodeID(childNode2->objectID()).isValid());
}

TEST_F(ObjectTreeViewDefaultModelTest, ObjectDeletionMultiSelectionExcludingParent) {
	nodeNames_ = {
		"root",
		"child1", "child2"};

	auto createdNodes = createNodes(Node::typeDescription.typeName, nodeNames_);
	auto parentNode = createdNodes.front();
	auto childNode1 = createdNodes[1];
	auto childNode2 = createdNodes.back();
	moveScenegraphChildren({childNode1}, parentNode);
	moveScenegraphChildren({childNode2}, parentNode);

	auto child1Index = viewModel_->indexFromTreeNodeID(childNode1->objectID());
	auto child2Index = viewModel_->indexFromTreeNodeID(childNode2->objectID());
	ASSERT_EQ(deleteObjectsAtIndices({child1Index, child2Index}), 2);
	ASSERT_EQ(project().instances().size(), 2);

	ASSERT_TRUE(viewModel_->indexFromTreeNodeID(parentNode->objectID()).isValid());
	ASSERT_FALSE(viewModel_->indexFromTreeNodeID(childNode1->objectID()).isValid());
	ASSERT_FALSE(viewModel_->indexFromTreeNodeID(childNode2->objectID()).isValid());
}

TEST_F(ObjectTreeViewDefaultModelTest, ObjectDeletionMultiSelectionIncludingPrefabInstanceChild) {
	auto prefab = create<Prefab>("prefab");
	auto prefabInstance = create<PrefabInstance>("prefabInstance");
	auto prefabChild = create<Node>("prefabChild");
	auto node = create<Node>("node");
	auto nodeChild = create<Node>("nodeChild");
	commandInterface().set({ prefabInstance, {"template"} }, prefab);
	commandInterface().moveScenegraphChildren({ nodeChild }, node);
	commandInterface().moveScenegraphChildren({ prefabChild }, prefab);
	viewModel_->buildObjectTree();
	auto prefabInstanceChild = prefabInstance->children_->asVector<SEditorObject>()[0]->as<Node>();

	auto nodeIndex = viewModel_->indexFromTreeNodeID(node->objectID());
	auto nodeChildIndex = viewModel_->indexFromTreeNodeID(nodeChild->objectID());
	auto prefabInstanceIndex = viewModel_->indexFromTreeNodeID(prefabInstance->objectID());
	auto prefabInstanceChildIndex = viewModel_->indexFromTreeNodeID(prefabInstanceChild->objectID());

	ASSERT_EQ(deleteObjectsAtIndices({ prefabInstanceChildIndex }), 0);
	nodeIndex = viewModel_->indexFromTreeNodeID(node->objectID());
	nodeChildIndex = viewModel_->indexFromTreeNodeID(nodeChild->objectID());
	prefabInstanceIndex = viewModel_->indexFromTreeNodeID(prefabInstance->objectID());
	prefabInstanceChildIndex = viewModel_->indexFromTreeNodeID(prefabInstanceChild->objectID());
	ASSERT_TRUE(nodeIndex.isValid());
	ASSERT_TRUE(nodeChildIndex.isValid());
	ASSERT_TRUE(prefabInstanceIndex.isValid());
	ASSERT_TRUE(prefabInstanceChildIndex.isValid());

	ASSERT_EQ(deleteObjectsAtIndices({ prefabInstanceChildIndex, nodeIndex }), 2);
	nodeIndex = viewModel_->indexFromTreeNodeID(node->objectID());
	nodeChildIndex = viewModel_->indexFromTreeNodeID(nodeChild->objectID());
	prefabInstanceIndex = viewModel_->indexFromTreeNodeID(prefabInstance->objectID());
	prefabInstanceChildIndex = viewModel_->indexFromTreeNodeID(prefabInstanceChild->objectID());
	ASSERT_FALSE(nodeIndex.isValid());
	ASSERT_FALSE(nodeChildIndex.isValid());
	ASSERT_TRUE(prefabInstanceIndex.isValid());
	ASSERT_TRUE(prefabInstanceChildIndex.isValid());

	ASSERT_EQ(deleteObjectsAtIndices({ prefabInstanceIndex, prefabInstanceChildIndex }), 2);
	prefabInstanceIndex = viewModel_->indexFromTreeNodeID(prefabInstance->objectID());
	prefabInstanceChildIndex = viewModel_->indexFromTreeNodeID(prefabInstanceChild->objectID());
	ASSERT_FALSE(prefabInstanceIndex.isValid());
	ASSERT_FALSE(prefabInstanceChildIndex.isValid());
}


TEST_F(ObjectTreeViewDefaultModelTest, TypesAllowedIntoIndexEmptyIndex) {
	auto allowedTypes = viewModel_->typesAllowedIntoIndex({});
	std::vector<std::string> allowedTypesAssert{ Animation::typeDescription.typeName,
		LuaInterface::typeDescription.typeName,
		LuaScript::typeDescription.typeName,
		MeshNode::typeDescription.typeName,
		Node::typeDescription.typeName,
		OrthographicCamera::typeDescription.typeName,
		PerspectiveCamera::typeDescription.typeName,
		PrefabInstance::typeDescription.typeName,
		Skin::typeDescription.typeName};

	EXPECT_EQ(allowedTypes, allowedTypesAssert);
}

TEST_F(ObjectTreeViewDefaultModelTest, TypesAllowedIntoIndexInvalidParent) {
	auto prefabInstance = createNodes(PrefabInstance::typeDescription.typeName, { "prefabInstance" }).front();
	auto luaScript = createNodes(LuaScript::typeDescription.typeName, { "luaScript" }).front();
	auto animation = createNodes(Animation::typeDescription.typeName, { "animation" }).front();

	ASSERT_TRUE(viewModel_->typesAllowedIntoIndex(viewModel_->indexFromTreeNodeID(prefabInstance->objectID())).empty());
	ASSERT_TRUE(viewModel_->typesAllowedIntoIndex(viewModel_->indexFromTreeNodeID(luaScript->objectID())).empty());
	ASSERT_TRUE(viewModel_->typesAllowedIntoIndex(viewModel_->indexFromTreeNodeID(animation->objectID())).empty());
}

TEST_F(ObjectTreeViewDefaultModelTest, TypesAllowedIntoIndexNode) {
	auto node = createNodes(Node::typeDescription.typeName, { "node" }).front();

	auto allowedTypes = viewModel_->typesAllowedIntoIndex(viewModel_->indexFromTreeNodeID(node->objectID()));
	std::vector<std::string> allowedTypesAssert{Animation::typeDescription.typeName,
		LuaInterface::typeDescription.typeName,
		LuaScript::typeDescription.typeName,
		MeshNode::typeDescription.typeName,
		Node::typeDescription.typeName,
		OrthographicCamera::typeDescription.typeName,
		PerspectiveCamera::typeDescription.typeName,
		PrefabInstance::typeDescription.typeName,
		Skin::typeDescription.typeName};

	EXPECT_EQ(allowedTypes, allowedTypesAssert);
}

TEST_F(ObjectTreeViewDefaultModelTest, AllowedObjsSceneGraphObjectsAreAllowedWithEmptyIndex) {
	for (const auto& typeName : getTypes()) {
		auto newObj = commandInterface().createObject(typeName);
		if (Queries::isNotResource(newObj) && typeName != Prefab::typeDescription.typeName) {
			ASSERT_TRUE(viewModel_->isObjectAllowedIntoIndex({}, newObj));
		}
	}
}

TEST_F(ObjectTreeViewDefaultModelTest, AllowedObjsCheckAllSceneGraphObjectCombinations) {
	auto allSceneGraphNodes = createAllSceneGraphObjects();

	for (const auto &typeName : getTypes()) {
		auto newObj = commandInterface().createObject(typeName);
		if (Queries::isNotResource(newObj) && typeName != Prefab::typeDescription.typeName) {
			for (const auto &sceneGraphNodeInScene : allSceneGraphNodes) {
				auto sceneObjIndex = viewModel_->indexFromTreeNodeID(sceneGraphNodeInScene->objectID());
				auto pastingSomethingUnderPrefabInstance = sceneGraphNodeInScene->as<PrefabInstance>();
				auto pastingSomethingUnderLuaScript = sceneGraphNodeInScene->as<LuaScript>();
				auto pastingSomethingUnderLuaInterface = sceneGraphNodeInScene->as<LuaInterface>();
				auto pastingSomethingUnderAnimaton = sceneGraphNodeInScene->as<Animation>();
				auto pastingSomethingUnderSkin = sceneGraphNodeInScene->as<Skin>();

				if (pastingSomethingUnderPrefabInstance || pastingSomethingUnderLuaScript || pastingSomethingUnderLuaInterface || pastingSomethingUnderAnimaton || pastingSomethingUnderSkin) {
					ASSERT_FALSE(viewModel_->isObjectAllowedIntoIndex(sceneObjIndex, newObj));
				} else {
					ASSERT_TRUE(viewModel_->isObjectAllowedIntoIndex(sceneObjIndex, newObj));
				}
			}
		}
	}
}

TEST_F(ObjectTreeViewDefaultModelTest, DuplicationCanDuplicateAllTopLevelItems) {
	auto allSceneGraphNodes = createAllSceneGraphObjects();

	for (const auto &sceneGraphNodeInScene : allSceneGraphNodes) {
		auto sceneObjIndex = viewModel_->indexFromTreeNodeID(sceneGraphNodeInScene->objectID());
		ASSERT_TRUE(viewModel_->canDuplicateAtIndices({sceneObjIndex}));
	}
}

TEST_F(ObjectTreeViewDefaultModelTest, DuplicationCanDuplicateAllChildren) {
	auto allSceneGraphNodes = createAllSceneGraphObjects();

	auto parent = createNodes(Node::typeDescription.typeName, {"Parent"}).front();
	moveScenegraphChildren(allSceneGraphNodes, parent);

	for (const auto &sceneGraphNodeInScene : allSceneGraphNodes) {
		auto sceneObjIndex = viewModel_->indexFromTreeNodeID(sceneGraphNodeInScene->objectID());
		ASSERT_TRUE(viewModel_->canDuplicateAtIndices({sceneObjIndex}));
	}
}

TEST_F(ObjectTreeViewDefaultModelTest, DuplicationParent) {
	auto parent = createNodes(Node::typeDescription.typeName, {"Parent"}).front();

	auto duplicatedObjs = viewModel_->duplicateObjectsAtIndices({viewModel_->indexFromTreeNodeID(parent->objectID())});

	ASSERT_EQ(duplicatedObjs.size(), 1);
	ASSERT_EQ(duplicatedObjs.front()->getParent(), nullptr);
	ASSERT_NE(duplicatedObjs.front()->objectID(), parent->objectID());
}

TEST_F(ObjectTreeViewDefaultModelTest, DuplicationChild) {
	auto hierarchy = createNodes(Node::typeDescription.typeName, {"Parent", "Child"});

	moveScenegraphChildren({hierarchy[1]}, hierarchy[0]);

	auto duplicatedObjs = viewModel_->duplicateObjectsAtIndices({viewModel_->indexFromTreeNodeID(hierarchy[1]->objectID())});

	ASSERT_EQ(duplicatedObjs.size(), 1);
	ASSERT_EQ(duplicatedObjs.front()->getParent(), hierarchy[0]);
	ASSERT_NE(duplicatedObjs.front()->objectID(), hierarchy[1]->objectID());
}

TEST_F(ObjectTreeViewDefaultModelTest, DuplicationCanNotDuplicateHierarchy) {
	auto allSceneGraphNodes = createAllSceneGraphObjects();

	auto parent = createNodes(Node::typeDescription.typeName, {"Parent"}).front();
	moveScenegraphChildren(allSceneGraphNodes, parent);

	for (const auto &sceneGraphNodeInScene : allSceneGraphNodes) {
		auto sceneObjIndex = viewModel_->indexFromTreeNodeID(sceneGraphNodeInScene->objectID());
		auto parentIndex = viewModel_->indexFromTreeNodeID(parent->objectID());
		ASSERT_FALSE(viewModel_->canDuplicateAtIndices({parentIndex, sceneObjIndex}));
	}
}

TEST_F(ObjectTreeViewDefaultModelTest, DuplicationCanNotDuplicatePrefabInstanceChildren) {
	auto allSceneGraphNodes = createAllSceneGraphObjects();
	auto prefab = createNodes(Prefab::typeDescription.typeName, {"Template"}).front();
	auto prefabInstance = createNodes(PrefabInstance::typeDescription.typeName, {"Instance"}).front();

	commandInterface().set({prefabInstance, &PrefabInstance::template_}, prefab);

	moveScenegraphChildren(allSceneGraphNodes, prefab);

	for (const auto &child : prefabInstance->children_->asVector<SEditorObject>()) {
		auto sceneObjIndex = viewModel_->indexFromTreeNodeID(child->objectID());		
		ASSERT_FALSE(viewModel_->canDuplicateAtIndices({sceneObjIndex}));
	}
}

TEST_F(ObjectTreeViewDefaultModelTest, DuplicationCanNotDuplicateNestedPrefabInstanceChildren) {
	auto allSceneGraphNodes = createAllSceneGraphObjects();
	auto prefab1 = createNodes(Prefab::typeDescription.typeName, {"Prefab1"}).front();
	auto prefabInstance1 = createNodes(PrefabInstance::typeDescription.typeName, {"Instance1"}).front();

	auto prefab2 = createNodes(Prefab::typeDescription.typeName, {"Prefab1"}).front();
	auto prefabInstance2 = createNodes(PrefabInstance::typeDescription.typeName, {"Instance2"}).front();

	moveScenegraphChildren(allSceneGraphNodes, prefab1);
	moveScenegraphChildren({prefabInstance1}, prefab2);

	commandInterface().set({prefabInstance1, &PrefabInstance::template_}, prefab1);
	commandInterface().set({prefabInstance2, &PrefabInstance::template_}, prefab2);

	viewModel_->buildObjectTree();

	for (const auto &child : prefabInstance2->children_->asVector<SEditorObject>()[0]->children_->asVector<SEditorObject>()) {
		auto sceneObjIndex = viewModel_->indexFromTreeNodeID(child->objectID());
		ASSERT_FALSE(viewModel_->canDuplicateAtIndices({sceneObjIndex}));
	}
}

TEST_F(ObjectTreeViewDefaultModelTest, DuplicationCanNotDuplicateChildAndUncle) {
	auto allSceneGraphNodes = createAllSceneGraphObjects();
	auto nodes = createNodes(Node::typeDescription.typeName, {"Grandparent", "Parent", "Child", "Uncle"});
	auto grandparent = nodes[0];
	auto parent = nodes[1];
	auto child = nodes[2];
	auto uncle = nodes[3];
	
	moveScenegraphChildren({parent}, grandparent);
	moveScenegraphChildren({uncle}, grandparent);
	moveScenegraphChildren({child}, parent);

	viewModel_->buildObjectTree();
	auto childIndex = viewModel_->indexFromTreeNodeID(child->objectID());
	auto uncleIndex = viewModel_->indexFromTreeNodeID(uncle->objectID());
	ASSERT_FALSE(viewModel_->canDuplicateAtIndices({childIndex, uncleIndex}));
}

TEST_F(ObjectTreeViewDefaultModelTest, DuplicationCanNotDuplicateNothing) {
	ASSERT_FALSE(viewModel_->canDuplicateAtIndices({}));
}

TEST_F(ObjectTreeViewDefaultModelTest, AllowedObjsResourcesAreNotAllowedOnTopLevel) {
	for (const auto &typeName : getTypes()) {
		auto newObj = commandInterface().createObject(typeName);
		if (Queries::isResource(newObj)) {
			ASSERT_FALSE(viewModel_->isObjectAllowedIntoIndex({}, newObj));
		}
	}
}

TEST_F(ObjectTreeViewDefaultModelTest, AllowedObjsResourcesAreNotAllowedUnderSceneGraphObjects) {
	auto allSceneGraphNodes = createAllSceneGraphObjects();

	for (const auto &typeName : getTypes()) {
		auto newObj = commandInterface().createObject(typeName);
		if (Queries::isResource(newObj)) {
			for (const auto &sceneGraphNodeInScene : allSceneGraphNodes) {
				auto sceneObjIndex = viewModel_->indexFromTreeNodeID(sceneGraphNodeInScene->objectID());
				ASSERT_FALSE(viewModel_->isObjectAllowedIntoIndex(sceneObjIndex, newObj));
			}
		}
	}
}

TEST_F(ObjectTreeViewDefaultModelTest, AllowedObjsPrefabsAreNotAllowed) {
	auto prefab = commandInterface().createObject(Prefab::typeDescription.typeName);

	auto allSceneGraphNodes = createAllSceneGraphObjects();

	ASSERT_FALSE(viewModel_->isObjectAllowedIntoIndex({}, prefab));
	for (const auto &sceneGraphNodeInScene : allSceneGraphNodes) {
		auto sceneObjIndex = viewModel_->indexFromTreeNodeID(sceneGraphNodeInScene->objectID());
		ASSERT_FALSE(viewModel_->isObjectAllowedIntoIndex(sceneObjIndex, prefab));
	}
}

TEST_F(ObjectTreeViewDefaultModelTest, AllowedObjsCheckPrefabInstanceCombinations) {
	auto prefabInstance = commandInterface().createObject(PrefabInstance::typeDescription.typeName);

	auto allSceneGraphNodes = createAllSceneGraphObjects();

	ASSERT_TRUE(viewModel_->isObjectAllowedIntoIndex({}, prefabInstance));
	for (const auto &sceneGraphNodeInScene : allSceneGraphNodes) {
		auto sceneObjIndex = viewModel_->indexFromTreeNodeID(sceneGraphNodeInScene->objectID());
		auto pastingPrefabInstanceUnderPrefabInstance = sceneGraphNodeInScene->as<PrefabInstance>();
		auto pastingPrefabInstanceUnderLuaScript = sceneGraphNodeInScene->as<LuaScript>();
		auto pastingPrefabInstanceUnderLuaInterface = sceneGraphNodeInScene->as<LuaInterface>();
		auto pastingPrefabInstanceUnderAnimation = sceneGraphNodeInScene->as<Animation>();
		auto pastingSomethingUnderSkin = sceneGraphNodeInScene->as<Skin>();

		if (pastingPrefabInstanceUnderPrefabInstance || pastingPrefabInstanceUnderLuaScript || pastingPrefabInstanceUnderLuaInterface || pastingPrefabInstanceUnderAnimation || pastingSomethingUnderSkin) {
			ASSERT_FALSE(viewModel_->isObjectAllowedIntoIndex(sceneObjIndex, prefabInstance));
		} else {
			ASSERT_TRUE(viewModel_->isObjectAllowedIntoIndex(sceneObjIndex, prefabInstance));
		}
	}
}

TEST_F(ObjectTreeViewDefaultModelTest, CanCopyAtIndicesMultiSelection) {
	auto createdNodes = createNodes(Node::typeDescription.typeName, {"parent1", "child1", "parent2"});
	auto parentNode1 = createdNodes[0];
	auto childNode1 = createdNodes[1];
	auto parentNode2 = createdNodes[2];
	moveScenegraphChildren({childNode1}, parentNode1);
	auto parent1Index = viewModel_->indexFromTreeNodeID(parentNode1->objectID());
	auto child1Index = viewModel_->indexFromTreeNodeID(childNode1->objectID());
	auto parent2Index = viewModel_->indexFromTreeNodeID(parentNode2->objectID());

	ASSERT_FALSE(viewModel_->canCopyAtIndices({}));
	ASSERT_FALSE(viewModel_->canCopyAtIndices({{}}));
	ASSERT_TRUE(viewModel_->canCopyAtIndices({parent1Index}));
	ASSERT_TRUE(viewModel_->canCopyAtIndices({parent1Index, {}}));
	ASSERT_TRUE(viewModel_->canCopyAtIndices({child1Index, parent2Index}));
	ASSERT_TRUE(viewModel_->canCopyAtIndices({parent1Index, child1Index, parent2Index}));
	ASSERT_TRUE(viewModel_->canCopyAtIndices({{}, parent1Index, child1Index, parent2Index, {}}));
}


TEST_F(ObjectTreeViewDefaultModelTest, CanDeleteAtIndicesMultiSelection) {
	auto prefab = create<Prefab>("prefab");
	auto prefabInstance = create<PrefabInstance>("prefabInstance");
	auto prefabChild = create<Node>("prefabChild");
	auto node = create<Node>("node");
	auto nodeChild = create<Node>("nodeChild");
	commandInterface().set({prefabInstance, {"template"}}, prefab);
	commandInterface().moveScenegraphChildren({nodeChild}, node);
	commandInterface().moveScenegraphChildren({prefabChild}, prefab);
	viewModel_->buildObjectTree();
	auto prefabInstanceChild = prefabInstance->children_->asVector<SEditorObject>()[0]->as<Node>();

	auto nodeIndex = viewModel_->indexFromTreeNodeID(node->objectID());
	auto nodeChildIndex = viewModel_->indexFromTreeNodeID(nodeChild->objectID());
	auto prefabInstanceIndex = viewModel_->indexFromTreeNodeID(prefabInstance->objectID());
	auto prefabInstanceChildIndex = viewModel_->indexFromTreeNodeID(prefabInstanceChild->objectID());

	ASSERT_FALSE(viewModel_->canDeleteAtIndices({}));
	ASSERT_FALSE(viewModel_->canDeleteAtIndices({{}}));
	ASSERT_FALSE(viewModel_->canDeleteAtIndices({prefabInstanceChildIndex}));
	ASSERT_TRUE(viewModel_->canDeleteAtIndices({nodeIndex}));
	ASSERT_TRUE(viewModel_->canDeleteAtIndices({nodeIndex, {}}));
	ASSERT_TRUE(viewModel_->canDeleteAtIndices({nodeChildIndex, prefabInstanceIndex}));
	ASSERT_TRUE(viewModel_->canDeleteAtIndices({nodeIndex, nodeChildIndex, prefabInstanceIndex}));
	ASSERT_TRUE(viewModel_->canDeleteAtIndices({{}, nodeIndex, nodeChildIndex, prefabInstanceIndex, {}}));
	ASSERT_TRUE(viewModel_->canDeleteAtIndices({prefabInstanceIndex, prefabInstanceChildIndex}));
}

TEST_F(ObjectTreeViewDefaultModelTest, AllowedObjsDeepCopiedSceneGraphWithResourcesIsAllowed) {
	auto meshNode = createNodes(MeshNode::typeDescription.typeName, {MeshNode::typeDescription.typeName}).front();
	auto mesh = createNodes(Mesh::typeDescription.typeName, {Mesh::typeDescription.typeName}).front();

	commandInterface().set(raco::core::ValueHandle{meshNode, {"mesh"}}, mesh);
	dispatch();

	auto cutObjs = commandInterface().cutObjects({meshNode}, true);
	dispatch();

	auto [parsedObjs, sourceProjectTopLevelObjectIds] = viewModel_->getObjectsAndRootIdsFromClipboardString(cutObjs);
	ASSERT_TRUE(viewModel_->canPasteIntoIndex({}, parsedObjs, sourceProjectTopLevelObjectIds));
}

TEST_F(ObjectTreeViewDefaultModelTest, AllowedObjsDeepCopiedPrefabInstanceWithPrefabIsAllowed) {
	auto prefabInstance = createNodes(PrefabInstance::typeDescription.typeName, {PrefabInstance::typeDescription.typeName}).front();
	auto prefab = createNodes(Prefab::typeDescription.typeName, {Prefab::typeDescription.typeName}).front();

	commandInterface().set(raco::core::ValueHandle{prefabInstance, {"template"}}, prefab);
	dispatch();

	auto cutObjs = commandInterface().cutObjects({prefabInstance}, true);
	dispatch();

	auto [parsedObjs, sourceProjectTopLevelObjectIds] = viewModel_->getObjectsAndRootIdsFromClipboardString(cutObjs);
	ASSERT_TRUE(viewModel_->canPasteIntoIndex({}, parsedObjs, sourceProjectTopLevelObjectIds));
}

TEST_F(ObjectTreeViewDefaultModelTest, AllowedObjsLuaScriptIsAllowedAsExtRefOnTopLevel) {
	auto luaScript = createNodes(LuaScript::typeDescription.typeName, {LuaScript::typeDescription.typeName}).front();
	luaScript->addAnnotation(std::make_shared<raco::core::ExternalReferenceAnnotation>("differentProject"));
	dispatch();

	auto cutObjs = commandInterface().cutObjects({luaScript}, true);
	dispatch();

	auto [parsedObjs, sourceProjectTopLevelObjectIds] = viewModel_->getObjectsAndRootIdsFromClipboardString(cutObjs);
	ASSERT_TRUE(viewModel_->canPasteIntoIndex({}, parsedObjs, sourceProjectTopLevelObjectIds, true));
}


TEST_F(ObjectTreeViewDefaultModelTest, AllowedObjsChildLuaScriptIsNotAllowedAsExtRef) {
	auto luaScript = createNodes(LuaScript::typeDescription.typeName, {LuaScript::typeDescription.typeName}).front();
	auto externalParentNode = createNodes(Node::typeDescription.typeName, {Node::typeDescription.typeName}).front();
	auto localParentNode = createNodes(Node::typeDescription.typeName, {"localParent"}).front();
	moveScenegraphChildren({luaScript}, externalParentNode);

	externalParentNode->addAnnotation(std::make_shared<raco::core::ExternalReferenceAnnotation>("differentProject"));
	luaScript->addAnnotation(std::make_shared<raco::core::ExternalReferenceAnnotation>("differentProject"));
	dispatch();

	auto cutObjs = commandInterface().cutObjects({luaScript}, true);
	ASSERT_EQ(cutObjs, "");
	dispatch();

	auto [parsedObjs, sourceProjectTopLevelObjectIds] = viewModel_->getObjectsAndRootIdsFromClipboardString(cutObjs);
	ASSERT_FALSE(viewModel_->canPasteIntoIndex({}, parsedObjs, sourceProjectTopLevelObjectIds, true));
	ASSERT_FALSE(viewModel_->canPasteIntoIndex(viewModel_->indexFromTreeNodeID(localParentNode->objectID()), parsedObjs, sourceProjectTopLevelObjectIds, true));
}

TEST_F(ObjectTreeViewDefaultModelTest, PasteLuaScriptAsExtRefNotAllowedAsChild) {
	auto luaScript = createNodes(LuaScript::typeDescription.typeName, {LuaScript::typeDescription.typeName}).front();
	auto localParentNode = createNodes(Node::typeDescription.typeName, {"localParent"}).front();
	luaScript->addAnnotation(std::make_shared<raco::core::ExternalReferenceAnnotation>("differentProject"));
	dispatch();

	auto cutObjs = commandInterface().cutObjects({luaScript}, true);
	dispatch();

	auto [parsedObjs, sourceProjectTopLevelObjectIds] = viewModel_->getObjectsAndRootIdsFromClipboardString(cutObjs);
	ASSERT_FALSE(viewModel_->canPasteIntoIndex(viewModel_->indexFromTreeNodeID(localParentNode->objectID()), parsedObjs, sourceProjectTopLevelObjectIds, true));
	ASSERT_TRUE(viewModel_->canPasteIntoIndex({}, parsedObjs, sourceProjectTopLevelObjectIds, true));
}

TEST_F(ObjectTreeViewDefaultModelTest, PasteLuaScriptAsExtRefNotAllowedWhenChildInExternalProject) {
	auto luaScript = createNodes(LuaScript::typeDescription.typeName, {LuaScript::typeDescription.typeName}).front();
	auto externalParentNode = createNodes(Node::typeDescription.typeName, {"externalParentNode"}).front();
	auto localParentNode = createNodes(Node::typeDescription.typeName, {"localParent"}).front();
	moveScenegraphChildren({luaScript}, externalParentNode);
	luaScript->addAnnotation(std::make_shared<raco::core::ExternalReferenceAnnotation>("differentProject"));
	dispatch();

	auto cutObjs = commandInterface().cutObjects({luaScript}, true);
	dispatch();

	auto [parsedObjs, sourceProjectTopLevelObjectIds] = viewModel_->getObjectsAndRootIdsFromClipboardString(cutObjs);
	ASSERT_FALSE(viewModel_->canPasteIntoIndex({}, parsedObjs, sourceProjectTopLevelObjectIds, true));
}

TEST_F(ObjectTreeViewDefaultModelTest, move_toplevel) {
	auto node = create<Node>("node");
	dispatch();

	auto mimeData = viewModel_->mimeData({viewModel_->indexFromTreeNodeID(node->objectID())});
	ASSERT_TRUE(viewModel_->canDropMimeData(mimeData, Qt::DropAction::MoveAction, 0, 0, {}));
}

TEST_F(ObjectTreeViewDefaultModelTest, CanNotMoveNodesInsidePrefabInstance) {
	auto node = create<Node>("node");
	auto prefab = create<Prefab>("prefab");
	auto prefabInstance = create<PrefabInstance>("prefabInstance");
	dispatch();

	moveScenegraphChildren({node}, prefab);
	dispatch();

	commandInterface().set({prefabInstance, &raco::user_types::PrefabInstance::template_}, prefab);
	dispatch();

	auto mimeData = viewModel_->mimeData({viewModel_->indexFromTreeNodeID(prefabInstance->children_->asVector<SEditorObject>().front()->objectID())});
	ASSERT_FALSE(viewModel_->canDropMimeData(mimeData, Qt::DropAction::MoveAction, 0, 0, {}));
}

TEST_F(ObjectTreeViewDefaultModelTest, move_to_self_child) {
	auto node = create<Node>("node");
	auto child = create<Node>("child", node);
	dispatch();

	auto childIndex = viewModel_->indexFromTreeNodeID(child->objectID());
	auto mimeData = viewModel_->mimeData({viewModel_->indexFromTreeNodeID(node->objectID())});
	ASSERT_FALSE(viewModel_->canDropMimeData(mimeData, Qt::DropAction::MoveAction, 0, 0, childIndex));
}

TEST_F(ObjectTreeViewDefaultModelTest, move_to_other_child) {
	auto node = create<Node>("node");
	auto child = create<Node>("child", node);
	auto move = create<Node>("move");
	dispatch();

	auto childIndex = viewModel_->indexFromTreeNodeID(child->objectID());
	auto mimeData = viewModel_->mimeData({viewModel_->indexFromTreeNodeID(move->objectID())});
	ASSERT_TRUE(viewModel_->canDropMimeData(mimeData, Qt::DropAction::MoveAction, 0, 0, childIndex));
}

TEST_F(ObjectTreeViewDefaultModelTest, CanCopyAndPasteObjectThatIsAlreadyInModel) {
	auto node = createNodes(Node::typeDescription.typeName, {Node::typeDescription.typeName}).front();
	auto copiedObjs = commandInterface().copyObjects({node});
	dispatch();

	auto [parsedObjs, sourceProjectTopLevelObjectIds] = viewModel_->getObjectsAndRootIdsFromClipboardString(copiedObjs);
	ASSERT_TRUE(viewModel_->canPasteIntoIndex({}, parsedObjs, sourceProjectTopLevelObjectIds));
}