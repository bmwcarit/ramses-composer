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
#include "user_types/AnimationChannelRaco.h"
#include "user_types/Prefab.h"
#include "user_types/PrefabInstance.h"

using namespace raco::user_types;

class AnimationAdaptorTest : public RamsesBaseFixture<> {
public:
	template <typename T, typename U>
	void check_interpolation(EnginePrimitive componentType, ramses::EPropertyType ramsesComponentType, MeshAnimationInterpolation interpolation,
		std::vector<float> timeStamps,
		std::vector<T> keyFrames, std::vector<T> tangentsIn, std::vector<T> tangentsOut,
		float refTime, U refValue) {
		auto channel = create<AnimationChannelRaco>("channel");

		commandInterface.set({channel, &AnimationChannelRaco::componentType_}, static_cast<int>(componentType));
		commandInterface.set({channel, &AnimationChannelRaco::interpolationType_}, static_cast<int>(interpolation));

		commandInterface.setAnimationData(channel, timeStamps, keyFrames, tangentsIn, tangentsOut);

		auto animation = create<Animation>("animation");
		commandInterface.set(ValueHandle{animation, &Animation::animationChannels_}[0], channel);

		dispatch();

		auto ramsesAnimation = selectCheckLogic<ramses::AnimationNode>(animation);

		ASSERT_TRUE(ramsesAnimation->getOutputs()->hasChild("Ch0.channel"));
		auto ramsesProp = ramsesAnimation->getOutputs()->getChild("Ch0.channel");
		ASSERT_EQ(ramsesProp->getType(), ramsesComponentType);
		ASSERT_EQ(ramsesProp->get<U>().value(), U());

		commandInterface.set({animation, &Animation::progress_}, refTime);
		dispatch();

		ASSERT_EQ(ramsesProp->get<U>().value(), refValue);
	}
};

TEST_F(AnimationAdaptorTest, animNode_Creation) {
	auto anim = context.createObject(Animation::typeDescription.typeName, "Animation Name");
	auto animChannel = context.createObject(AnimationChannel::typeDescription.typeName, "Animation Sampler Name");

	dispatch();

	std::string uriPath{(test_path() / "meshes" / "InterpolationTest" / "InterpolationTest.gltf").string()};
	commandInterface.set({animChannel, &user_types::AnimationChannel::uri_}, uriPath);
	dispatch();

	context.set({animChannel, &user_types::AnimationChannel::animationIndex_}, 4);
	context.set(ValueHandle(anim, &Animation::animationChannels_)[0], animChannel);
	dispatch();

	ASSERT_EQ(sceneContext.logicEngine().getCollection<ramses::AnimationNode>().size(), 1);
	ASSERT_NE(select<ramses::AnimationNode>(sceneContext.logicEngine(), "Animation Name"), nullptr);
}

TEST_F(AnimationAdaptorTest, animNode_Deletion) {
	auto anim = context.createObject(Animation::typeDescription.typeName, "Animation Name");
	auto animChannel = context.createObject(AnimationChannel::typeDescription.typeName, "Animation Sampler Name");

	dispatch();

	std::string uriPath{(test_path() / "meshes" / "InterpolationTest" / "InterpolationTest.gltf").string()};
	commandInterface.set({animChannel, &user_types::AnimationChannel::uri_}, uriPath);
	dispatch();

	context.set({animChannel, &user_types::AnimationChannel::animationIndex_}, 4);
	context.set(ValueHandle(anim, &Animation::animationChannels_)[0], animChannel);
	dispatch();

	ASSERT_EQ(sceneContext.logicEngine().getCollection<ramses::AnimationNode>().size(), 1);

	context.set(ValueHandle(anim, &Animation::animationChannels_)[0], SEditorObject());
	dispatch();

	ASSERT_EQ(sceneContext.logicEngine().getCollection<ramses::AnimationNode>().size(), 0);
}

TEST_F(AnimationAdaptorTest, animNode_animName) {
	auto anim = context.createObject(Animation::typeDescription.typeName, "Animation Name");
	auto animChannel = context.createObject(AnimationChannel::typeDescription.typeName, "Animation Sampler Name");

	dispatch();

	std::string uriPath{(test_path() / "meshes" / "InterpolationTest" / "InterpolationTest.gltf").string()};
	commandInterface.set({animChannel, &user_types::AnimationChannel::uri_}, uriPath);
	dispatch();

	context.set({animChannel, &user_types::AnimationChannel::animationIndex_}, 4);
	context.set(ValueHandle(anim, &Animation::animationChannels_)[0], animChannel);
	dispatch();

	context.set({anim, {"objectName"}}, "Changed");
	dispatch();

	ASSERT_EQ(sceneContext.logicEngine().getCollection<ramses::AnimationNode>().size(), 1);
	ASSERT_EQ(sceneContext.logicEngine().getCollection<ramses::AnimationNode>().begin()->getUserId(), anim->objectIDAsRamsesLogicID());
	ASSERT_NE(select<ramses::AnimationNode>(sceneContext.logicEngine(), "Changed"), nullptr);
}

TEST_F(AnimationAdaptorTest, prefab_noAnimNode) {
	auto anim = context.createObject(Animation::typeDescription.typeName, "Animation Name");
	auto animChannel = context.createObject(AnimationChannel::typeDescription.typeName, "Animation Sampler Name");

	dispatch();

	std::string uriPath{(test_path() / "meshes" / "InterpolationTest" / "InterpolationTest.gltf").string()};
	commandInterface.set({animChannel, &user_types::AnimationChannel::uri_}, uriPath);
	dispatch();

	context.set({animChannel, &user_types::AnimationChannel::animationIndex_}, 4);
	context.set(ValueHandle(anim, &Animation::animationChannels_)[0], animChannel);
	dispatch();

	context.set({anim, {"objectName"}}, "Changed");
	dispatch();

	auto prefab = create<Prefab>("Prefab");
	dispatch();

	commandInterface.moveScenegraphChildren({anim}, prefab);
	dispatch();

	ASSERT_EQ(sceneContext.logicEngine().getCollection<ramses::AnimationNode>().size(), 0);
}

TEST_F(AnimationAdaptorTest, animNode_multiple_channels) {
	auto anim = context.createObject(Animation::typeDescription.typeName, "Animation Name");
	auto animChannel1 = context.createObject(AnimationChannel::typeDescription.typeName, "Animation Sampler Name 1");
	auto animChannel2 = context.createObject(AnimationChannel::typeDescription.typeName, "Animation Sampler Name 2");
	auto animChannel3 = context.createObject(AnimationChannel::typeDescription.typeName, "Animation Sampler Name 3");
	ValueHandle channelsHandle(anim, &Animation::animationChannels_);
	context.resizeArray(channelsHandle, 3);

	dispatch();

	std::string uriPath{(test_path() / "meshes" / "InterpolationTest" / "InterpolationTest.gltf").string()};
	commandInterface.set({animChannel1, &user_types::AnimationChannel::uri_}, uriPath);
	commandInterface.set({animChannel2, &user_types::AnimationChannel::uri_}, uriPath);
	commandInterface.set({animChannel3, &user_types::AnimationChannel::uri_}, uriPath);
	dispatch();

	context.set({animChannel1, &user_types::AnimationChannel::animationIndex_}, 4);
	context.set({animChannel2, &user_types::AnimationChannel::animationIndex_}, 2);
	context.set({animChannel3, &user_types::AnimationChannel::animationIndex_}, 3);

	context.set(channelsHandle[0], animChannel1);
	dispatch();

	context.set(channelsHandle[1], animChannel2);
	dispatch();

	context.set(channelsHandle[2], animChannel3);
	dispatch();

	ASSERT_EQ(anim->get("outputs")->asTable().size(), 3);
	ASSERT_NE(select<ramses::DataArray>(sceneContext.logicEngine(), "Animation Sampler Name 1.keyframes"), nullptr);
	ASSERT_EQ(select<ramses::DataArray>(sceneContext.logicEngine(), "Animation Sampler Name 1.keyframes")->getUserId(), animChannel1->objectIDAsRamsesLogicID());
	ASSERT_NE(select<ramses::DataArray>(sceneContext.logicEngine(), "Animation Sampler Name 1.timestamps"), nullptr);
	ASSERT_EQ(select<ramses::DataArray>(sceneContext.logicEngine(), "Animation Sampler Name 1.timestamps")->getUserId(), animChannel1->objectIDAsRamsesLogicID());

	ASSERT_NE(select<ramses::DataArray>(sceneContext.logicEngine(), "Animation Sampler Name 2.keyframes"), nullptr);
	ASSERT_EQ(select<ramses::DataArray>(sceneContext.logicEngine(), "Animation Sampler Name 2.keyframes")->getUserId(), animChannel2->objectIDAsRamsesLogicID());
	ASSERT_NE(select<ramses::DataArray>(sceneContext.logicEngine(), "Animation Sampler Name 2.timestamps"), nullptr);
	ASSERT_EQ(select<ramses::DataArray>(sceneContext.logicEngine(), "Animation Sampler Name 2.timestamps")->getUserId(), animChannel2->objectIDAsRamsesLogicID());

	ASSERT_NE(select<ramses::DataArray>(sceneContext.logicEngine(), "Animation Sampler Name 3.keyframes"), nullptr);
	ASSERT_EQ(select<ramses::DataArray>(sceneContext.logicEngine(), "Animation Sampler Name 3.keyframes")->getUserId(), animChannel3->objectIDAsRamsesLogicID());
	ASSERT_NE(select<ramses::DataArray>(sceneContext.logicEngine(), "Animation Sampler Name 3.timestamps"), nullptr);
	ASSERT_EQ(select<ramses::DataArray>(sceneContext.logicEngine(), "Animation Sampler Name 3.timestamps")->getUserId(), animChannel3->objectIDAsRamsesLogicID());
}

TEST_F(AnimationAdaptorTest, afterSync_dataArrays_get_cleaned_up) {
	auto anim = context.createObject(Animation::typeDescription.typeName, "Animation Name");
	auto animChannel = context.createObject(AnimationChannel::typeDescription.typeName, "Animation Sampler Name");

	dispatch();

	std::string uriPath{(test_path() / "meshes" / "InterpolationTest" / "InterpolationTest.gltf").string()};
	commandInterface.set({animChannel, &user_types::AnimationChannel::uri_}, uriPath);
	dispatch();

	context.set({animChannel, &user_types::AnimationChannel::animationIndex_}, 4);
	context.set(ValueHandle(anim, &Animation::animationChannels_)[0], animChannel);
	dispatch();

	ASSERT_EQ(sceneContext.logicEngine().getCollection<ramses::DataArray>().size(), 4);
	ASSERT_NE(select<ramses::DataArray>(sceneContext.logicEngine(), "Animation Sampler Name.keyframes"), nullptr);
	ASSERT_NE(select<ramses::DataArray>(sceneContext.logicEngine(), "Animation Sampler Name.timestamps"), nullptr);
	ASSERT_NE(select<ramses::DataArray>(sceneContext.logicEngine(), "Animation Sampler Name.tangentIn"), nullptr);
	ASSERT_NE(select<ramses::DataArray>(sceneContext.logicEngine(), "Animation Sampler Name.tangentOut"), nullptr);

	// moving from cubic to non-cubic anim: tangent data arrays should be deallocated
	context.set({animChannel, &user_types::AnimationChannel::animationIndex_}, 3);

	dispatch();

	ASSERT_EQ(sceneContext.logicEngine().getCollection<ramses::DataArray>().size(), 2);
	ASSERT_NE(select<ramses::DataArray>(sceneContext.logicEngine(), "Animation Sampler Name.keyframes"), nullptr);
	ASSERT_NE(select<ramses::DataArray>(sceneContext.logicEngine(), "Animation Sampler Name.timestamps"), nullptr);
	ASSERT_EQ(select<ramses::DataArray>(sceneContext.logicEngine(), "Animation Sampler Name.tangentIn"), nullptr);
	ASSERT_EQ(select<ramses::DataArray>(sceneContext.logicEngine(), "Animation Sampler Name.tangentOut"), nullptr);
}

TEST_F(AnimationAdaptorTest, link_animation_to_node) {
	auto anim = context.createObject(Animation::typeDescription.typeName, "Animation Name");
	auto animChannel = context.createObject(AnimationChannel::typeDescription.typeName, "Animation Sampler Name");
	auto node = context.createObject(Node::typeDescription.typeName, "Node");
	dispatch();

	std::string uriPath{(test_path() / "meshes" / "InterpolationTest" / "InterpolationTest.gltf").string()};
	commandInterface.set({animChannel, &user_types::AnimationChannel::uri_}, uriPath);
	dispatch();

	context.set(ValueHandle(anim, &Animation::animationChannels_)[0], animChannel);
	dispatch();

	auto ramsesNode = select<ramses::Node>(*sceneContext.scene(), "Node");
	glm::vec3 v;
	ramsesNode->getTranslation(v);
	EXPECT_EQ(v.x, 0.0);

	commandInterface.addLink({anim, {"outputs", "Ch0.Animation Sampler Name"}}, {node, {"translation"}});
	dispatch();

	ramsesNode->getTranslation(v);
	EXPECT_EQ(v.x, 1.0);
}

TEST_F(AnimationAdaptorTest, component_type_array_valid) {
	auto anim = context.createObject(Animation::typeDescription.typeName, "Animation Name");
	auto animChannel = context.createObject(AnimationChannel::typeDescription.typeName, "Animation Sampler Name");

	std::string uriPath{(test_path() / "meshes" / "AnimatedMorphCube" / "AnimatedMorphCube.gltf").string()};
	commandInterface.set({animChannel, &user_types::AnimationChannel::uri_}, uriPath);
	dispatch();

	context.set(ValueHandle(anim, &Animation::animationChannels_)[0], animChannel);
	dispatch();

	ASSERT_EQ(sceneContext.logicEngine().getCollection<ramses::AnimationNode>().size(), 1);
	ASSERT_NE(select<ramses::AnimationNode>(sceneContext.logicEngine(), "Animation Name"), nullptr);
}

TEST_F(AnimationAdaptorTest, raco_channel_interpolation_float_linear) {
	check_interpolation<float, float>(EnginePrimitive::Double, ramses::EPropertyType::Float, MeshAnimationInterpolation::Linear,
		{0.0, 1.0}, {0.0, 4.0}, {}, {}, 0.5, 2.0);
}

TEST_F(AnimationAdaptorTest, raco_channel_interpolation_float_step) {
	check_interpolation<float, float>(EnginePrimitive::Double, ramses::EPropertyType::Float, MeshAnimationInterpolation::Step,
		{0.0, 1.0}, {0.0, 4.0}, {}, {}, 0.5, 0.0);
}

TEST_F(AnimationAdaptorTest, raco_channel_interpolation_float_cubic) {
	check_interpolation<float, float>(EnginePrimitive::Double, ramses::EPropertyType::Float, MeshAnimationInterpolation::CubicSpline,
		{0.0, 1.0}, {0.0, 4.0}, {0.0, 0.0}, {0.0, 0.0}, 0.5, 2.0);
}

TEST_F(AnimationAdaptorTest, raco_channel_interpolation_float_cubic_2) {
	check_interpolation<float, float>(EnginePrimitive::Double, ramses::EPropertyType::Float, MeshAnimationInterpolation::CubicSpline,
		{0.0, 1.0}, {0.0, 4.0}, {1.0, 1.0}, {1.0, 1.0}, 0.5, 2.0);
}

TEST_F(AnimationAdaptorTest, raco_channel_interpolation_vec2f_linear) {
	check_interpolation<std::vector<float>, glm::vec2>(EnginePrimitive::Vec2f, ramses::EPropertyType::Vec2f, MeshAnimationInterpolation::Linear,
		{0.0, 1.0}, {{0.0, 0.0}, {4.0, 6.0}}, {}, {}, 0.5, {2.0, 3.0});
}

TEST_F(AnimationAdaptorTest, raco_channel_interpolation_vec2f_step) {
	check_interpolation<std::vector<float>, glm::vec2>(EnginePrimitive::Vec2f, ramses::EPropertyType::Vec2f, MeshAnimationInterpolation::Step,
		{0.0, 1.0}, {{0.0, 0.0}, {4.0, 6.0}}, {}, {}, 0.5, {0.0, 0.0});
}

TEST_F(AnimationAdaptorTest, raco_channel_interpolation_vec2f_cubic) {
	check_interpolation<std::vector<float>, glm::vec2>(EnginePrimitive::Vec2f, ramses::EPropertyType::Vec2f, MeshAnimationInterpolation::CubicSpline,
		{0.0, 1.0}, {{0.0, 0.0}, {4.0, 6.0}}, {{0.0, 0.0}, {0.0, 0.0}}, {{0.0, 0.0}, {0.0, 0.0}}, 0.5, {2.0, 3.0});
}

TEST_F(AnimationAdaptorTest, raco_channel_interpolation_vec3f_linear) {
	check_interpolation<std::vector<float>, glm::vec3>(EnginePrimitive::Vec3f, ramses::EPropertyType::Vec3f, MeshAnimationInterpolation::Linear,
		{0.0, 1.0}, {{0.0, 0.0, 0.0}, {4.0, 6.0, 8.0}}, {}, {}, 0.5, {2.0, 3.0, 4.0});
}

TEST_F(AnimationAdaptorTest, raco_channel_interpolation_vec3f_step) {
	check_interpolation<std::vector<float>, glm::vec3>(EnginePrimitive::Vec3f, ramses::EPropertyType::Vec3f, MeshAnimationInterpolation::Step,
		{0.0, 1.0}, {{0.0, 0.0, 0.0}, {4.0, 6.0, 8.0}}, {}, {}, 0.5, {0.0, 0.0, 0.0});
}

TEST_F(AnimationAdaptorTest, raco_channel_interpolation_vec3f_cubic) {
	check_interpolation<std::vector<float>, glm::vec3>(EnginePrimitive::Vec3f, ramses::EPropertyType::Vec3f, MeshAnimationInterpolation::CubicSpline,
		{0.0, 1.0}, {{0.0, 0.0, 0.0}, {4.0, 6.0, 8.0}}, {{0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}}, {{0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}}, 0.5, {2.0, 3.0, 4.0});
}

TEST_F(AnimationAdaptorTest, raco_channel_interpolation_vec4f_linear) {
	check_interpolation<std::vector<float>, glm::vec4>(EnginePrimitive::Vec4f, ramses::EPropertyType::Vec4f, MeshAnimationInterpolation::Linear,
		{0.0, 1.0}, {{0.0, 0.0, 0.0, 0.0}, {4.0, 6.0, 8.0, 10.0}}, {}, {}, 0.5, {2.0, 3.0, 4.0, 5.0});
}

TEST_F(AnimationAdaptorTest, raco_channel_interpolation_vec4f_step) {
	check_interpolation<std::vector<float>, glm::vec4>(EnginePrimitive::Vec4f, ramses::EPropertyType::Vec4f, MeshAnimationInterpolation::Step,
		{0.0, 1.0}, {{0.0, 0.0, 0.0, 0.0}, {4.0, 6.0, 8.0, 10.0}}, {}, {}, 0.5, {0.0, 0.0, 0.0, 0.0});
}

TEST_F(AnimationAdaptorTest, raco_channel_interpolation_vec4f_cubic) {
	check_interpolation<std::vector<float>, glm::vec4>(EnginePrimitive::Vec4f, ramses::EPropertyType::Vec4f, MeshAnimationInterpolation::CubicSpline,
		{0.0, 1.0}, {{0.0, 0.0, 0.0, 0.0}, {4.0, 6.0, 8.0, 10.0}}, {{0.0, 0.0, 0.0, 0.0}, {0.0, 0.0, 0.0, 0.0}}, {{0.0, 0.0, 0.0, 0.0}, {0.0, 0.0, 0.0, 0.0}}, 0.5, {2.0, 3.0, 4.0, 5.0});
}


TEST_F(AnimationAdaptorTest, raco_channel_interpolation_int_linear) {
	check_interpolation<int, int32_t>(EnginePrimitive::Int32, ramses::EPropertyType::Int32, MeshAnimationInterpolation::Linear,
		{0.0, 1.0}, {0, 5}, {}, {}, 0.5, 3);
}

TEST_F(AnimationAdaptorTest, raco_channel_interpolation_int_step) {
	check_interpolation<int, int32_t>(EnginePrimitive::Int32, ramses::EPropertyType::Int32, MeshAnimationInterpolation::Step,
		{0.0, 1.0}, {0, 5}, {}, {}, 0.5, 0);
}

TEST_F(AnimationAdaptorTest, raco_channel_interpolation_int_cubic) {
	check_interpolation<int, int32_t>(EnginePrimitive::Int32, ramses::EPropertyType::Int32, MeshAnimationInterpolation::CubicSpline,
		{0.0, 1.0}, {0, 5}, {0, 0}, {0, 0}, 0.5, 3);
}


TEST_F(AnimationAdaptorTest, raco_channel_interpolation_vec2i_linear) {
	check_interpolation<std::vector<int>, glm::ivec2>(EnginePrimitive::Vec2i, ramses::EPropertyType::Vec2i, MeshAnimationInterpolation::Linear,
		{0.0, 1.0}, {{0, 0}, {5, 7}}, {}, {}, 0.5, {3, 4});
}

TEST_F(AnimationAdaptorTest, raco_channel_interpolation_vec2i_step) {
	check_interpolation<std::vector<int>, glm::ivec2>(EnginePrimitive::Vec2i, ramses::EPropertyType::Vec2i, MeshAnimationInterpolation::Step,
		{0.0, 1.0}, {{0, 0}, {5, 7}}, {}, {}, 0.5, {0, 0});
}

TEST_F(AnimationAdaptorTest, raco_channel_interpolation_vec2i_cubic) {
	check_interpolation<std::vector<int>, glm::ivec2>(EnginePrimitive::Vec2i, ramses::EPropertyType::Vec2i, MeshAnimationInterpolation::CubicSpline,
		{0.0, 1.0}, {{0, 0}, {5, 7}}, {std::vector<int>(2, 0), std::vector<int>(2, 0)}, {std::vector<int>(2, 0), std::vector<int>(2, 0)}, 0.5, {3, 4});
}


TEST_F(AnimationAdaptorTest, raco_channel_interpolation_vec3i_linear) {
	check_interpolation<std::vector<int>, glm::ivec3>(EnginePrimitive::Vec3i, ramses::EPropertyType::Vec3i, MeshAnimationInterpolation::Linear,
		{0.0, 1.0}, {{0, 0, 0}, {5, 7, 9}}, {}, {}, 0.5, {3, 4, 5});
}

TEST_F(AnimationAdaptorTest, raco_channel_interpolation_vec3i_step) {
	check_interpolation<std::vector<int>, glm::ivec3>(EnginePrimitive::Vec3i, ramses::EPropertyType::Vec3i, MeshAnimationInterpolation::Step,
		{0.0, 1.0}, {{0, 0, 0}, {5, 7, 9}}, {}, {}, 0.5, {0, 0, 0});
}

TEST_F(AnimationAdaptorTest, raco_channel_interpolation_vec3i_cubic) {
	check_interpolation<std::vector<int>, glm::ivec3>(EnginePrimitive::Vec3i, ramses::EPropertyType::Vec3i, MeshAnimationInterpolation::CubicSpline,
		{0.0, 1.0}, {{0, 0, 0}, {5, 7, 9}}, {std::vector<int>(3, 0), std::vector<int>(3, 0)}, {std::vector<int>(3, 0), std::vector<int>(3, 0)}, 0.5, {3, 4, 5});
}


TEST_F(AnimationAdaptorTest, raco_channel_interpolation_vec4i_linear) {
	check_interpolation<std::vector<int>, glm::ivec4>(EnginePrimitive::Vec4i, ramses::EPropertyType::Vec4i, MeshAnimationInterpolation::Linear,
		{0.0, 1.0}, {{0, 0, 0, 0}, {5, 7, 9, 11}}, {}, {}, 0.5, {3, 4, 5, 6});
}

TEST_F(AnimationAdaptorTest, raco_channel_interpolation_vec4i_step) {
	check_interpolation<std::vector<int>, glm::ivec4>(EnginePrimitive::Vec4i, ramses::EPropertyType::Vec4i, MeshAnimationInterpolation::Step,
		{0.0, 1.0}, {{0, 0, 0, 0}, {5, 7, 9, 11}}, {}, {}, 0.5, {0, 0, 0, 0});
}

TEST_F(AnimationAdaptorTest, raco_channel_interpolation_vec4i_cubic) {
	check_interpolation<std::vector<int>, glm::ivec4>(EnginePrimitive::Vec4i, ramses::EPropertyType::Vec4i, MeshAnimationInterpolation::CubicSpline,
		{0.0, 1.0}, {{0, 0, 0, 0}, {5, 7, 9, 11}}, {std::vector<int>(4, 0), std::vector<int>(4, 0)}, {std::vector<int>(4, 0), std::vector<int>(4, 0)}, 0.5, {3, 4, 5, 6});
}
