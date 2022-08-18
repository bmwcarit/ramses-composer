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

#include "object_tree_view_model/ObjectTreeNode.h"

#include <array>

using namespace raco::object_tree::model;
using namespace raco::core;

TEST(ObjectTreeNodeTest, StructureChildGetsAdded) {
	auto parent = new ObjectTreeNode(ObjectTreeNodeType::Root, nullptr);
	auto child = new ObjectTreeNode(SEditorObject(), parent);

	ASSERT_EQ(parent->childCount(), 1);
	ASSERT_EQ(parent->getChild(0), child);
	ASSERT_EQ(child->getParent(), parent);

	delete parent;
}


TEST(ObjectTreeNodeTest, StructureParentGetsDeleted) {
	constexpr auto NODE_AMOUNT = 3;
	static_assert(NODE_AMOUNT > 0, "Need at least two nodes for ParentGetsDeleted unit test to work");

	std::array<ObjectTreeNode*, NODE_AMOUNT> nodes;

	auto *rootNode = nodes[0] = new ObjectTreeNode(ObjectTreeNodeType::Root, nullptr);
	for (auto i = 1; i < NODE_AMOUNT; ++i) {
		nodes[i] = new ObjectTreeNode(SEditorObject(), nodes[i - 1]);
	}
	delete rootNode;

	// assert that all nodes have been deallocated.
	for (auto &node : nodes) {
		ASSERT_DEATH(std::cout << node->getRepresentedObject()->objectID() << std::endl, ".*");
	}
}