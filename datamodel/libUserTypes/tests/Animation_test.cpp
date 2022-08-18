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

#include "user_types/Animation.h"
#include "user_types/AnimationChannel.h"
#include <gtest/gtest.h>

using namespace raco::core;
using namespace raco::user_types;

class AnimationTest : public TestEnvironmentCore {};

TEST_F(AnimationTest, newObjectNoErrors) {
	auto anim = commandInterface.createObject(Animation::typeDescription.typeName, "Animation Name");
	ASSERT_FALSE(commandInterface.errors().hasError(anim));
}


TEST_F(AnimationTest, emptyAnimChannelsErrors) {
	auto anim = commandInterface.createObject(Animation::typeDescription.typeName, "Animation Name");
	auto animChannel = commandInterface.createObject(AnimationChannel::typeDescription.typeName, "Animation Channel Name");

	commandInterface.set({anim, {"animationChannels", "Channel 0"}}, animChannel);
	ASSERT_TRUE(commandInterface.errors().hasError({anim, {"animationChannels", "Channel 0"}}));

	commandInterface.set({anim, {"animationChannels", "Channel 3"}}, animChannel);
	ASSERT_TRUE(commandInterface.errors().hasError({anim, {"animationChannels", "Channel 0"}}));
	ASSERT_TRUE(commandInterface.errors().hasError({anim, {"animationChannels", "Channel 3"}}));

	commandInterface.set({anim, {"animationChannels", "Channel 3"}}, SEditorObject());
	ASSERT_TRUE(commandInterface.errors().hasError({anim, {"animationChannels", "Channel 0"}}));
	ASSERT_FALSE(commandInterface.errors().hasError({anim, {"animationChannels", "Channel 3"}}));

	ValueHandle uriHandle{animChannel, {"uri"}};
	auto animChannelPath = test_path().append("meshes/CesiumMilkTruck/CesiumMilkTruck.gltf").string();
	commandInterface.set(uriHandle, animChannelPath);

	ASSERT_FALSE(commandInterface.errors().hasError({anim, {"animationChannels", "Channel 0"}}));
	ASSERT_FALSE(commandInterface.errors().hasError({anim, {"animationChannels", "Channel 3"}}));
}

TEST_F(AnimationTest, link_with_meshNode_mesh_changed) {
	auto anim = context.createObject(Animation::typeDescription.typeName, "Animation Name");
	auto animChannel = context.createObject(AnimationChannel::typeDescription.typeName, "Animation Sampler Name");
	auto mesh = context.createObject(Mesh::typeDescription.typeName, "Mesh");
	auto meshNode = context.createObject(MeshNode::typeDescription.typeName, "MeshNode");

	std::string uriPath{(test_path() / "meshes" / "InterpolationTest" / "InterpolationTest.gltf").string()};
	commandInterface.set({animChannel, &raco::user_types::AnimationChannel::uri_}, uriPath);
	commandInterface.set({mesh, &raco::user_types::Mesh::uri_}, uriPath);

	commandInterface.set({anim, {"animationChannels", "Channel 0"}}, animChannel);
	commandInterface.set({mesh, &raco::user_types::Mesh::bakeMeshes_}, false);
	commandInterface.set({meshNode, &raco::user_types::MeshNode::mesh_}, mesh);

	commandInterface.addLink({anim, {"outputs", "Ch0.Animation Sampler Name"}}, {meshNode, {"translation"}});

	commandInterface.set({mesh, &raco::user_types::Mesh::uri_}, (test_path() / "meshes" / "CesiumMilkTruck" / "CesiumMilkTruck.gltf").string());

	ASSERT_EQ(commandInterface.project()->links().size(), 1);
	ASSERT_TRUE(commandInterface.project()->links().front()->isValid());
}

TEST_F(AnimationTest, link_with_meshNode_submesh_index_changed) {
	auto anim = context.createObject(Animation::typeDescription.typeName, "Animation Name");
	auto animChannel = context.createObject(AnimationChannel::typeDescription.typeName, "Animation Sampler Name");
	auto mesh = context.createObject(Mesh::typeDescription.typeName, "Mesh");
	auto meshNode = context.createObject(MeshNode::typeDescription.typeName, "MeshNode");

	std::string uriPath{(test_path() / "meshes" / "InterpolationTest" / "InterpolationTest.gltf").string()};
	commandInterface.set({animChannel, &raco::user_types::AnimationChannel::uri_}, uriPath);
	commandInterface.set({mesh, &raco::user_types::Mesh::uri_}, uriPath);

	commandInterface.set({anim, {"animationChannels", "Channel 0"}}, animChannel);
	commandInterface.set({mesh, &raco::user_types::Mesh::bakeMeshes_}, false);
	commandInterface.set({meshNode, &raco::user_types::MeshNode::mesh_}, mesh);

	commandInterface.addLink({anim, {"outputs", "Ch0.Animation Sampler Name"}}, {meshNode, {"translation"}});

	commandInterface.set({mesh, {"meshIndex"}}, 1);

	ASSERT_EQ(commandInterface.project()->links().size(), 1);
	ASSERT_TRUE(commandInterface.project()->links().front()->isValid());
}

TEST_F(AnimationTest, link_with_meshNode_channel_data_changed_valid_type) {
	auto anim = context.createObject(Animation::typeDescription.typeName, "Animation Name");
	auto animChannel = context.createObject(AnimationChannel::typeDescription.typeName, "Animation Sampler Name");
	auto mesh = context.createObject(Mesh::typeDescription.typeName, "Mesh");
	auto meshNode = context.createObject(MeshNode::typeDescription.typeName, "MeshNode");

	std::string uriPath{(test_path() / "meshes" / "InterpolationTest" / "InterpolationTest.gltf").string()};
	commandInterface.set({animChannel, &raco::user_types::AnimationChannel::uri_}, uriPath);
	commandInterface.set({mesh, &raco::user_types::Mesh::uri_}, uriPath);

	commandInterface.set({anim, {"animationChannels", "Channel 0"}}, animChannel);
	commandInterface.set({mesh, &raco::user_types::Mesh::bakeMeshes_}, false);
	commandInterface.set({meshNode, &raco::user_types::MeshNode::mesh_}, mesh);

	commandInterface.addLink({anim, {"outputs", "Ch0.Animation Sampler Name"}}, {meshNode, {"translation"}});

	commandInterface.set({animChannel, &raco::user_types::AnimationChannel::animationIndex_}, 1);

	ASSERT_EQ(commandInterface.project()->links().size(), 1);
	ASSERT_TRUE(commandInterface.project()->links().front()->isValid());
}

TEST_F(AnimationTest, link_with_meshNode_channel_data_changed_invalid_type) {
	auto anim = context.createObject(Animation::typeDescription.typeName, "Animation Name");
	auto animChannel = context.createObject(AnimationChannel::typeDescription.typeName, "Animation Sampler Name");
	auto mesh = context.createObject(Mesh::typeDescription.typeName, "Mesh");
	auto meshNode = context.createObject(MeshNode::typeDescription.typeName, "MeshNode");

	std::string uriPath{(test_path() / "meshes" / "InterpolationTest" / "InterpolationTest.gltf").string()};
	commandInterface.set({animChannel, &raco::user_types::AnimationChannel::uri_}, uriPath);
	commandInterface.set({mesh, &raco::user_types::Mesh::uri_}, uriPath);

	commandInterface.set({anim, {"animationChannels", "Channel 0"}}, animChannel);
	commandInterface.set({mesh, &raco::user_types::Mesh::bakeMeshes_}, false);
	commandInterface.set({meshNode, &raco::user_types::MeshNode::mesh_}, mesh);

	commandInterface.addLink({anim, {"outputs", "Ch0.Animation Sampler Name"}}, {meshNode, {"translation"}});

	// changing from vec3f output to vec4f output
	commandInterface.set({animChannel, &raco::user_types::AnimationChannel::animationIndex_}, 3);

	ASSERT_EQ(commandInterface.project()->links().size(), 1);
	ASSERT_FALSE(commandInterface.project()->links().front()->isValid());
}

TEST_F(AnimationTest, link_with_meshNode_channel_removed) {
	auto anim = context.createObject(Animation::typeDescription.typeName, "Animation Name");
	auto animChannel = context.createObject(AnimationChannel::typeDescription.typeName, "Animation Sampler Name");
	auto mesh = context.createObject(Mesh::typeDescription.typeName, "Mesh");
	auto meshNode = context.createObject(MeshNode::typeDescription.typeName, "MeshNode");

	std::string uriPath{(test_path() / "meshes" / "InterpolationTest" / "InterpolationTest.gltf").string()};
	commandInterface.set({animChannel, &raco::user_types::AnimationChannel::uri_}, uriPath);
	commandInterface.set({mesh, &raco::user_types::Mesh::uri_}, uriPath);

	commandInterface.set({anim, {"animationChannels", "Channel 0"}}, animChannel);
	commandInterface.set({mesh, &raco::user_types::Mesh::bakeMeshes_}, false);
	commandInterface.set({meshNode, &raco::user_types::MeshNode::mesh_}, mesh);

	commandInterface.addLink({anim, {"outputs", "Ch0.Animation Sampler Name"}}, {meshNode, {"translation"}});

	commandInterface.set({anim, {"animationChannels", "Channel 0"}}, SEditorObject());

	ASSERT_EQ(commandInterface.project()->links().size(), 1);
	ASSERT_FALSE(commandInterface.project()->links().front()->isValid());

	commandInterface.set({anim, {"animationChannels", "Channel 0"}}, animChannel);

	ASSERT_EQ(commandInterface.project()->links().size(), 1);
	ASSERT_TRUE(commandInterface.project()->links().front()->isValid());
}

TEST_F(AnimationTest, anim_in_prefab_prefabinstance_link_inside_prefabinstance) {
	auto anim = context.createObject(Animation::typeDescription.typeName, "Animation Name");
	auto animChannel = context.createObject(AnimationChannel::typeDescription.typeName, "Animation Sampler Name");
	auto mesh = context.createObject(Mesh::typeDescription.typeName, "Mesh");
	auto meshNode = context.createObject(MeshNode::typeDescription.typeName, "MeshNode");
	auto prefab = context.createObject(Prefab::typeDescription.typeName, "Prefab");
	auto prefabInstance = context.createObject(PrefabInstance::typeDescription.typeName, "PrefabInstance");

	std::string uriPath{(test_path() / "meshes" / "InterpolationTest" / "InterpolationTest.gltf").string()};
	commandInterface.set({animChannel, &raco::user_types::AnimationChannel::uri_}, uriPath);
	commandInterface.set({mesh, &raco::user_types::Mesh::uri_}, uriPath);

	commandInterface.moveScenegraphChildren({anim}, prefab);
	commandInterface.moveScenegraphChildren({meshNode}, prefab);
	commandInterface.set({anim, {"animationChannels", "Channel 1"}}, animChannel);
	commandInterface.set({mesh, &raco::user_types::Mesh::bakeMeshes_}, false);
	commandInterface.set({meshNode, &raco::user_types::MeshNode::mesh_}, mesh);

	commandInterface.addLink({anim, {"outputs", "Ch1.Animation Sampler Name"}}, {meshNode, {"translation"}});

	ASSERT_EQ(commandInterface.project()->links().size(), 1);
	ASSERT_TRUE(commandInterface.project()->links().front()->isValid());

	commandInterface.set({prefabInstance, {"template"}}, prefab);

	ASSERT_EQ(commandInterface.project()->links().size(), 2);
	ASSERT_TRUE(commandInterface.project()->links().front()->isValid());
	ASSERT_TRUE(commandInterface.project()->links().back()->isValid());

	commandInterface.set({prefabInstance, {"template"}}, SEditorObject());

	ASSERT_EQ(commandInterface.project()->links().size(), 1);
	ASSERT_TRUE(commandInterface.project()->links().front()->isValid());
}
