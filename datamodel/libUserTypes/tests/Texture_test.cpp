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
#include "application/RaCoApplication.h"
#include "application/RaCoProject.h"
#include "core/Errors.h"
#include "user_types/Texture.h"
#include <gtest/gtest.h>

using namespace raco::core;
using namespace raco::user_types;

class TextureTest : public TestEnvironmentCore {};

TEST_F(TextureTest, invalidLevels) {
	auto texture{commandInterface.createObject(Texture::typeDescription.typeName)};

	ASSERT_FALSE(commandInterface.errors().hasError({texture, &raco::user_types::Texture::mipmapLevel_}));

	commandInterface.set({texture, &raco::user_types::Texture::mipmapLevel_}, -1);
	ASSERT_TRUE(commandInterface.errors().hasError({texture, &raco::user_types::Texture::mipmapLevel_}));

	commandInterface.set({texture, &raco::user_types::Texture::mipmapLevel_}, 1);
	ASSERT_FALSE(commandInterface.errors().hasError({texture, &raco::user_types::Texture::mipmapLevel_}));

	commandInterface.set({texture, &raco::user_types::Texture::mipmapLevel_}, 5);
	ASSERT_TRUE(commandInterface.errors().hasError({texture, &raco::user_types::Texture::mipmapLevel_}));

	commandInterface.set({texture, &raco::user_types::Texture::mipmapLevel_}, 4);
	ASSERT_FALSE(commandInterface.errors().hasError({texture, &raco::user_types::Texture::mipmapLevel_}));
}

TEST_F(TextureTest, levelOtherThanOneWhenGenerationFlagIsActivated) {
	auto texture{commandInterface.createObject(Texture::typeDescription.typeName)};

	commandInterface.set({texture, &raco::user_types::Texture::mipmapLevel_}, 2);
	ASSERT_FALSE(commandInterface.errors().hasError({texture, &raco::user_types::Texture::mipmapLevel_}));

	commandInterface.set({texture, &raco::user_types::Texture::generateMipmaps_}, true);
	ASSERT_TRUE(commandInterface.errors().hasError({texture, &raco::user_types::Texture::mipmapLevel_}));
	ASSERT_EQ(commandInterface.errors().getError({texture, &raco::user_types::Texture::mipmapLevel_}).level(), raco::core::ErrorLevel::WARNING);

	commandInterface.set({texture, &raco::user_types::Texture::generateMipmaps_}, false);
	ASSERT_FALSE(commandInterface.errors().hasError({texture, &raco::user_types::Texture::mipmapLevel_}));
}

TEST_F(TextureTest, error_present_after_load) {
	raco::ramses_base::HeadlessEngineBackend backend{raco::ramses_base::BaseEngineBackend::maxFeatureLevel};

	{
		raco::application::RaCoApplication app{backend};
		auto& cmd = *app.activeRaCoProject().commandInterface();

		auto texture{cmd.createObject(Texture::typeDescription.typeName, "texture")};
		cmd.set({texture, &raco::user_types::Texture::mipmapLevel_}, -1);
		ASSERT_TRUE(cmd.errors().hasError({texture, &raco::user_types::Texture::mipmapLevel_}));

		std::string msg;
		app.activeRaCoProject().saveAs(QString::fromStdString((test_path() / "test.rca").string()), msg);
	}

	{
		raco::application::RaCoApplicationLaunchSettings settings;
		settings.initialProject = (test_path() / "test.rca").string().c_str();
		raco::application::RaCoApplication app{backend, settings};

		auto& project = *app.activeRaCoProject().project();

		auto texture = raco::core::Queries::findByName(project.instances(), "texture");

		ASSERT_TRUE(app.activeRaCoProject().errors()->hasError({texture, &raco::user_types::Texture::mipmapLevel_}));
	}
}