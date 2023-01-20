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
#include "ramses_adaptor/SkinAdaptor.h"

using namespace raco::user_types;

class SkinAdaptorTest : public RamsesBaseFixture<> {};

TEST_F(SkinAdaptorTest, create_success) {
	auto material = create_material("material", "shaders/skinning-template.vert", "shaders/skinning-template.frag");
	auto mesh = create_mesh("mesh", "meshes/SimpleSkin/SimpleSkin.gltf");
	auto meshnode = create_meshnode("meshnode", mesh, material);
	commandInterface.set(meshnode->getMaterialPrivateHandle(0), true);

	auto node1 = create<Node>("node1");
	auto node2 = create<Node>("node2");

	auto skin = create_skin("skin", "meshes/SimpleSkin/SimpleSkin.gltf", 0, meshnode, {node1, node2});

	dispatch();

	EXPECT_FALSE(commandInterface.errors().hasError({skin}));

	ASSERT_EQ(sceneContext.logicEngine().getCollection<rlogic::SkinBinding>().size(), 1);
}

TEST_F(SkinAdaptorTest, error_no_meshnode) {
	auto material = create_material("material", "shaders/skinning-template.vert", "shaders/skinning-template.frag");
	auto mesh = create_mesh("mesh", "meshes/SimpleSkin/SimpleSkin.gltf");
	auto meshnode = create_meshnode("meshnode", mesh, material);
	commandInterface.set(meshnode->getMaterialPrivateHandle(0), true);

	auto node1 = create<Node>("node1");
	auto node2 = create<Node>("node2");

	auto skin = create_skin("skin", "meshes/SimpleSkin/SimpleSkin.gltf", 0, nullptr, {node1, node2});

	dispatch();
	EXPECT_TRUE(commandInterface.errors().hasError({skin}));
	ASSERT_EQ(sceneContext.logicEngine().getCollection<rlogic::SkinBinding>().size(), 0);
}

TEST_F(SkinAdaptorTest, error_node_missing_end) {
	auto material = create_material("material", "shaders/skinning-template.vert", "shaders/skinning-template.frag");
	auto mesh = create_mesh("mesh", "meshes/SimpleSkin/SimpleSkin.gltf");
	auto meshnode = create_meshnode("meshnode", mesh, material);
	commandInterface.set(meshnode->getMaterialPrivateHandle(0), true);

	auto node1 = create<Node>("node1");
	auto node2 = create<Node>("node2");

	auto skin = create_skin("skin", "meshes/SimpleSkin/SimpleSkin.gltf", 0, meshnode, {node1});

	dispatch();
	EXPECT_TRUE(commandInterface.errors().hasError({skin}));
	ASSERT_EQ(sceneContext.logicEngine().getCollection<rlogic::SkinBinding>().size(), 0);
}

TEST_F(SkinAdaptorTest, error_node_missing_front) {
	auto material = create_material("material", "shaders/skinning-template.vert", "shaders/skinning-template.frag");
	auto mesh = create_mesh("mesh", "meshes/SimpleSkin/SimpleSkin.gltf");
	auto meshnode = create_meshnode("meshnode", mesh, material);
	commandInterface.set(meshnode->getMaterialPrivateHandle(0), true);

	auto node1 = create<Node>("node1");
	auto node2 = create<Node>("node2");

	auto skin = create_skin("skin", "meshes/SimpleSkin/SimpleSkin.gltf", 0, meshnode, {nullptr, node2});

	dispatch();
	EXPECT_TRUE(commandInterface.errors().hasError({skin}));
	ASSERT_EQ(sceneContext.logicEngine().getCollection<rlogic::SkinBinding>().size(), 0);
}

TEST_F(SkinAdaptorTest, error_appearance_public) {
	auto material = create_material("material", "shaders/skinning-template.vert", "shaders/skinning-template.frag");
	auto mesh = create_mesh("mesh", "meshes/SimpleSkin/SimpleSkin.gltf");
	auto meshnode = create_meshnode("meshnode", mesh, material);
	commandInterface.set(meshnode->getMaterialPrivateHandle(0), false);

	auto node1 = create<Node>("node1");
	auto node2 = create<Node>("node2");

	auto skin = create_skin("skin", "meshes/SimpleSkin/SimpleSkin.gltf", 0, meshnode, {node1, node2});

	dispatch();
	EXPECT_TRUE(commandInterface.errors().hasError({skin}));
	ASSERT_EQ(sceneContext.logicEngine().getCollection<rlogic::SkinBinding>().size(), 0);
}
