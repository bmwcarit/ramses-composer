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
#include "user_types/CubeMap.h"
#include <gtest/gtest.h>

using namespace raco::core;
using namespace raco::user_types;

class CubeMapTest : public TestEnvironmentCore {};

TEST_F(CubeMapTest, invalidLevels) {
	auto cubeMap{commandInterface.createObject(CubeMap::typeDescription.typeName)};

	ASSERT_FALSE(commandInterface.errors().hasError({cubeMap, &raco::user_types::CubeMap::mipmapLevel_}));

	commandInterface.set({cubeMap, &raco::user_types::CubeMap::mipmapLevel_}, -1);
	ASSERT_TRUE(commandInterface.errors().hasError({cubeMap, &raco::user_types::CubeMap::mipmapLevel_}));

	commandInterface.set({cubeMap, &raco::user_types::CubeMap::mipmapLevel_}, 1);
	ASSERT_FALSE(commandInterface.errors().hasError({cubeMap, &raco::user_types::CubeMap::mipmapLevel_}));

	commandInterface.set({cubeMap, &raco::user_types::CubeMap::mipmapLevel_}, 5);
	ASSERT_TRUE(commandInterface.errors().hasError({cubeMap, &raco::user_types::CubeMap::mipmapLevel_}));

	commandInterface.set({cubeMap, &raco::user_types::CubeMap::mipmapLevel_}, 4);
	ASSERT_FALSE(commandInterface.errors().hasError({cubeMap, &raco::user_types::CubeMap::mipmapLevel_}));
}

TEST_F(CubeMapTest, levelOtherThanOneWhenGenerationFlagIsActivated) {
	auto cubeMap{commandInterface.createObject(CubeMap::typeDescription.typeName)};

	commandInterface.set({cubeMap, &raco::user_types::CubeMap::mipmapLevel_}, 2);
	ASSERT_FALSE(commandInterface.errors().hasError({cubeMap, &raco::user_types::CubeMap::mipmapLevel_}));

	commandInterface.set({cubeMap, &raco::user_types::CubeMap::generateMipmaps_}, true);
	ASSERT_TRUE(commandInterface.errors().hasError({cubeMap, &raco::user_types::CubeMap::mipmapLevel_}));
	ASSERT_EQ(commandInterface.errors().getError({cubeMap, &raco::user_types::CubeMap::mipmapLevel_}).level(), raco::core::ErrorLevel::WARNING);

	commandInterface.set({cubeMap, &raco::user_types::CubeMap::generateMipmaps_}, false);
	ASSERT_FALSE(commandInterface.errors().hasError({cubeMap, &raco::user_types::CubeMap::mipmapLevel_}));
}
