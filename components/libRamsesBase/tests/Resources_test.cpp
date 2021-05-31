/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include <gtest/gtest.h>

#include "RamsesBaseFixture.h"
#include "ramses_adaptor/TextureSamplerAdaptor.h"
#include "ramses_adaptor/SceneAdaptor.h"
#include "ramses_adaptor/utilities.h"

using namespace raco;

class ResourcesAdaptorFixture : public RamsesBaseFixture<> {};


TEST_F(ResourcesAdaptorFixture, texture_name_change) {
	auto texture = create<user_types::Texture>("texture name");
	context.set({texture, {"uri"}}, (cwd_path() / "images" / "DuckCM.png").string());

	dispatch();

	{
		auto engineTextures{select<ramses::TextureSampler>(*sceneContext.scene(), ramses::ERamsesObjectType::ERamsesObjectType_TextureSampler)};
		EXPECT_EQ(engineTextures.size(), 1);
		EXPECT_STREQ("texture name", engineTextures[0]->getName());
	}

	context.set({texture, {"objectName"}}, std::string("Changed"));
	dispatch();

	{
		auto engineTextures{select<ramses::TextureSampler>(*sceneContext.scene(), ramses::ERamsesObjectType::ERamsesObjectType_TextureSampler)};
		EXPECT_EQ(engineTextures.size(), 1);
		EXPECT_STREQ("Changed", engineTextures[0]->getName());
	}
}
