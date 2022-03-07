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
#include "ramses_adaptor/CubeMapAdaptor.h"

class CubeMapAdaptorFixture : public RamsesBaseFixture<> {};

TEST_F(CubeMapAdaptorFixture, cubeMapGenerationAtMultipleLevels) {
	auto cubeMap = create<raco::user_types::CubeMap>("Cubemap");

	commandInterface.set({cubeMap, &raco::user_types::CubeMap::uriBack_}, (test_path() / "images" / "blue_1024.png").string());
	commandInterface.set({cubeMap, &raco::user_types::CubeMap::uriBottom_}, (test_path() / "images" / "blue_1024.png").string());
	commandInterface.set({cubeMap, &raco::user_types::CubeMap::uriFront_}, (test_path() / "images" / "blue_1024.png").string());
	commandInterface.set({cubeMap, &raco::user_types::CubeMap::uriLeft_}, (test_path() / "images" / "blue_1024.png").string());
	commandInterface.set({cubeMap, &raco::user_types::CubeMap::uriRight_}, (test_path() / "images" / "blue_1024.png").string());
	commandInterface.set({cubeMap, &raco::user_types::CubeMap::uriTop_}, (test_path() / "images" / "blue_1024.png").string());

	dispatch();

	commandInterface.set({cubeMap, &raco::user_types::CubeMap::level2uriBack_}, (test_path() / "images" / "green_512.png").string());
	commandInterface.set({cubeMap, &raco::user_types::CubeMap::level2uriBottom_}, (test_path() / "images" / "green_512.png").string());
	commandInterface.set({cubeMap, &raco::user_types::CubeMap::level2uriFront_}, (test_path() / "images" / "green_512.png").string());
	commandInterface.set({cubeMap, &raco::user_types::CubeMap::level2uriLeft_}, (test_path() / "images" / "green_512.png").string());
	commandInterface.set({cubeMap, &raco::user_types::CubeMap::level2uriRight_}, (test_path() / "images" / "green_512.png").string());
	commandInterface.set({cubeMap, &raco::user_types::CubeMap::level2uriTop_}, (test_path() / "images" / "green_512.png").string());

	dispatch();

	commandInterface.set({cubeMap, &raco::user_types::CubeMap::level3uriBack_}, (test_path() / "images" / "yellow_256.png").string());
	commandInterface.set({cubeMap, &raco::user_types::CubeMap::level3uriBottom_}, (test_path() / "images" / "yellow_256.png").string());
	commandInterface.set({cubeMap, &raco::user_types::CubeMap::level3uriFront_}, (test_path() / "images" / "yellow_256.png").string());
	commandInterface.set({cubeMap, &raco::user_types::CubeMap::level3uriLeft_}, (test_path() / "images" / "yellow_256.png").string());
	commandInterface.set({cubeMap, &raco::user_types::CubeMap::level3uriRight_}, (test_path() / "images" / "yellow_256.png").string());
	commandInterface.set({cubeMap, &raco::user_types::CubeMap::level3uriTop_}, (test_path() / "images" / "yellow_256.png").string());

	dispatch();

	commandInterface.set({cubeMap, &raco::user_types::CubeMap::level4uriBack_}, (test_path() / "images" / "red_128.png").string());
	commandInterface.set({cubeMap, &raco::user_types::CubeMap::level4uriBottom_}, (test_path() / "images" / "red_128.png").string());
	commandInterface.set({cubeMap, &raco::user_types::CubeMap::level4uriFront_}, (test_path() / "images" / "red_128.png").string());
	commandInterface.set({cubeMap, &raco::user_types::CubeMap::level4uriLeft_}, (test_path() / "images" / "red_128.png").string());
	commandInterface.set({cubeMap, &raco::user_types::CubeMap::level4uriRight_}, (test_path() / "images" / "red_128.png").string());
	commandInterface.set({cubeMap, &raco::user_types::CubeMap::level4uriTop_}, (test_path() / "images" / "red_128.png").string());

	dispatch();

	auto cubeMapStuff{select<ramses::TextureSampler>(*sceneContext.scene(), ramses::ERamsesObjectType::ERamsesObjectType_TextureSampler)};
	ASSERT_EQ(cubeMapStuff.size(), 1);

	commandInterface.set({cubeMap, &raco::user_types::CubeMap::mipmapLevel_}, 2);
	dispatch();
	cubeMapStuff = select<ramses::TextureSampler>(*sceneContext.scene(), ramses::ERamsesObjectType::ERamsesObjectType_TextureSampler);
	ASSERT_EQ(cubeMapStuff.size(), 1);

	commandInterface.set({cubeMap, &raco::user_types::CubeMap::mipmapLevel_}, 3);
	dispatch();
	cubeMapStuff = select<ramses::TextureSampler>(*sceneContext.scene(), ramses::ERamsesObjectType::ERamsesObjectType_TextureSampler);
	ASSERT_EQ(cubeMapStuff.size(), 1);

	commandInterface.set({cubeMap, &raco::user_types::CubeMap::mipmapLevel_}, 4);
	dispatch();
	cubeMapStuff = select<ramses::TextureSampler>(*sceneContext.scene(), ramses::ERamsesObjectType::ERamsesObjectType_TextureSampler);
	ASSERT_EQ(cubeMapStuff.size(), 1);
}

TEST_F(CubeMapAdaptorFixture, wrongMipMapLevelImageSizes) {
	auto cubeMap = create<raco::user_types::CubeMap>("Cubemap");

	commandInterface.set({cubeMap, &raco::user_types::CubeMap::mipmapLevel_}, 4);
	dispatch();

	commandInterface.set({cubeMap, &raco::user_types::CubeMap::uriBack_}, (test_path() / "images" / "blue_1024.png").string());
	commandInterface.set({cubeMap, &raco::user_types::CubeMap::uriBottom_}, (test_path() / "images" / "blue_1024.png").string());
	commandInterface.set({cubeMap, &raco::user_types::CubeMap::uriFront_}, (test_path() / "images" / "blue_1024.png").string());
	commandInterface.set({cubeMap, &raco::user_types::CubeMap::uriLeft_}, (test_path() / "images" / "blue_1024.png").string());
	commandInterface.set({cubeMap, &raco::user_types::CubeMap::uriRight_}, (test_path() / "images" / "blue_1024.png").string());
	commandInterface.set({cubeMap, &raco::user_types::CubeMap::uriTop_}, (test_path() / "images" / "blue_1024.png").string());
	dispatch();

	commandInterface.set({cubeMap, &raco::user_types::CubeMap::level2uriBack_}, (test_path() / "images" / "blue_1024.png").string());
	commandInterface.set({cubeMap, &raco::user_types::CubeMap::level2uriBottom_}, (test_path() / "images" / "blue_1024.png").string());
	commandInterface.set({cubeMap, &raco::user_types::CubeMap::level2uriFront_}, (test_path() / "images" / "blue_1024.png").string());
	commandInterface.set({cubeMap, &raco::user_types::CubeMap::level2uriLeft_}, (test_path() / "images" / "blue_1024.png").string());
	commandInterface.set({cubeMap, &raco::user_types::CubeMap::level2uriRight_}, (test_path() / "images" / "blue_1024.png").string());
	commandInterface.set({cubeMap, &raco::user_types::CubeMap::level2uriTop_}, (test_path() / "images" / "blue_1024.png").string());
	dispatch();

	ASSERT_TRUE(commandInterface.errors().hasError({cubeMap, &raco::user_types::CubeMap::level2uriBack_}));
	ASSERT_TRUE(commandInterface.errors().hasError({cubeMap, &raco::user_types::CubeMap::level2uriBottom_}));
	ASSERT_TRUE(commandInterface.errors().hasError({cubeMap, &raco::user_types::CubeMap::level2uriFront_}));
	ASSERT_TRUE(commandInterface.errors().hasError({cubeMap, &raco::user_types::CubeMap::level2uriLeft_}));
	ASSERT_TRUE(commandInterface.errors().hasError({cubeMap, &raco::user_types::CubeMap::level2uriRight_}));
	ASSERT_TRUE(commandInterface.errors().hasError({cubeMap, &raco::user_types::CubeMap::level2uriTop_}));

	commandInterface.set({cubeMap, &raco::user_types::CubeMap::level3uriBack_}, (test_path() / "images" / "blue_1024.png").string());
	commandInterface.set({cubeMap, &raco::user_types::CubeMap::level3uriBottom_}, (test_path() / "images" / "blue_1024.png").string());
	commandInterface.set({cubeMap, &raco::user_types::CubeMap::level3uriFront_}, (test_path() / "images" / "blue_1024.png").string());
	commandInterface.set({cubeMap, &raco::user_types::CubeMap::level3uriLeft_}, (test_path() / "images" / "blue_1024.png").string());
	commandInterface.set({cubeMap, &raco::user_types::CubeMap::level3uriRight_}, (test_path() / "images" / "blue_1024.png").string());
	commandInterface.set({cubeMap, &raco::user_types::CubeMap::level3uriTop_}, (test_path() / "images" / "blue_1024.png").string());
	dispatch();

	ASSERT_TRUE(commandInterface.errors().hasError({cubeMap, &raco::user_types::CubeMap::level3uriBack_}));
	ASSERT_TRUE(commandInterface.errors().hasError({cubeMap, &raco::user_types::CubeMap::level3uriBottom_}));
	ASSERT_TRUE(commandInterface.errors().hasError({cubeMap, &raco::user_types::CubeMap::level3uriFront_}));
	ASSERT_TRUE(commandInterface.errors().hasError({cubeMap, &raco::user_types::CubeMap::level3uriLeft_}));
	ASSERT_TRUE(commandInterface.errors().hasError({cubeMap, &raco::user_types::CubeMap::level3uriRight_}));
	ASSERT_TRUE(commandInterface.errors().hasError({cubeMap, &raco::user_types::CubeMap::level3uriTop_}));

	commandInterface.set({cubeMap, &raco::user_types::CubeMap::level4uriBack_}, (test_path() / "images" / "blue_1024.png").string());
	commandInterface.set({cubeMap, &raco::user_types::CubeMap::level4uriBottom_}, (test_path() / "images" / "blue_1024.png").string());
	commandInterface.set({cubeMap, &raco::user_types::CubeMap::level4uriFront_}, (test_path() / "images" / "blue_1024.png").string());
	commandInterface.set({cubeMap, &raco::user_types::CubeMap::level4uriLeft_}, (test_path() / "images" / "blue_1024.png").string());
	commandInterface.set({cubeMap, &raco::user_types::CubeMap::level4uriRight_}, (test_path() / "images" / "blue_1024.png").string());
	commandInterface.set({cubeMap, &raco::user_types::CubeMap::level4uriTop_}, (test_path() / "images" / "blue_1024.png").string());
	dispatch();

	ASSERT_TRUE(commandInterface.errors().hasError({cubeMap, &raco::user_types::CubeMap::level4uriBack_}));
	ASSERT_TRUE(commandInterface.errors().hasError({cubeMap, &raco::user_types::CubeMap::level4uriBottom_}));
	ASSERT_TRUE(commandInterface.errors().hasError({cubeMap, &raco::user_types::CubeMap::level4uriFront_}));
	ASSERT_TRUE(commandInterface.errors().hasError({cubeMap, &raco::user_types::CubeMap::level4uriLeft_}));
	ASSERT_TRUE(commandInterface.errors().hasError({cubeMap, &raco::user_types::CubeMap::level4uriRight_}));
	ASSERT_TRUE(commandInterface.errors().hasError({cubeMap, &raco::user_types::CubeMap::level4uriTop_}));

	commandInterface.set({cubeMap, &raco::user_types::CubeMap::mipmapLevel_}, 3);
	dispatch();

	ASSERT_FALSE(commandInterface.errors().hasError({cubeMap, &raco::user_types::CubeMap::level4uriBack_}));
	ASSERT_FALSE(commandInterface.errors().hasError({cubeMap, &raco::user_types::CubeMap::level4uriBottom_}));
	ASSERT_FALSE(commandInterface.errors().hasError({cubeMap, &raco::user_types::CubeMap::level4uriFront_}));
	ASSERT_FALSE(commandInterface.errors().hasError({cubeMap, &raco::user_types::CubeMap::level4uriLeft_}));
	ASSERT_FALSE(commandInterface.errors().hasError({cubeMap, &raco::user_types::CubeMap::level4uriRight_}));
	ASSERT_FALSE(commandInterface.errors().hasError({cubeMap, &raco::user_types::CubeMap::level4uriTop_}));

	commandInterface.set({cubeMap, &raco::user_types::CubeMap::mipmapLevel_}, 2);
	dispatch();

	ASSERT_FALSE(commandInterface.errors().hasError({cubeMap, &raco::user_types::CubeMap::level3uriBack_}));
	ASSERT_FALSE(commandInterface.errors().hasError({cubeMap, &raco::user_types::CubeMap::level3uriBottom_}));
	ASSERT_FALSE(commandInterface.errors().hasError({cubeMap, &raco::user_types::CubeMap::level3uriFront_}));
	ASSERT_FALSE(commandInterface.errors().hasError({cubeMap, &raco::user_types::CubeMap::level3uriLeft_}));
	ASSERT_FALSE(commandInterface.errors().hasError({cubeMap, &raco::user_types::CubeMap::level3uriRight_}));
	ASSERT_FALSE(commandInterface.errors().hasError({cubeMap, &raco::user_types::CubeMap::level3uriTop_}));

	commandInterface.set({cubeMap, &raco::user_types::CubeMap::mipmapLevel_}, 1);
	dispatch();

	ASSERT_FALSE(commandInterface.errors().hasError({cubeMap, &raco::user_types::CubeMap::level2uriBack_}));
	ASSERT_FALSE(commandInterface.errors().hasError({cubeMap, &raco::user_types::CubeMap::level2uriBottom_}));
	ASSERT_FALSE(commandInterface.errors().hasError({cubeMap, &raco::user_types::CubeMap::level2uriFront_}));
	ASSERT_FALSE(commandInterface.errors().hasError({cubeMap, &raco::user_types::CubeMap::level2uriLeft_}));
	ASSERT_FALSE(commandInterface.errors().hasError({cubeMap, &raco::user_types::CubeMap::level2uriRight_}));
	ASSERT_FALSE(commandInterface.errors().hasError({cubeMap, &raco::user_types::CubeMap::level2uriTop_}));
}

TEST_F(CubeMapAdaptorFixture, ramsesAutoMipMapGenerationWarningPersistsAfterChangingURI) {
	auto cubeMap = create<raco::user_types::CubeMap>("Cubemap");

	commandInterface.set({cubeMap, &raco::user_types::CubeMap::generateMipmaps_}, true);
	commandInterface.set({cubeMap, &raco::user_types::CubeMap::mipmapLevel_}, 2);
	dispatch();
	ASSERT_TRUE(commandInterface.errors().hasError({cubeMap, &raco::user_types::CubeMap::mipmapLevel_}));

	commandInterface.set({cubeMap, &raco::user_types::CubeMap::uriBack_}, (test_path() / "images" / "blue_1024.png").string());
	commandInterface.set({cubeMap, &raco::user_types::CubeMap::uriBottom_}, (test_path() / "images" / "blue_1024.png").string());
	commandInterface.set({cubeMap, &raco::user_types::CubeMap::uriFront_}, (test_path() / "images" / "blue_1024.png").string());
	commandInterface.set({cubeMap, &raco::user_types::CubeMap::uriLeft_}, (test_path() / "images" / "blue_1024.png").string());
	commandInterface.set({cubeMap, &raco::user_types::CubeMap::uriRight_}, (test_path() / "images" / "blue_1024.png").string());
	commandInterface.set({cubeMap, &raco::user_types::CubeMap::uriTop_}, (test_path() / "images" / "blue_1024.png").string());

	dispatch();
	ASSERT_TRUE(commandInterface.errors().hasError({cubeMap, &raco::user_types::CubeMap::mipmapLevel_}));

	commandInterface.set({cubeMap, &raco::user_types::CubeMap::level2uriBack_}, (test_path() / "images" / "green_512.png").string());
	commandInterface.set({cubeMap, &raco::user_types::CubeMap::level2uriBottom_}, (test_path() / "images" / "green_512.png").string());
	commandInterface.set({cubeMap, &raco::user_types::CubeMap::level2uriFront_}, (test_path() / "images" / "green_512.png").string());
	commandInterface.set({cubeMap, &raco::user_types::CubeMap::level2uriLeft_}, (test_path() / "images" / "green_512.png").string());
	commandInterface.set({cubeMap, &raco::user_types::CubeMap::level2uriRight_}, (test_path() / "images" / "green_512.png").string());
	commandInterface.set({cubeMap, &raco::user_types::CubeMap::level2uriTop_}, (test_path() / "images" / "green_512.png").string());

	dispatch();
	ASSERT_TRUE(commandInterface.errors().hasError({cubeMap, &raco::user_types::CubeMap::mipmapLevel_}));
}