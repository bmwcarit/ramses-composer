/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "gtest/gtest.h"

#include "ObjectTreeViewDefaultModel_test.h"
#include "object_tree_view_model/ObjectTreeNode.h"
#include "user_types/LuaScript.h"
#include "user_types/Material.h"
#include "user_types/Mesh.h"
#include "user_types/MeshNode.h"
#include "user_types/Node.h"

#include <QApplication>

using namespace raco::core;
using namespace raco::object_tree::model;
using namespace raco::user_types;

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
			case (ObjectTreeViewDefaultModel::COLUMNINDEX_TYPE): {
				treeValue = objTreeNode->getRepresentedObject()->getTypeDescription().typeName;
				objValue = obj->getTypeDescription().typeName;
			}
			break;
			case ObjectTreeViewDefaultModel::COLUMNINDEX_PROJECT: {
				treeValue = project.getProjectNameForObject(objTreeNode->getRepresentedObject());
				objValue = project.getProjectNameForObject(obj);
			} break;
			default: {
				FAIL() << "Need to check value equivalence for new ObjectTreeViewDefaultModel column enum value";
			}
		}

		ASSERT_EQ(treeValue, objValue);	
	}
}


TEST_F(ObjectTreeViewDefaultModelTest, TreeBuildingSimple) {
	auto singleNodeName = "Test";
	nodeNames_ = {singleNodeName};

	auto singleNode = createNodes(MeshNode::typeDescription.typeName, nodeNames_).front();

	auto singleNodeIndex = viewModel_.index(0, 0);
	ASSERT_EQ(viewModel_.parent(singleNodeIndex), viewModel_.getInvisibleRootIndex());
	compareValuesInTree(singleNode, singleNodeIndex, viewModel_);
}


TEST_F(ObjectTreeViewDefaultModelTest, TreeBuildingThreeRootNodes) {
	nodeNames_ = {"Test1", "Test2", "Test3"};

	auto createdNodes = createNodes(Node::typeDescription.typeName, nodeNames_);

	for (size_t i = 0; i < nodeNames_.size(); ++i) {
		auto currentNodeModel = viewModel_.index(i, 0);

		ASSERT_EQ(viewModel_.parent(currentNodeModel).row(), viewModel_.getInvisibleRootIndex().row());
		compareValuesInTree(createdNodes[i], currentNodeModel, viewModel_);
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
	moveScenegraphChild(childNode, rootNode);

	auto rootNodeModelIndex = viewModel_.index(0, ObjectTreeViewDefaultModel::COLUMNINDEX_NAME);
	auto childNodeModelIndex = viewModel_.index(0, ObjectTreeViewDefaultModel::COLUMNINDEX_NAME, rootNodeModelIndex);

	auto compareParentRelationshipPerColumn = [this, &rootNodeModelIndex, &childNodeModelIndex](auto column) {
		auto parent = viewModel_.indexToTreeNode(rootNodeModelIndex);
		auto child = viewModel_.indexToTreeNode(childNodeModelIndex);
		auto invisibleRootNode = viewModel_.indexToTreeNode(viewModel_.getInvisibleRootIndex());
		
		ASSERT_EQ(parent->getParent()->getRepresentedObject(), invisibleRootNode->getRepresentedObject());
		ASSERT_EQ(parent->getChildren().size(), 1);
		ASSERT_EQ(parent->getChild(0), child);
		ASSERT_EQ(child->getParent(), parent);		
	};

	for (auto i = 0; i < ObjectTreeViewDefaultModel::COLUMNINDEX_COLUMN_COUNT; ++i) {
		compareParentRelationshipPerColumn(i);
	}

	compareValuesInTree(rootNode, rootNodeModelIndex, viewModel_);
	compareValuesInTree(childNode, childNodeModelIndex, viewModel_);
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
	moveScenegraphChild(childNode1, rootNode1);

	nodeNames_ = {"childNode2"};
	auto childNode2 = createNodes(MeshNode::typeDescription.typeName, nodeNames_).front();
	moveScenegraphChild(childNode2, rootNode1);

	nodeNames_ = {"rootNode2"};
	auto rootNode2 = createNodes(Node::typeDescription.typeName, nodeNames_).front();

	auto invisibleRoot = viewModel_.getInvisibleRootIndex();
	auto invisibleRootNode = viewModel_.indexToTreeNode(viewModel_.getInvisibleRootIndex());
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
		moveScenegraphChild(createdNodes[i + 1], createdNodes[i]);
	}

	QModelIndex parent = viewModel_.getInvisibleRootIndex();
	for (int i = 0; i < NODE_AMOUNT - 1; ++i) {
		auto currentIndex = viewModel_.index(0, ObjectTreeViewDefaultModel::COLUMNINDEX_NAME, parent);
		auto actualParent = viewModel_.parent(currentIndex);
		ASSERT_EQ(parent, actualParent);
		parent = currentIndex;
	}
}


TEST_F(ObjectTreeViewDefaultModelTest, SceneGraphMoveCreateTwoNodes) {
	nodeNames_ = {"rootNode", "childNode"};

	auto wrongItemIndex = viewModel_.indexFromObjectID("nothing");
	ASSERT_FALSE(wrongItemIndex.isValid());

	auto createdNodes = createNodes(Node::typeDescription.typeName, nodeNames_);
	auto rootIndex = viewModel_.indexFromObjectID(createdNodes.front()->objectID());
	auto childIndex = viewModel_.indexFromObjectID(createdNodes.back()->objectID());

	ASSERT_EQ(rootIndex.row(), 0);
	ASSERT_EQ(rootIndex.parent(), viewModel_.getInvisibleRootIndex());
	ASSERT_EQ(childIndex.row(), 1);
	ASSERT_EQ(childIndex.parent(), viewModel_.getInvisibleRootIndex());
}


TEST_F(ObjectTreeViewDefaultModelTest, SceneGraphMoveCreateParentAndChild) {
	nodeNames_ = {"rootNode", "childNode"};

	auto createdNodes = createNodes(Node::typeDescription.typeName, nodeNames_);
	auto rootNode = createdNodes.front();
	auto childNode = createdNodes.back();

	moveScenegraphChild(childNode, rootNode);
	ASSERT_EQ(childNode->getParent(), rootNode);

	auto rootIndex = viewModel_.indexFromObjectID(rootNode->objectID());
	auto childIndex = viewModel_.indexFromObjectID(childNode->objectID());

	ASSERT_EQ(rootIndex.row(), 0);
	ASSERT_EQ(rootIndex.parent(), viewModel_.getInvisibleRootIndex());
	ASSERT_EQ(childIndex.row(), 0);
	ASSERT_EQ(childIndex.parent(), rootIndex);

	ASSERT_EQ(viewModel_.indexToTreeNode(rootIndex)->childCount(), 1);

	auto rootChildIndex = rootIndex.child(0, 0);
	ASSERT_EQ(rootChildIndex.row(), childIndex.row());
}


TEST_F(ObjectTreeViewDefaultModelTest, SceneGraphMoveDontAllowMovingObjectIntoItself) {
	nodeNames_ = {"rootNode"};

	auto createdNodes = createNodes(Node::typeDescription.typeName, nodeNames_);
	auto rootNode = createdNodes.front();

	moveScenegraphChild(rootNode, rootNode);
	auto rootIndex = viewModel_.indexFromObjectID(createdNodes.front()->objectID());

	ASSERT_EQ(rootNode->getParent(), nullptr);
	ASSERT_EQ(rootIndex.row(), 0);
	ASSERT_EQ(rootIndex.parent(), viewModel_.getInvisibleRootIndex());
}


TEST_F(ObjectTreeViewDefaultModelTest, SceneGraphMoveDontAllowMovingParentIntoChild) {
	nodeNames_ = {"rootNode", "childNode"};

	auto createdNodes = createNodes(Node::typeDescription.typeName, nodeNames_);
	auto rootNode = createdNodes.front();
	auto childNode = createdNodes.back();

	moveScenegraphChild(childNode, rootNode);
	ASSERT_EQ(childNode->getParent(), rootNode);

	moveScenegraphChild(rootNode, childNode);
	auto rootIndex = viewModel_.indexFromObjectID(createdNodes.front()->objectID());
	auto childIndex = viewModel_.indexFromObjectID(createdNodes.back()->objectID());

	ASSERT_EQ(childNode->getParent(), rootNode);
	ASSERT_EQ(rootNode->getParent(), nullptr);
	ASSERT_EQ(rootIndex.row(), 0);
	ASSERT_EQ(rootIndex.parent(), viewModel_.getInvisibleRootIndex());
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
		moveScenegraphChild(createdNodes[i + 1], createdNodes[i]);
	}

	for (size_t i = 1; i < createdNodes.size(); ++i) {
		ASSERT_EQ(createdNodes[i]->getParent(), createdNodes[i - 1]);

		auto currentNodeIndex = viewModel_.indexFromObjectID(createdNodes[i]->objectID());
		auto parentNodeIndex = viewModel_.indexFromObjectID(createdNodes[i - 1]->objectID());
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
		moveScenegraphChild(createdNodes[i + 1], createdNodes[i]);
	}

	moveScenegraphChild(createdNodes[0], createdNodes[1]);
	moveScenegraphChild(createdNodes[1], createdNodes[2]);
	moveScenegraphChild(createdNodes[0], createdNodes[2]);

	for (size_t i = 1; i < createdNodes.size(); ++i) {
		ASSERT_EQ(createdNodes[i]->getParent(), createdNodes[i - 1]);

		auto currentNodeIndex = viewModel_.indexFromObjectID(createdNodes[i]->objectID());
		auto parentNodeIndex = viewModel_.indexFromObjectID(createdNodes[i - 1]->objectID());
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
		moveScenegraphChild(createdNodes[i + 1], createdNodes[i]);
	}

	auto rootNode = createdNodes.front();
	auto midNode = createdNodes[1];
	auto childNode = createdNodes.back();

	moveScenegraphChild(childNode, rootNode);
	moveScenegraphChild(midNode, rootNode);

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
		moveScenegraphChild(createdNodes[i], createdNodes[0], 0);
	}

	auto rootNode = createdNodes.front();
	auto childAmount = createdNodes.size() - 1;
	
	auto rootChildren = rootNode->children_->asVector<SEditorObject>();
	ASSERT_EQ(rootChildren.size(), childAmount);

	auto rootTreeNode = viewModel_.indexToTreeNode(viewModel_.indexFromObjectID(rootNode->objectID()));
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

	commandInterface.moveScenegraphChild({childNode}, {rootNode});
	dataChangeDispatcher_->dispatch(recorder);
	
	rootNodeChildren = rootNode->children_->asVector<SEditorObject>();
	ASSERT_EQ(rootNodeChildren.size(), 1);
	ASSERT_EQ(rootNodeChildren.front(), childNode);
}


TEST_F(ObjectTreeViewDefaultModelTest, SceneGraphMoveWrongIndex) {
	nodeNames_ = {"rootNode", "childNode"};

	auto createdNodes = createNodes(Node::typeDescription.typeName, nodeNames_);
	auto rootNode = createdNodes.front();
	auto childNode = createdNodes.back();

	ASSERT_DEATH(commandInterface.moveScenegraphChild({childNode}, {rootNode}, 1), "");
}


TEST_F(ObjectTreeViewDefaultModelTest, SceneGraphMoveMovingObjectsAround) {
	nodeNames_ = {
		"root",
		"child1", "child2", "child3"
	};

	auto createdNodes = createNodes(Node::typeDescription.typeName, nodeNames_);
	for (size_t i = 1; i < createdNodes.size(); ++i) {
		moveScenegraphChild(createdNodes[i], createdNodes[0]);
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
	
	moveScenegraphChild(movedChild, rootNode, 0);
	checkRootChildenOrder({1, 2, 3});

	moveScenegraphChild(movedChild, rootNode, 1);
	checkRootChildenOrder({1, 2, 3});

	moveScenegraphChild(movedChild, rootNode, 2);
	checkRootChildenOrder({2, 1, 3});

	moveScenegraphChild(movedChild, rootNode, 3);
	checkRootChildenOrder({2, 3, 1});

	moveScenegraphChild(movedChild, rootNode, 2);
	checkRootChildenOrder({2, 3, 1});

	moveScenegraphChild(movedChild, rootNode, 1);
	checkRootChildenOrder({2, 1, 3});

	moveScenegraphChild(movedChild, rootNode, 0);
	checkRootChildenOrder({1, 2, 3});

	moveScenegraphChild(movedChild, {});
	checkRootChildenOrder({2, 3});
}


TEST_F(ObjectTreeViewDefaultModelTest, ObjectDeletionJustOneObject) {
	auto justANode = createNodes(Node::typeDescription.typeName, {"Node"}).front();

	ASSERT_EQ(project.instances().size(), 1);

	auto justAnIndex = viewModel_.indexFromObjectID(justANode->objectID());
	auto delObjAmount = deleteObjectAtIndex(justAnIndex);

	ASSERT_EQ(delObjAmount, 1);
	ASSERT_TRUE(project.instances().empty());

	justAnIndex = viewModel_.indexFromObjectID(justANode->objectID());
	ASSERT_FALSE(justAnIndex.isValid());
}


TEST_F(ObjectTreeViewDefaultModelTest, ObjectDeletionChildNode) {
	nodeNames_ = {"rootNode", "childNode"};

	auto createdNodes = createNodes(Node::typeDescription.typeName, nodeNames_);
	auto parentNode = createdNodes.front();
	auto childNode = createdNodes.back();
	moveScenegraphChild(childNode, parentNode);

	auto childIndex = viewModel_.indexFromObjectID(childNode->objectID());
	auto delObjAmount = deleteObjectAtIndex(childIndex);
	ASSERT_EQ(delObjAmount, 1);
	ASSERT_EQ(project.instances().size(), 1);

	auto parentIndex = viewModel_.indexFromObjectID(parentNode->objectID());
	ASSERT_EQ(viewModel_.indexToTreeNode(parentIndex)->childCount(), 0);

	childIndex = viewModel_.indexFromObjectID(childNode->objectID());
	ASSERT_FALSE(childIndex.isValid());
}


TEST_F(ObjectTreeViewDefaultModelTest, ObjectDeletionParentNode) {
	nodeNames_ = {
		"root", 
		"child1", "child2"};

	auto createdNodes = createNodes(Node::typeDescription.typeName, nodeNames_);
	auto parentNode = createdNodes.front();
	auto child1Node = createdNodes[1];
	auto child2Node = createdNodes.back();
	moveScenegraphChild(child1Node, parentNode);
	moveScenegraphChild(child2Node, parentNode);

	auto parentIndex = viewModel_.indexFromObjectID(parentNode->objectID());
	auto delObjAmount = deleteObjectAtIndex(parentIndex);
	ASSERT_EQ(delObjAmount, 3);
	ASSERT_TRUE(project.instances().empty());

	ASSERT_FALSE(viewModel_.indexFromObjectID(parentNode->objectID()).isValid());
	ASSERT_FALSE(viewModel_.indexFromObjectID(child1Node->objectID()).isValid());
	ASSERT_FALSE(viewModel_.indexFromObjectID(child2Node->objectID()).isValid());
}

TEST_F(ObjectTreeViewDefaultModelTest, ObjectDeletionMidNode) {
	nodeNames_ = {
		"root", 
		"mid", 
		"child1", "child2"};

	auto createdNodes = createNodes(Node::typeDescription.typeName, nodeNames_);
	auto parentNode = createdNodes.front();
	auto midNode = createdNodes[1];
	auto child1Node = createdNodes[2];
	auto child2Node = createdNodes.back();
	moveScenegraphChild(midNode, parentNode);
	moveScenegraphChild(child1Node, midNode);
	moveScenegraphChild(child2Node, midNode);

	auto midIndex = viewModel_.indexFromObjectID(midNode->objectID());
	auto delObjAmount = deleteObjectAtIndex(midIndex);
	ASSERT_EQ(delObjAmount, 3);
	ASSERT_EQ(project.instances().size(), 1);

	ASSERT_TRUE(viewModel_.indexFromObjectID(parentNode->objectID()).isValid());
	ASSERT_FALSE(viewModel_.indexFromObjectID(midNode->objectID()).isValid());
	ASSERT_FALSE(viewModel_.indexFromObjectID(child1Node->objectID()).isValid());
	ASSERT_FALSE(viewModel_.indexFromObjectID(child2Node->objectID()).isValid());
}