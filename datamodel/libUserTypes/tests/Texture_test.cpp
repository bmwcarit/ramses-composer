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
