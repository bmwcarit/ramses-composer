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
using raco::ramses_adaptor::NodeAdaptor;
using raco::user_types::Node;
using raco::user_types::SNode;
using raco::core::ValueHandle;

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

	const char* ramsesObjectName = adaptor.ramsesObject().getName();
	EXPECT_EQ(sizeof(ramsesObjectName), sizeof(name));
	EXPECT_TRUE(0 == std::memcmp(ramsesObjectName, name, sizeof(ramsesObjectName)));
}

TEST_F(NodeAdaptorTest, onDataChange_updatesTranslation) {
	auto node = context.createObject(Node::typeDescription.typeName, "SomeName");
	context.set(ValueHandle(node, {"translation", "x"}), 1.0);

	dispatch();

	auto engineNode{select<ramses::MeshNode>(*sceneContext.scene(), "SomeName")};
    float x, y, z;
	engineNode->getTranslation(x, y, z);
    EXPECT_EQ(x, 1.0f);
    EXPECT_EQ(y, 0.0f);
    EXPECT_EQ(z, 0.0f);
}


TEST_F(NodeAdaptorTest, context_node_name_change) {
	auto node = context.createObject(Node::typeDescription.typeName, "Node Name");

	dispatch();

	auto nodes{select<ramses::Node>(*sceneContext.scene(), ramses::ERamsesObjectType::ERamsesObjectType_Node)};

	EXPECT_EQ(nodes.size(), 1);
	EXPECT_STREQ("Node Name", nodes[0]->getName());
	EXPECT_EQ(select<rlogic::RamsesNodeBinding>(sceneContext.logicEngine(), "Node Name_NodeBinding")->getUserId(), node->objectIDAsRamsesLogicID());

	context.set({node, {"objectName"}}, std::string("Changed"));
	dispatch();

	EXPECT_STREQ("Changed", nodes[0]->getName());
	EXPECT_EQ(select<rlogic::RamsesNodeBinding>(sceneContext.logicEngine(), "Changed_NodeBinding")->getUserId(), node->objectIDAsRamsesLogicID());
}
