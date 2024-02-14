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
#include "ramses_adaptor/NodeAdaptor.h"
#include "user_types/Node.h"

using namespace raco;
using ramses_adaptor::NodeAdaptor;
using user_types::Node;
using user_types::SNode;
using core::ValueHandle;

class NodeAdaptorTest : public RamsesBaseFixture<> {};

 TEST_F(NodeAdaptorTest, constructor_doesntFail) {
	SNode node{new Node{}};
	NodeAdaptor adaptor{&sceneContext, node};
	EXPECT_TRUE(true);
}

TEST_F(NodeAdaptorTest, constructor_sets_RamsesObject_name) {
	const char* name{"SomeName"};
	SNode node{new Node{name}};
	NodeAdaptor adaptor{&sceneContext, node};

	EXPECT_TRUE(adaptor.ramsesObject().getName() == name);
}

TEST_F(NodeAdaptorTest, onDataChange_updatesTranslation) {
	auto node = context.createObject(Node::typeDescription.typeName, "SomeName");
	context.set(ValueHandle(node, {"translation", "x"}), 1.0);

	dispatch();

	auto engineNode{select<ramses::Node>(*sceneContext.scene(), "SomeName")};
	EXPECT_EQ(ramses_adaptor::getRamsesTranslation(engineNode), glm::vec3(1.0, 0.0, 0.0));
}


TEST_F(NodeAdaptorTest, context_node_name_change) {
	auto node = context.createObject(Node::typeDescription.typeName, "Node Name");

	dispatch();

	auto nodes{select<ramses::Node>(*sceneContext.scene(), ramses::ERamsesObjectType::Node)};

	EXPECT_EQ(nodes.size(), 1);
	EXPECT_TRUE("Node Name" == nodes[0]->getName());
	EXPECT_EQ(select<ramses::NodeBinding>(sceneContext.logicEngine(), "Node Name_NodeBinding")->getUserId(), node->objectIDAsRamsesLogicID());

	context.set({node, {"objectName"}}, std::string("Changed"));
	dispatch();

	EXPECT_TRUE("Changed" == nodes[0]->getName());
	EXPECT_EQ(select<ramses::NodeBinding>(sceneContext.logicEngine(), "Changed_NodeBinding")->getUserId(), node->objectIDAsRamsesLogicID());
}
