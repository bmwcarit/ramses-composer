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
#include "ramses_adaptor/AnimationAdaptor.h"
#include "ramses_adaptor/utilities.h"
#include "user_types/AnimationChannel.h"
#include "user_types/Prefab.h"
#include "user_types/PrefabInstance.h"

using namespace raco::user_types;

class AnimationAdaptorTest : public RamsesBaseFixture<> {};

class AnimationAdaptorTest_FL3 : public RamsesBaseFixture<> {
public:
	AnimationAdaptorTest_FL3() : RamsesBaseFixture(false, static_cast<rlogic::EFeatureLevel>(3)) {}
};


TEST_F(AnimationAdaptorTest, animNode_Creation) {
	auto anim = context.createObject(Animation::typeDescription.typeName, "Animation Name");
	auto animChannel = context.createObject(AnimationChannel::typeDescription.typeName, "Animation Sampler Name");

	dispatch();

	std::string uriPath{(test_path() / "meshes" / "InterpolationTest" / "InterpolationTest.gltf").string()};
	commandInterface.set({animChannel, &raco::user_types::AnimationChannel::uri_}, uriPath);
	dispatch();

	context.set({animChannel, &raco::user_types::AnimationChannel::animationIndex_}, 4);
	context.set({anim, {"animationChannels", "Channel 0"}}, animChannel);
	dispatch();

	ASSERT_EQ(sceneContext.logicEngine().getCollection<rlogic::AnimationNode>().size(), 1);
	ASSERT_NE(select<rlogic::AnimationNode>(sceneContext.logicEngine(), "Animation Name"), nullptr);
}

TEST_F(AnimationAdaptorTest, animNode_Deletion) {
	auto anim = context.createObject(Animation::typeDescription.typeName, "Animation Name");
	auto animChannel = context.createObject(AnimationChannel::typeDescription.typeName, "Animation Sampler Name");

	dispatch();

	std::string uriPath{(test_path() / "meshes" / "InterpolationTest" / "InterpolationTest.gltf").string()};
	commandInterface.set({animChannel, &raco::user_types::AnimationChannel::uri_}, uriPath);
	dispatch();

	context.set({animChannel, &raco::user_types::AnimationChannel::animationIndex_}, 4);
	context.set({anim, {"animationChannels", "Channel 0"}}, animChannel);
	dispatch();

	ASSERT_EQ(sceneContext.logicEngine().getCollection<rlogic::AnimationNode>().size(), 1);

	context.set({anim, {"animationChannels", "Channel 0"}}, SEditorObject{});
	dispatch();

	ASSERT_EQ(sceneContext.logicEngine().getCollection<rlogic::AnimationNode>().size(), 0);
}

TEST_F(AnimationAdaptorTest, animNode_animName) {
	auto anim = context.createObject(Animation::typeDescription.typeName, "Animation Name");
	auto animChannel = context.createObject(AnimationChannel::typeDescription.typeName, "Animation Sampler Name");

	dispatch();

	std::string uriPath{(test_path() / "meshes" / "InterpolationTest" / "InterpolationTest.gltf").string()};
	commandInterface.set({animChannel, &raco::user_types::AnimationChannel::uri_}, uriPath);
	dispatch();

	context.set({animChannel, &raco::user_types::AnimationChannel::animationIndex_}, 4);
	context.set({anim, {"animationChannels", "Channel 0"}}, animChannel);
	dispatch();

	context.set({anim, {"objectName"}}, "Changed");
	dispatch();

	ASSERT_EQ(sceneContext.logicEngine().getCollection<rlogic::AnimationNode>().size(), 1);
	ASSERT_EQ(sceneContext.logicEngine().getCollection<rlogic::AnimationNode>().begin()->getUserId(), anim->objectIDAsRamsesLogicID());
	ASSERT_NE(select<rlogic::AnimationNode>(sceneContext.logicEngine(), "Changed"), nullptr);
}

TEST_F(AnimationAdaptorTest, prefab_noAnimNode) {
	auto anim = context.createObject(Animation::typeDescription.typeName, "Animation Name");
	auto animChannel = context.createObject(AnimationChannel::typeDescription.typeName, "Animation Sampler Name");

	dispatch();

	std::string uriPath{(test_path() / "meshes" / "InterpolationTest" / "InterpolationTest.gltf").string()};
	commandInterface.set({animChannel, &raco::user_types::AnimationChannel::uri_}, uriPath);
	dispatch();

	context.set({animChannel, &raco::user_types::AnimationChannel::animationIndex_}, 4);
	context.set({anim, {"animationChannels", "Channel 0"}}, animChannel);
	dispatch();

	context.set({anim, {"objectName"}}, "Changed");
	dispatch();

	auto prefab = create<Prefab>("Prefab");
	dispatch();

	commandInterface.moveScenegraphChildren({anim}, prefab);
	dispatch();

	ASSERT_EQ(sceneContext.logicEngine().getCollection<rlogic::AnimationNode>().size(), 0);
}

TEST_F(AnimationAdaptorTest, animNode_multiple_channels) {
	auto anim = context.createObject(Animation::typeDescription.typeName, "Animation Name");
	auto animChannel1 = context.createObject(AnimationChannel::typeDescription.typeName, "Animation Sampler Name 1");
	auto animChannel2 = context.createObject(AnimationChannel::typeDescription.typeName, "Animation Sampler Name 2");
	auto animChannel3 = context.createObject(AnimationChannel::typeDescription.typeName, "Animation Sampler Name 3");

	dispatch();

	std::string uriPath{(test_path() / "meshes" / "InterpolationTest" / "InterpolationTest.gltf").string()};
	commandInterface.set({animChannel1, &raco::user_types::AnimationChannel::uri_}, uriPath);
	commandInterface.set({animChannel2, &raco::user_types::AnimationChannel::uri_}, uriPath);
	commandInterface.set({animChannel3, &raco::user_types::AnimationChannel::uri_}, uriPath);
	dispatch();

	context.set({animChannel1, &raco::user_types::AnimationChannel::animationIndex_}, 4);
	context.set({animChannel2, &raco::user_types::AnimationChannel::animationIndex_}, 2);
	context.set({animChannel3, &raco::user_types::AnimationChannel::animationIndex_}, 3);

	context.set({anim, {"animationChannels", "Channel 0"}}, animChannel1);
	dispatch();

	context.set({anim, {"animationChannels", "Channel 1"}}, animChannel2);
	dispatch();

	context.set({anim, {"animationChannels", "Channel 2"}}, animChannel3);
	dispatch();

	ASSERT_EQ(anim->get("outputs")->asTable().size(), 3);
	ASSERT_NE(select<rlogic::DataArray>(sceneContext.logicEngine(), "Animation Sampler Name 1.keyframes"), nullptr);
	ASSERT_EQ(select<rlogic::DataArray>(sceneContext.logicEngine(), "Animation Sampler Name 1.keyframes")->getUserId(), animChannel1->objectIDAsRamsesLogicID());
	ASSERT_NE(select<rlogic::DataArray>(sceneContext.logicEngine(), "Animation Sampler Name 1.timestamps"), nullptr);
	ASSERT_EQ(select<rlogic::DataArray>(sceneContext.logicEngine(), "Animation Sampler Name 1.timestamps")->getUserId(), animChannel1->objectIDAsRamsesLogicID());

	ASSERT_NE(select<rlogic::DataArray>(sceneContext.logicEngine(), "Animation Sampler Name 2.keyframes"), nullptr);
	ASSERT_EQ(select<rlogic::DataArray>(sceneContext.logicEngine(), "Animation Sampler Name 2.keyframes")->getUserId(), animChannel2->objectIDAsRamsesLogicID());
	ASSERT_NE(select<rlogic::DataArray>(sceneContext.logicEngine(), "Animation Sampler Name 2.timestamps"), nullptr);
	ASSERT_EQ(select<rlogic::DataArray>(sceneContext.logicEngine(), "Animation Sampler Name 2.timestamps")->getUserId(), animChannel2->objectIDAsRamsesLogicID());

	ASSERT_NE(select<rlogic::DataArray>(sceneContext.logicEngine(), "Animation Sampler Name 3.keyframes"), nullptr);
	ASSERT_EQ(select<rlogic::DataArray>(sceneContext.logicEngine(), "Animation Sampler Name 3.keyframes")->getUserId(), animChannel3->objectIDAsRamsesLogicID());
	ASSERT_NE(select<rlogic::DataArray>(sceneContext.logicEngine(), "Animation Sampler Name 3.timestamps"), nullptr);
	ASSERT_EQ(select<rlogic::DataArray>(sceneContext.logicEngine(), "Animation Sampler Name 3.timestamps")->getUserId(), animChannel3->objectIDAsRamsesLogicID());
}

TEST_F(AnimationAdaptorTest, afterSync_dataArrays_get_cleaned_up) {
	auto anim = context.createObject(Animation::typeDescription.typeName, "Animation Name");
	auto animChannel = context.createObject(AnimationChannel::typeDescription.typeName, "Animation Sampler Name");

	dispatch();

	std::string uriPath{(test_path() / "meshes" / "InterpolationTest" / "InterpolationTest.gltf").string()};
	commandInterface.set({animChannel, &raco::user_types::AnimationChannel::uri_}, uriPath);
	dispatch();

	context.set({animChannel, &raco::user_types::AnimationChannel::animationIndex_}, 4);
	context.set({anim, {"animationChannels", "Channel 0"}}, animChannel);
	dispatch();

	ASSERT_EQ(sceneContext.logicEngine().getCollection<rlogic::DataArray>().size(), 4);
	ASSERT_NE(select<rlogic::DataArray>(sceneContext.logicEngine(), "Animation Sampler Name.keyframes"), nullptr);
	ASSERT_NE(select<rlogic::DataArray>(sceneContext.logicEngine(), "Animation Sampler Name.timestamps"), nullptr);
	ASSERT_NE(select<rlogic::DataArray>(sceneContext.logicEngine(), "Animation Sampler Name.tangentIn"), nullptr);
	ASSERT_NE(select<rlogic::DataArray>(sceneContext.logicEngine(), "Animation Sampler Name.tangentOut"), nullptr);

	// moving from cubic to non-cubic anim: tangent data arrays should be deallocated
	context.set({animChannel, &raco::user_types::AnimationChannel::animationIndex_}, 3);

	dispatch();

	ASSERT_EQ(sceneContext.logicEngine().getCollection<rlogic::DataArray>().size(), 2);
	ASSERT_NE(select<rlogic::DataArray>(sceneContext.logicEngine(), "Animation Sampler Name.keyframes"), nullptr);
	ASSERT_NE(select<rlogic::DataArray>(sceneContext.logicEngine(), "Animation Sampler Name.timestamps"), nullptr);
	ASSERT_EQ(select<rlogic::DataArray>(sceneContext.logicEngine(), "Animation Sampler Name.tangentIn"), nullptr);
	ASSERT_EQ(select<rlogic::DataArray>(sceneContext.logicEngine(), "Animation Sampler Name.tangentOut"), nullptr);
}

TEST_F(AnimationAdaptorTest, link_animation_to_node) {
	auto anim = context.createObject(Animation::typeDescription.typeName, "Animation Name");
	auto animChannel = context.createObject(AnimationChannel::typeDescription.typeName, "Animation Sampler Name");
	auto node = context.createObject(Node::typeDescription.typeName, "Node");
	dispatch();

	std::string uriPath{(test_path() / "meshes" / "InterpolationTest" / "InterpolationTest.gltf").string()};
	commandInterface.set({animChannel, &raco::user_types::AnimationChannel::uri_}, uriPath);
	dispatch();

	commandInterface.set({anim, {"animationChannels", "Channel 0"}}, animChannel);
	dispatch();

	auto ramsesNode = select<ramses::Node>(*sceneContext.scene(), "Node");
	float x, y, z;

	ramsesNode->getTranslation(x, y, z);
	EXPECT_EQ(x, 0.0);

	commandInterface.addLink({anim, {"outputs", "Ch0.Animation Sampler Name"}}, {node, {"translation"}});
	dispatch();

	ramsesNode->getTranslation(x, y, z);
	EXPECT_EQ(x, 1.0);
}

TEST_F(AnimationAdaptorTest, component_type_array_valid) {
	auto anim = context.createObject(Animation::typeDescription.typeName, "Animation Name");
	auto animChannel = context.createObject(AnimationChannel::typeDescription.typeName, "Animation Sampler Name");

	std::string uriPath{(test_path() / "meshes" / "AnimatedMorphCube" / "AnimatedMorphCube.gltf").string()};
	commandInterface.set({animChannel, &raco::user_types::AnimationChannel::uri_}, uriPath);
	dispatch();

	context.set({anim, {"animationChannels", "Channel 0"}}, animChannel);
	dispatch();

	ASSERT_EQ(sceneContext.logicEngine().getCollection<rlogic::AnimationNode>().size(), 1);
	ASSERT_NE(select<rlogic::AnimationNode>(sceneContext.logicEngine(), "Animation Name"), nullptr);
}

TEST_F(AnimationAdaptorTest_FL3, component_type_array_invalid_fl3) {
	auto anim = context.createObject(Animation::typeDescription.typeName, "Animation Name");
	auto animChannel = context.createObject(AnimationChannel::typeDescription.typeName, "Animation Sampler Name");

	std::string uriPath{(test_path() / "meshes" / "AnimatedMorphCube" / "AnimatedMorphCube.gltf").string()};
	commandInterface.set({animChannel, &raco::user_types::AnimationChannel::uri_}, uriPath);
	dispatch();

	context.set({anim, {"animationChannels", "Channel 0"}}, animChannel);
	dispatch();

	ASSERT_EQ(sceneContext.logicEngine().getCollection<rlogic::AnimationNode>().size(), 0);
}
