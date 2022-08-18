/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "ObjectTreeViewMultipleModels_test.h"

using namespace raco::core;
using namespace raco::object_tree::model;
using namespace raco::user_types;


TEST_F(ObjectTreeViewMultipleModelsTest, MoveTopLevelNodeUnderPrefab) {
	auto node = createNodes(raco::user_types::Node::typeDescription.typeName, {"Node"}).front();
	moveScenegraphChildren({node}, prefab_);

	auto *prefabNode = prefabModel_.indexToTreeNode(prefabModel_.index(0, 0, {}));
	auto *viewModelRootNode = viewModel_->indexToTreeNode({});

	ASSERT_TRUE(viewModelRootNode->getChildren().empty());
	ASSERT_EQ(prefabNode->getChildren().size(), 1);
	ASSERT_EQ(prefabNode->getChild(0)->getRepresentedObject()->objectName(), "Node");
}

TEST_F(ObjectTreeViewMultipleModelsTest, MovePrefabNodeToTopLevel) {
	auto node = createNodes(raco::user_types::Node::typeDescription.typeName, {"Node"}).front();
	moveScenegraphChildren({node}, prefab_);
	moveScenegraphChildren({node}, nullptr);

	auto *prefabNode = prefabModel_.indexToTreeNode(prefabModel_.index(0, 0, {}));
	auto *viewModelRootNode = viewModel_->indexToTreeNode({});

	ASSERT_TRUE(prefabNode->getChildren().empty());
	ASSERT_EQ(viewModelRootNode->getChildren().size(), 1);
	ASSERT_EQ(viewModelRootNode->getChild(0)->getRepresentedObject()->objectName(), "Node");
}


TEST_F(ObjectTreeViewMultipleModelsTest, MoveChildNodeUnderPrefab) {
	auto nodeParent = createNodes(raco::user_types::Node::typeDescription.typeName, {"NodeParent"}).front();
	auto node = createNodes(raco::user_types::Node::typeDescription.typeName, {"Node"}).front();
	moveScenegraphChildren({node}, nodeParent);
	moveScenegraphChildren({node}, prefab_);

	auto *prefabNode = prefabModel_.indexToTreeNode(prefabModel_.index(0, 0, {}));
	auto *viewModelRootNode = viewModel_->indexToTreeNode({});

	ASSERT_EQ(viewModelRootNode->getChildren().size(), 1);
	ASSERT_EQ(viewModelRootNode->getChild(0)->getRepresentedObject()->objectName(), "NodeParent");
	ASSERT_TRUE(viewModelRootNode->getChild(0)->getChildren().empty());

	ASSERT_EQ(prefabNode->getChildren().size(), 1);
	ASSERT_EQ(prefabNode->getChild(0)->getRepresentedObject()->objectName(), "Node");
}

TEST_F(ObjectTreeViewMultipleModelsTest, MovePrefabNodeToNodeParent) {
	auto nodeParent = createNodes(raco::user_types::Node::typeDescription.typeName, {"NodeParent"}).front();
	auto node = createNodes(raco::user_types::Node::typeDescription.typeName, {"Node"}).front();
	moveScenegraphChildren({node}, prefab_);
	moveScenegraphChildren({node}, nodeParent);

	auto *prefabNode = prefabModel_.indexToTreeNode(prefabModel_.index(0, 0, {}));
	auto *viewModelRootNode = viewModel_->indexToTreeNode({});

	ASSERT_EQ(viewModelRootNode->getChildren().size(), 1);
	ASSERT_EQ(viewModelRootNode->getChild(0)->getRepresentedObject()->objectName(), "NodeParent");
	ASSERT_EQ(viewModelRootNode->getChild(0)->getChildren().size(), 1);
	ASSERT_EQ(viewModelRootNode->getChild(0)->getChild(0)->getRepresentedObject()->objectName(), "Node");

	ASSERT_TRUE(prefabNode->getChildren().empty());
}