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

#include "testing/TestUtil.h"
#include "user_types/AnimationChannelRaco.h"

#include <gtest/gtest.h>

using namespace raco;
using namespace raco::core;
using namespace raco::user_types;

template <typename ElementType, int N>
std::vector<std::vector<ElementType>> toVector(const std::vector<glm::vec<N, ElementType, glm::defaultp>>& data) {
	std::vector<std::vector<ElementType>> result;
	for (size_t index = 0; index < data.size(); index++) {
		std::vector<ElementType> v;
		for (size_t component = 0; component < N; component++) {
			v.push_back(data[index][component]);
		}
		result.push_back(v);
	}
	return result;
}

template <typename ElementType>
std::vector<ElementType> toVector(const std::vector<ElementType>& data) {
	return data;
}


class AnimationChannelRacoTest : public TestEnvironmentCore {
public:
	template <typename T, typename U>
	void check_set_get(EnginePrimitive componentType, MeshAnimationInterpolation interpolation,
		std::vector<float> timeStamps,
		std::vector<T> keyFrames, std::vector<T> tangentsIn = {}, std::vector<T> tangentsOut = {}) {
		auto channel = create<AnimationChannelRaco>("channel");

		commandInterface.set({channel, &AnimationChannelRaco::componentType_}, static_cast<int>(componentType));
		commandInterface.set({channel, &AnimationChannelRaco::interpolationType_}, static_cast<int>(interpolation));
		commandInterface.set({channel, &AnimationChannelRaco::componentArraySize_}, 5);

		commandInterface.setAnimationData(channel, timeStamps, keyFrames, tangentsIn, tangentsOut);

		EXPECT_TRUE(channel->data_->hasProperty(user_types::AnimationChannelRaco::PROPNAME_TIME_STAMPS));
		EXPECT_TRUE(channel->data_->hasProperty(user_types::AnimationChannelRaco::PROPNAME_KEYFRAMES));
		EXPECT_EQ(channel->data_->hasProperty(user_types::AnimationChannelRaco::PROPNAME_TANGENTS_IN), !tangentsIn.empty());
		EXPECT_EQ(channel->data_->hasProperty(user_types::AnimationChannelRaco::PROPNAME_TANGENTS_OUT), !tangentsOut.empty());

		EXPECT_EQ(channel->data_->get(user_types::AnimationChannelRaco::PROPNAME_TIME_STAMPS)->asArray().size(), timeStamps.size());
		EXPECT_EQ(channel->data_->get(user_types::AnimationChannelRaco::PROPNAME_KEYFRAMES)->asArray().size(), keyFrames.size());

		EXPECT_EQ(channel->currentSamplerData_->timeStamps, timeStamps);

		EXPECT_TRUE(std::holds_alternative<AnimationOutputData<U>>(channel->currentSamplerData_->output));

		auto data = std::get<AnimationOutputData<U>>(channel->currentSamplerData_->output);
		EXPECT_EQ(toVector(data.keyFrames), keyFrames);
		EXPECT_EQ(toVector(data.tangentsIn), tangentsIn);
		EXPECT_EQ(toVector(data.tangentsOut), tangentsOut);
	}
};

TEST_F(AnimationChannelRacoTest, creation) {
	auto channel = create<AnimationChannelRaco>("channel");
}

TEST_F(AnimationChannelRacoTest, validate_interpolation) {
	auto channel = create<AnimationChannelRaco>("channel");

	ValueHandle interpolationHandle{channel, &AnimationChannelRaco::interpolationType_};

	commandInterface.set({channel, &AnimationChannelRaco::componentType_}, static_cast<int>(EnginePrimitive::Vec3f));
	
	commandInterface.set(interpolationHandle, static_cast<int>(MeshAnimationInterpolation::Linear));
	EXPECT_FALSE(commandInterface.errors().hasError(interpolationHandle));

	commandInterface.set(interpolationHandle, static_cast<int>(MeshAnimationInterpolation::Linear_Quaternion));
	EXPECT_TRUE(commandInterface.errors().hasError(interpolationHandle));

	commandInterface.set({channel, &AnimationChannelRaco::componentType_}, static_cast<int>(EnginePrimitive::Vec4f));

	commandInterface.set(interpolationHandle, static_cast<int>(MeshAnimationInterpolation::Linear));
	EXPECT_FALSE(commandInterface.errors().hasError(interpolationHandle));

	commandInterface.set(interpolationHandle, static_cast<int>(MeshAnimationInterpolation::Linear_Quaternion));
	EXPECT_FALSE(commandInterface.errors().hasError(interpolationHandle));
}

TEST_F(AnimationChannelRacoTest, set_data_float_empty_linear) {
	auto channel = create<AnimationChannelRaco>("channel");

	commandInterface.set({channel, &AnimationChannelRaco::componentType_}, static_cast<int>(EnginePrimitive::Double));
	commandInterface.set({channel, &AnimationChannelRaco::interpolationType_}, static_cast<int>(MeshAnimationInterpolation::Linear));

	std::vector<float> timeStamps;
	std::vector<float> keyFrames;

	commandInterface.setAnimationData(channel, timeStamps, keyFrames);

	EXPECT_TRUE(channel->data_->hasProperty(user_types::AnimationChannelRaco::PROPNAME_TIME_STAMPS));
	EXPECT_TRUE(channel->data_->hasProperty(user_types::AnimationChannelRaco::PROPNAME_KEYFRAMES));
	EXPECT_FALSE(channel->data_->hasProperty(user_types::AnimationChannelRaco::PROPNAME_TANGENTS_IN));
	EXPECT_FALSE(channel->data_->hasProperty(user_types::AnimationChannelRaco::PROPNAME_TANGENTS_OUT));

	EXPECT_EQ(channel->data_->get(user_types::AnimationChannelRaco::PROPNAME_TIME_STAMPS)->asArray().size(), 0);
	EXPECT_EQ(channel->data_->get(user_types::AnimationChannelRaco::PROPNAME_KEYFRAMES)->asArray().size(), 0);
}

TEST_F(AnimationChannelRacoTest, set_data_array_empty_linear) {
	auto channel = create<AnimationChannelRaco>("channel");

	commandInterface.set({channel, &AnimationChannelRaco::componentType_}, static_cast<int>(EnginePrimitive::Array));
	commandInterface.set({channel, &AnimationChannelRaco::interpolationType_}, static_cast<int>(MeshAnimationInterpolation::Linear));
	commandInterface.set({channel, &AnimationChannelRaco::componentArraySize_}, 5);

	std::vector<float> timeStamps;
	std::vector<std::vector<float>> keyFrames;

	commandInterface.setAnimationData(channel, timeStamps, keyFrames);

	EXPECT_TRUE(channel->data_->hasProperty(user_types::AnimationChannelRaco::PROPNAME_TIME_STAMPS));
	EXPECT_TRUE(channel->data_->hasProperty(user_types::AnimationChannelRaco::PROPNAME_KEYFRAMES));
	EXPECT_FALSE(channel->data_->hasProperty(user_types::AnimationChannelRaco::PROPNAME_TANGENTS_IN));
	EXPECT_FALSE(channel->data_->hasProperty(user_types::AnimationChannelRaco::PROPNAME_TANGENTS_OUT));

	EXPECT_EQ(channel->data_->get(user_types::AnimationChannelRaco::PROPNAME_TIME_STAMPS)->asArray().size(), 0);
	EXPECT_EQ(channel->data_->get(user_types::AnimationChannelRaco::PROPNAME_KEYFRAMES)->asArray().size(), 0);

	EXPECT_EQ(channel->currentSamplerData_->getOutputComponentSize(), 5);
}


TEST_F(AnimationChannelRacoTest, set_data_float_empty_cubic) {
	auto channel = create<AnimationChannelRaco>("channel");

	commandInterface.set({channel, &AnimationChannelRaco::componentType_}, static_cast<int>(EnginePrimitive::Double));
	commandInterface.set({channel, &AnimationChannelRaco::interpolationType_}, static_cast<int>(MeshAnimationInterpolation::CubicSpline));

	std::vector<float> timeStamps;
	std::vector<float> keyFrames;
	std::vector<float> tangentsIn;
	std::vector<float> tangentsOut;

	commandInterface.setAnimationData(channel, timeStamps, keyFrames, tangentsIn, tangentsOut);

	EXPECT_TRUE(channel->data_->hasProperty(user_types::AnimationChannelRaco::PROPNAME_TIME_STAMPS));
	EXPECT_TRUE(channel->data_->hasProperty(user_types::AnimationChannelRaco::PROPNAME_KEYFRAMES));
	EXPECT_FALSE(channel->data_->hasProperty(user_types::AnimationChannelRaco::PROPNAME_TANGENTS_IN));
	EXPECT_FALSE(channel->data_->hasProperty(user_types::AnimationChannelRaco::PROPNAME_TANGENTS_OUT));

	EXPECT_EQ(channel->data_->get(user_types::AnimationChannelRaco::PROPNAME_TIME_STAMPS)->asArray().size(), 0);
	EXPECT_EQ(channel->data_->get(user_types::AnimationChannelRaco::PROPNAME_KEYFRAMES)->asArray().size(), 0);
}

TEST_F(AnimationChannelRacoTest, set_data_float_linear) {
	check_set_get<float, float>(EnginePrimitive::Double, MeshAnimationInterpolation::Linear, 
		{0.0, 1.0, 2.0}, {0.0, 1.0, 4.0});
}

TEST_F(AnimationChannelRacoTest, set_data_float_cubic) {
	check_set_get<float, float>(EnginePrimitive::Double, MeshAnimationInterpolation::CubicSpline,
		{0.0, 1.0, 2.0}, {0.0, 1.0, 4.0}, std::vector<float>(3, 0.0), std::vector<float>(3, 1.0));
}

TEST_F(AnimationChannelRacoTest, set_data_vec2f_linear) {
	check_set_get<std::vector<float>, glm::vec2>(EnginePrimitive::Vec2f, MeshAnimationInterpolation::Linear,
		{0.0, 1.0, 2.0}, {{1.0, 2.0}, {2.0, 3.0}, {3.0, 4.0}});
}

TEST_F(AnimationChannelRacoTest, set_data_vec2f_cubic) {
	check_set_get<std::vector<float>, glm::vec2>(EnginePrimitive::Vec2f, MeshAnimationInterpolation::CubicSpline,
		{0.0, 1.0, 2.0}, {{1.0, 2.0}, {2.0, 3.0}, {3.0, 4.0}},
		{{0, 0}, {0, 0}, {0, 0}}, {{1, 1}, {1, 1}, {1, 1}});
}

TEST_F(AnimationChannelRacoTest, set_data_vec3f_linear) {
	check_set_get<std::vector<float>, glm::vec3>(EnginePrimitive::Vec3f, MeshAnimationInterpolation::Linear,
		{0.0, 1.0, 2.0}, {{1.0, 2.0, 3.0}, {2.0, 3.0, 4.0}, {3.0, 4.0, 5.0}});
}

TEST_F(AnimationChannelRacoTest, set_data_vec3f_cubic) {
	check_set_get<std::vector<float>, glm::vec3>(EnginePrimitive::Vec3f, MeshAnimationInterpolation::CubicSpline,
		{0.0, 1.0, 2.0}, {{1.0, 2.0, 3.0}, {2.0, 3.0, 4.0}, {3.0, 4.0, 5.0}},
		{{0, 0, 0}, {0, 0, 0}, {0, 0, 0}}, {{1, 1,1 }, {1, 1, 1}, {1, 1, 1}});
}

TEST_F(AnimationChannelRacoTest, set_data_vec4f_linear) {
	check_set_get<std::vector<float>, glm::vec4>(EnginePrimitive::Vec4f, MeshAnimationInterpolation::Linear,
		{0.0, 1.0, 2.0}, {{1.0, 2.0, 3.0, 4.0}, {2.0, 3.0, 4.0, 5.0}, {3.0, 4.0, 5.0, 6.0}});
}

TEST_F(AnimationChannelRacoTest, set_data_vec4f_cubic) {
	check_set_get<std::vector<float>, glm::vec4>(EnginePrimitive::Vec4f, MeshAnimationInterpolation::CubicSpline,
		{0.0, 1.0, 2.0}, {{1.0, 2.0, 3.0, 4.0}, {2.0, 3.0, 4.0, 5.0}, {3.0, 4.0, 5.0, 6.0}},
		{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, {{1, 1, 1, 1}, {1, 1, 1, 1}, {1, 1, 1, 1}});
}


TEST_F(AnimationChannelRacoTest, set_data_int_linear) {
	check_set_get<int, int32_t>(EnginePrimitive::Int32, MeshAnimationInterpolation::Linear,
		{0.0, 1.0, 2.0}, {0, 1, 4});
}

TEST_F(AnimationChannelRacoTest, set_data_int_cubic) {
	check_set_get<int, int32_t>(EnginePrimitive::Int32, MeshAnimationInterpolation::CubicSpline,
		{0, 1, 2}, {0, 1, 4}, std::vector<int>(3, 0), std::vector<int>(3, 1));
}

TEST_F(AnimationChannelRacoTest, set_data_vec2i_linear) {
	check_set_get<std::vector<int>, glm::ivec2>(EnginePrimitive::Vec2i, MeshAnimationInterpolation::Linear,
		{0, 1, 2}, {{1, 2}, {2, 3}, {3, 4}});
}

TEST_F(AnimationChannelRacoTest, set_data_vec2i_cubic) {
	check_set_get<std::vector<int>, glm::ivec2>(EnginePrimitive::Vec2i, MeshAnimationInterpolation::CubicSpline,
		{0, 1, 2}, {{1, 2}, {2, 3}, {3, 4}},
		{{0, 0}, {0, 0}, {0, 0}}, {{1, 1}, {1, 1}, {1, 1}});
}

TEST_F(AnimationChannelRacoTest, set_data_vec3i_linear) {
	check_set_get<std::vector<int>, glm::ivec3>(EnginePrimitive::Vec3i, MeshAnimationInterpolation::Linear,
		{0, 1, 2}, {{1, 2, 3}, {2, 3, 4}, {3, 4, 5}});
}

TEST_F(AnimationChannelRacoTest, set_data_vec3i_cubic) {
	check_set_get<std::vector<int>, glm::ivec3>(EnginePrimitive::Vec3i, MeshAnimationInterpolation::CubicSpline,
		{0, 1, 2}, {{1, 2, 3}, {2, 3, 4}, {3, 4, 5}},
		{{0, 0, 0}, {0, 0, 0}, {0, 0, 0}}, {{1, 1, 1}, {1, 1, 1}, {1, 1, 1}});
}

TEST_F(AnimationChannelRacoTest, set_data_vec4i_linear) {
	check_set_get<std::vector<int>, glm::ivec4>(EnginePrimitive::Vec4i, MeshAnimationInterpolation::Linear,
		{0, 1, 2}, {{1, 2, 3, 4}, {2, 3, 4, 5}, {3, 4, 5, 6}});
}

TEST_F(AnimationChannelRacoTest, set_data_vec4i_cubic) {
	check_set_get<std::vector<int>, glm::ivec4>(EnginePrimitive::Vec4i, MeshAnimationInterpolation::CubicSpline,
		{0, 1, 2}, {{1, 2, 3, 4}, {2, 3, 4, 5}, {3, 4, 5, 6}},
		{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, {{1, 1, 1, 1}, {1, 1, 1, 1}, {1, 1, 1, 1}});
}


TEST_F(AnimationChannelRacoTest, set_data_array_linear) {
	check_set_get<std::vector<float>, std::vector<float>>(EnginePrimitive::Array, MeshAnimationInterpolation::Linear,
		{0.0, 1.0, 2.0}, {{1.0, 2.0, 3.0, 4.0, 5.0}, {2.0, 3.0, 4.0, 5.0, 6.0}, {3.0, 4.0, 5.0, 6.0, 7.0}});
}

TEST_F(AnimationChannelRacoTest, set_data_array_cubic) {
	check_set_get<std::vector<float>, std::vector<float>>(EnginePrimitive::Array, MeshAnimationInterpolation::CubicSpline,
		{0.0, 1.0, 2.0}, {{1.0, 2.0, 3.0, 4.0, 5.0}, {2.0, 3.0, 4.0, 5.0, 6.0}, {3.0, 4.0, 5.0, 6.0, 7.0}},
		{{0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}}, {{1, 1, 1, 1, 1}, {1, 1, 1, 1, 1}, {1, 1, 1, 1, 1}});
}

TEST_F(AnimationChannelRacoTest, conv_gtlf_to_raco_morph_weights) {
	auto channel_gltf = create_animation_channel("channel", "meshes/AnimatedMorphCube/AnimatedMorphCube.gltf", 0, 0);
	auto animation = create_animation("animation", {channel_gltf});

	auto data_gltf = channel_gltf->currentSamplerData_;

	ASSERT_EQ(project.instances().size(), 3);
	ASSERT_TRUE(select<AnimationChannel>(project.instances()) != nullptr);
	EXPECT_EQ(**animation->animationChannels_->get(0), channel_gltf);

	auto raco_channels = commandInterface.convertToAnimationChannelRaco({channel_gltf});
	ASSERT_EQ(raco_channels.size(), 1);

	ASSERT_EQ(project.instances().size(), 3);
	ASSERT_TRUE(select<AnimationChannel>(project.instances()) == nullptr);
	ASSERT_TRUE(select<AnimationChannelRaco>(project.instances()) != nullptr);

	auto channel_raco = raco_channels[0]->as<AnimationChannelRaco>();
	ASSERT_TRUE(channel_raco != nullptr);
	EXPECT_EQ(**animation->animationChannels_->get(0), channel_raco);

	EXPECT_EQ(*channel_raco->componentType_, static_cast<int>(EnginePrimitive::Array));
	EXPECT_EQ(*channel_raco->interpolationType_, static_cast<int>(MeshAnimationInterpolation::Linear));
	EXPECT_EQ(*channel_raco->componentArraySize_, 2);

	EXPECT_EQ(*data_gltf, *channel_raco->currentSamplerData_);
}

TEST_F(AnimationChannelRacoTest, conv_gtlf_to_raco_transformation) {
	std::map<int, MeshAnimationInterpolation> interpolation = {
		// Scale
		{0, MeshAnimationInterpolation::Step},
		{1, MeshAnimationInterpolation::Linear},
		{2, MeshAnimationInterpolation::CubicSpline},
		// Rotation
		{3, MeshAnimationInterpolation::Step},
		{4, MeshAnimationInterpolation::CubicSpline_Quaternion},
		{5, MeshAnimationInterpolation::Linear_Quaternion},
		// Translation
		{6, MeshAnimationInterpolation::Step},
		{7, MeshAnimationInterpolation::CubicSpline},
		{8, MeshAnimationInterpolation::Linear}
	};

	for (auto animIndex = 0; animIndex < 9; animIndex++) {
		auto channel_gltf = create_animation_channel("channel", "meshes/InterpolationTest/InterpolationTest.gltf", animIndex, 0);

		auto data_gltf = channel_gltf->currentSamplerData_;

		ASSERT_EQ(project.instances().size(), 2);
		ASSERT_TRUE(select<AnimationChannel>(project.instances()) != nullptr);

		auto raco_channels = commandInterface.convertToAnimationChannelRaco({channel_gltf});
		ASSERT_EQ(raco_channels.size(), 1);

		ASSERT_EQ(project.instances().size(), 2);
		ASSERT_TRUE(select<AnimationChannel>(project.instances()) == nullptr);
		ASSERT_TRUE(select<AnimationChannelRaco>(project.instances()) != nullptr);

		auto channel_raco = raco_channels[0]->as<AnimationChannelRaco>();
		ASSERT_TRUE(channel_raco != nullptr);

		auto refComponentType = (animIndex / 3) == 1 ? EnginePrimitive::Vec4f : EnginePrimitive::Vec3f;
		EXPECT_EQ(*channel_raco->componentType_, static_cast<int>(refComponentType)) << fmt::format("anim index = {}", animIndex);
		EXPECT_EQ(*channel_raco->interpolationType_, static_cast<int>(interpolation[animIndex])) << fmt::format("anim index = {}", animIndex);
		EXPECT_EQ(*channel_raco->componentArraySize_, 1);

		EXPECT_EQ(*data_gltf, *channel_raco->currentSamplerData_);

		commandInterface.deleteObjects({channel_raco});
	}
}
