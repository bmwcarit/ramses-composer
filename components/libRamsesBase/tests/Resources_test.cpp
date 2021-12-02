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
#include "ramses_adaptor/CubeMapAdaptor.h"
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

TEST_F(ResourcesAdaptorFixture, texture_info_box) {
	auto texture = create<user_types::Texture>("texture name");
	context.set({texture, {"uri"}}, (cwd_path() / "images" / "DuckCM.png").string());
	dispatch();

	auto infoBoxError = context.errors().getError(raco::core::ValueHandle{texture});
	EXPECT_EQ(infoBoxError.category(), raco::core::ErrorCategory::GENERAL);
	EXPECT_EQ(infoBoxError.level(), raco::core::ErrorLevel::INFORMATION);
	EXPECT_EQ(infoBoxError.message(), "Texture information\n\nWidth: 512 px\nHeight: 512 px\n\nFormat: RGBA8");
}

TEST_F(ResourcesAdaptorFixture, cube_map_info_box) {
	auto cubemap = create<user_types::CubeMap>("cube map name");
	context.set({cubemap, {"uriFront"}}, (cwd_path() / "images" / "DuckCM.png").string());
	context.set({cubemap, {"uriBack"}}, (cwd_path() / "images" / "DuckCM.png").string());
	context.set({cubemap, {"uriLeft"}}, (cwd_path() / "images" / "DuckCM.png").string());
	context.set({cubemap, {"uriRight"}}, (cwd_path() / "images" / "DuckCM.png").string());
	context.set({cubemap, {"uriTop"}}, (cwd_path() / "images" / "DuckCM.png").string());
	context.set({cubemap, {"uriBottom"}}, (cwd_path() / "images" / "DuckCM.png").string());
	dispatch();

	auto infoBoxError = context.errors().getError(raco::core::ValueHandle{cubemap});
	EXPECT_EQ(infoBoxError.category(), raco::core::ErrorCategory::GENERAL);
	EXPECT_EQ(infoBoxError.level(), raco::core::ErrorLevel::INFORMATION);
	EXPECT_EQ(infoBoxError.message(), "CubeMap information\n\nWidth: 512 px\nHeight: 512 px\n\nFormat: RGBA8");
}
