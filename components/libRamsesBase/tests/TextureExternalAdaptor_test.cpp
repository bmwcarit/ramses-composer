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
#include "ramses_adaptor/TextureExternalAdaptor.h"
#include "testing/TestUtil.h"

class TextureExternalAdaptorFixture : public RamsesBaseFixture<> {};

TEST_F(TextureExternalAdaptorFixture, creation_sets_name) {
    auto texture = create<user_types::TextureExternal>("test");

    dispatch();

    auto engineSampler = select<ramses::TextureSamplerExternal>(*sceneContext.scene(), "test");
	EXPECT_TRUE(engineSampler != nullptr);
	EXPECT_EQ(engineSampler->getName(), std::string("test"));
}

TEST_F(TextureExternalAdaptorFixture, change_name) {
	auto texture = create<user_types::TextureExternal>("test");

	dispatch();
	{
		auto engineSamplers{select<ramses::TextureSamplerExternal>(*sceneContext.scene(), ramses::ERamsesObjectType::TextureSamplerExternal)};
		EXPECT_EQ(engineSamplers.size(), 1);
		EXPECT_TRUE("test" == engineSamplers[0]->getName());
	}

	commandInterface.set({texture, &user_types::TextureExternal::objectName_}, std::string("newName"));

	dispatch();
	{
		auto engineSamplers{select<ramses::TextureSamplerExternal>(*sceneContext.scene(), ramses::ERamsesObjectType::TextureSamplerExternal)};
		EXPECT_EQ(engineSamplers.size(), 1);
		EXPECT_TRUE("newName" == engineSamplers[0]->getName());
	}
}