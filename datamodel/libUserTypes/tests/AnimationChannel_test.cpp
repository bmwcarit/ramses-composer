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
#include "user_types/AnimationChannel.h"
#include "utils/FileUtils.h"

#include <gtest/gtest.h>

using namespace raco::core;
using namespace raco::user_types;

class AnimationChannelTest : public TestEnvironmentCore {};

TEST_F(AnimationChannelTest, URI_setValidURI) {
	auto animChannel{commandInterface.createObject(AnimationChannel::typeDescription.typeName)};
	ValueHandle m{animChannel};
	ValueHandle m_uri{m.get("uri")};
	auto animChannelPath = test_path().append("meshes/CesiumMilkTruck/CesiumMilkTruck.gltf").string();
	commandInterface.set(m_uri, animChannelPath);
	ASSERT_EQ(m_uri.asString(), animChannelPath);
}

TEST_F(AnimationChannelTest, URI_setInvalidURI_error) {
	auto animChannel{commandInterface.createObject(AnimationChannel::typeDescription.typeName)};
	ValueHandle uriHandle{animChannel, {"uri"}};

	ASSERT_TRUE(commandInterface.errors().hasError(uriHandle));
}

TEST_F(AnimationChannelTest, URI_setValidURI_noError) {
	auto animChannel{commandInterface.createObject(AnimationChannel::typeDescription.typeName)};
	ValueHandle uriHandle{animChannel, {"uri"}};
	auto animChannelPath = test_path().append("meshes/CesiumMilkTruck/CesiumMilkTruck.gltf").string();
	commandInterface.set(uriHandle, animChannelPath);

	ASSERT_FALSE(commandInterface.errors().hasError(uriHandle));
}

TEST_F(AnimationChannelTest, URI_setValidURI_noAnims) {
	auto animChannel{commandInterface.createObject(AnimationChannel::typeDescription.typeName)};
	ValueHandle uriHandle{animChannel, {"uri"}};
	auto animChannelPath = test_path().append("meshes/Duck.glb").string();
	commandInterface.set(uriHandle, animChannelPath);

	ASSERT_TRUE(commandInterface.errors().hasError(uriHandle));
	ASSERT_EQ(commandInterface.errors().getError(uriHandle).level(), core::ErrorLevel::WARNING);
}

TEST_F(AnimationChannelTest, Inputs_setInvalidAnimationIndex_error) {
	auto animChannel{commandInterface.createObject(AnimationChannel::typeDescription.typeName)};
	ValueHandle animHandle{animChannel};
	ValueHandle uriHandle{animChannel, {"uri"}};
	ValueHandle animIndexHandle{animChannel, {"animationIndex"}};
	ValueHandle samplerIndexHandle{animChannel, {"samplerIndex"}};
	auto animChannelPath = test_path().append("meshes/CesiumMilkTruck/CesiumMilkTruck.gltf").string();
	commandInterface.set(uriHandle, animChannelPath);

	ASSERT_TRUE(commandInterface.errors().hasError(animHandle));
	ASSERT_EQ(commandInterface.errors().getError(animHandle).level(), core::ErrorLevel::INFORMATION);

	ASSERT_FALSE(commandInterface.errors().hasError(animIndexHandle));
	ASSERT_FALSE(commandInterface.errors().hasError(samplerIndexHandle));

	commandInterface.set(animIndexHandle, -1);
	ASSERT_FALSE(commandInterface.errors().hasError(animHandle));
	ASSERT_TRUE(commandInterface.errors().hasError(animIndexHandle));
	ASSERT_FALSE(commandInterface.errors().hasError(samplerIndexHandle));

	commandInterface.set(animIndexHandle, 0);
	ASSERT_TRUE(commandInterface.errors().hasError(animHandle));
	ASSERT_EQ(commandInterface.errors().getError(animHandle).level(), core::ErrorLevel::INFORMATION);
	ASSERT_FALSE(commandInterface.errors().hasError(animIndexHandle));
	ASSERT_FALSE(commandInterface.errors().hasError(samplerIndexHandle));

	commandInterface.set(animIndexHandle, 22);
	ASSERT_FALSE(commandInterface.errors().hasError(animHandle));
	ASSERT_TRUE(commandInterface.errors().hasError(animIndexHandle));
	ASSERT_FALSE(commandInterface.errors().hasError(samplerIndexHandle));

	commandInterface.set(animIndexHandle, 0);
	ASSERT_TRUE(commandInterface.errors().hasError(animHandle));
	ASSERT_EQ(commandInterface.errors().getError(animHandle).level(), core::ErrorLevel::INFORMATION);
	ASSERT_FALSE(commandInterface.errors().hasError(animIndexHandle));
	ASSERT_FALSE(commandInterface.errors().hasError(samplerIndexHandle));
}

TEST_F(AnimationChannelTest, Inputs_setInvalidSamplerIndex_noError) {
	auto animChannel{commandInterface.createObject(AnimationChannel::typeDescription.typeName)};
	ValueHandle animHandle{animChannel};
	ValueHandle uriHandle{animChannel, {"uri"}};
	ValueHandle animIndexHandle{animChannel, {"animationIndex"}};
	ValueHandle samplerIndexHandle{animChannel, {"samplerIndex"}};
	auto animChannelPath = test_path().append("meshes/CesiumMilkTruck/CesiumMilkTruck.gltf").string();
	commandInterface.set(uriHandle, animChannelPath);

	ASSERT_TRUE(commandInterface.errors().hasError(animHandle));
	ASSERT_EQ(commandInterface.errors().getError(animHandle).level(), core::ErrorLevel::INFORMATION);

	ASSERT_FALSE(commandInterface.errors().hasError(animIndexHandle));
	ASSERT_FALSE(commandInterface.errors().hasError(samplerIndexHandle));

	commandInterface.set(samplerIndexHandle, -1);
	ASSERT_FALSE(commandInterface.errors().hasError(animHandle));
	ASSERT_FALSE(commandInterface.errors().hasError(animIndexHandle));
	ASSERT_TRUE(commandInterface.errors().hasError(samplerIndexHandle));

	commandInterface.set(samplerIndexHandle, 0);
	ASSERT_TRUE(commandInterface.errors().hasError(animHandle));
	ASSERT_EQ(commandInterface.errors().getError(animHandle).level(), core::ErrorLevel::INFORMATION);
	ASSERT_FALSE(commandInterface.errors().hasError(animIndexHandle));
	ASSERT_FALSE(commandInterface.errors().hasError(samplerIndexHandle));

	commandInterface.set(samplerIndexHandle, 22);
	ASSERT_FALSE(commandInterface.errors().hasError(animHandle));
	ASSERT_FALSE(commandInterface.errors().hasError(animIndexHandle));
	ASSERT_TRUE(commandInterface.errors().hasError(samplerIndexHandle));

	commandInterface.set(samplerIndexHandle, 0);
	ASSERT_TRUE(commandInterface.errors().hasError(animHandle));
	ASSERT_EQ(commandInterface.errors().getError(animHandle).level(), core::ErrorLevel::INFORMATION);
	ASSERT_FALSE(commandInterface.errors().hasError(animIndexHandle));
	ASSERT_FALSE(commandInterface.errors().hasError(samplerIndexHandle));
}

TEST_F(AnimationChannelTest, invalidAnim_weights_supported) {
	auto animChannel{commandInterface.createObject(AnimationChannel::typeDescription.typeName)};
	ValueHandle uriHandle{animChannel, {"uri"}};

	std::string uriPath{(test_path() / "meshes" / "AnimatedMorphCube" / "AnimatedMorphCube.gltf").string()};
	commandInterface.set({animChannel, {"uri"}}, uriPath);

	ValueHandle samplerIndexHandle{animChannel, {"samplerIndex"}};
	ValueHandle animHandle{animChannel};
	ASSERT_TRUE(commandInterface.errors().hasError(animHandle));
	ASSERT_EQ(commandInterface.errors().getError(animHandle).level(), core::ErrorLevel::INFORMATION);
	ASSERT_FALSE(commandInterface.errors().hasError(samplerIndexHandle));
}

TEST_F(AnimationChannelTest, validAnim_madeInvalid) {
	auto animChannel = create<AnimationChannel>("anim_channel");
	ValueHandle uriHandle{animChannel, &user_types::AnimationChannel::uri_};

	auto animChannelPath = test_path().append("meshes/CesiumMilkTruck/CesiumMilkTruck.gltf").string();
	commandInterface.set(uriHandle, animChannelPath);
	ASSERT_FALSE(commandInterface.errors().hasError(uriHandle));

	recorder.reset();
	utils::file::write(animChannelPath, "");
	EXPECT_TRUE(awaitPreviewDirty(recorder, animChannel));

	ASSERT_TRUE(commandInterface.errors().hasError(uriHandle));
}
