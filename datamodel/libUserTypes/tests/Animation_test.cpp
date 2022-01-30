/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
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