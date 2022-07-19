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

#include <ramses-client-api/TextureEnums.h>

class CubeMapAdaptorFixture : public RamsesBaseFixture<> {
public:
	void assertLevel1Errors(raco::core::SEditorObject cubemap, raco::core::ErrorLevel level) {
		if (level == raco::core::ErrorLevel::NONE) {
			ASSERT_FALSE(commandInterface.errors().hasError({cubemap, &raco::user_types::CubeMap::uriBack_}));
			ASSERT_FALSE(commandInterface.errors().hasError({cubemap, &raco::user_types::CubeMap::uriBottom_}));
			ASSERT_FALSE(commandInterface.errors().hasError({cubemap, &raco::user_types::CubeMap::uriFront_}));
			ASSERT_FALSE(commandInterface.errors().hasError({cubemap, &raco::user_types::CubeMap::uriLeft_}));
			ASSERT_FALSE(commandInterface.errors().hasError({cubemap, &raco::user_types::CubeMap::uriRight_}));
			ASSERT_FALSE(commandInterface.errors().hasError({cubemap, &raco::user_types::CubeMap::uriTop_}));

		} else {
			ASSERT_TRUE(commandInterface.errors().hasError({cubemap, &raco::user_types::CubeMap::uriBack_}));
			ASSERT_EQ(commandInterface.errors().getError({cubemap, &raco::user_types::CubeMap::uriBack_}).level(), level);

			ASSERT_TRUE(commandInterface.errors().hasError({cubemap, &raco::user_types::CubeMap::uriBottom_}));
			ASSERT_EQ(commandInterface.errors().getError({cubemap, &raco::user_types::CubeMap::uriBottom_}).level(), level);

			ASSERT_TRUE(commandInterface.errors().hasError({cubemap, &raco::user_types::CubeMap::uriFront_}));
			ASSERT_EQ(commandInterface.errors().getError({cubemap, &raco::user_types::CubeMap::uriFront_}).level(), level);

			ASSERT_TRUE(commandInterface.errors().hasError({cubemap, &raco::user_types::CubeMap::uriLeft_}));
			ASSERT_EQ(commandInterface.errors().getError({cubemap, &raco::user_types::CubeMap::uriLeft_}).level(), level);

			ASSERT_TRUE(commandInterface.errors().hasError({cubemap, &raco::user_types::CubeMap::uriRight_}));
			ASSERT_EQ(commandInterface.errors().getError({cubemap, &raco::user_types::CubeMap::uriRight_}).level(), level);

			ASSERT_TRUE(commandInterface.errors().hasError({cubemap, &raco::user_types::CubeMap::uriTop_}));
			ASSERT_EQ(commandInterface.errors().getError({cubemap, &raco::user_types::CubeMap::uriTop_}).level(), level);
		}
	}

	void checkTextureFormats(raco::core::SEditorObject cubemap, const std::map<ramses::ETextureFormat, raco::core::ErrorLevel>& formats) {
		for (const auto& [format, level] : formats) {
			commandInterface.set({cubemap, &raco::user_types::CubeMap::textureFormat_}, static_cast<int>(format));
			dispatch();
			assertLevel1Errors(cubemap, level);
		}
	}
};

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

TEST_F(CubeMapAdaptorFixture, textureFormat8BitPalette) {
	auto cubemap = create<raco::user_types::CubeMap>("cubemap");

	commandInterface.set({cubemap, &raco::user_types::CubeMap::uriBack_}, (test_path() / "images" / "text-back-palette.png").string());
	commandInterface.set({cubemap, &raco::user_types::CubeMap::uriBottom_}, (test_path() / "images" / "text-back-palette.png").string());
	commandInterface.set({cubemap, &raco::user_types::CubeMap::uriFront_}, (test_path() / "images" / "text-back-palette.png").string());
	commandInterface.set({cubemap, &raco::user_types::CubeMap::uriLeft_}, (test_path() / "images" / "text-back-palette.png").string());
	commandInterface.set({cubemap, &raco::user_types::CubeMap::uriRight_}, (test_path() / "images" / "text-back-palette.png").string());
	commandInterface.set({cubemap, &raco::user_types::CubeMap::uriTop_}, (test_path() / "images" / "text-back-palette.png").string());
	dispatch();

	checkTextureFormats(cubemap, {{ramses::ETextureFormat::R8, raco::core::ErrorLevel::INFORMATION},
									 {ramses::ETextureFormat::RG8, raco::core::ErrorLevel::INFORMATION},
									 {ramses::ETextureFormat::RGB8, raco::core::ErrorLevel::INFORMATION},
									 {ramses::ETextureFormat::RGBA8, raco::core::ErrorLevel::INFORMATION},
									 {ramses::ETextureFormat::RGB16F, raco::core::ErrorLevel::ERROR},
									 {ramses::ETextureFormat::RGBA16F, raco::core::ErrorLevel::ERROR},
									 {ramses::ETextureFormat::SRGB8, raco::core::ErrorLevel::INFORMATION},
									 {ramses::ETextureFormat::SRGB8_ALPHA8, raco::core::ErrorLevel::INFORMATION}});
}

TEST_F(CubeMapAdaptorFixture, textureFormatR8) {
	auto cubemap = create<raco::user_types::CubeMap>("cubemap");

	commandInterface.set({cubemap, &raco::user_types::CubeMap::uriBack_}, (test_path() / "images" / "green_512_gray.png").string());
	commandInterface.set({cubemap, &raco::user_types::CubeMap::uriBottom_}, (test_path() / "images" / "green_512_gray.png").string());
	commandInterface.set({cubemap, &raco::user_types::CubeMap::uriFront_}, (test_path() / "images" / "green_512_gray.png").string());
	commandInterface.set({cubemap, &raco::user_types::CubeMap::uriLeft_}, (test_path() / "images" / "green_512_gray.png").string());
	commandInterface.set({cubemap, &raco::user_types::CubeMap::uriRight_}, (test_path() / "images" / "green_512_gray.png").string());
	commandInterface.set({cubemap, &raco::user_types::CubeMap::uriTop_}, (test_path() / "images" / "green_512_gray.png").string());
	dispatch();

	checkTextureFormats(cubemap, {{ramses::ETextureFormat::R8, raco::core::ErrorLevel::NONE},
									 {ramses::ETextureFormat::RG8, raco::core::ErrorLevel::WARNING},
									 {ramses::ETextureFormat::RGB8, raco::core::ErrorLevel::WARNING},
									 {ramses::ETextureFormat::RGBA8, raco::core::ErrorLevel::WARNING},
									 {ramses::ETextureFormat::RGB16F, raco::core::ErrorLevel::ERROR},
									 {ramses::ETextureFormat::RGBA16F, raco::core::ErrorLevel::ERROR},
									 {ramses::ETextureFormat::SRGB8, raco::core::ErrorLevel::WARNING},
									 {ramses::ETextureFormat::SRGB8_ALPHA8, raco::core::ErrorLevel::WARNING}});
}

TEST_F(CubeMapAdaptorFixture, textureFormatRG16) {
	auto cubemap = create<raco::user_types::CubeMap>("cubemap");

	commandInterface.set({cubemap, &raco::user_types::CubeMap::uriBack_}, (test_path() / "images" / "green_512_gray_16f.png").string());
	commandInterface.set({cubemap, &raco::user_types::CubeMap::uriBottom_}, (test_path() / "images" / "green_512_gray_16f.png").string());
	commandInterface.set({cubemap, &raco::user_types::CubeMap::uriFront_}, (test_path() / "images" / "green_512_gray_16f.png").string());
	commandInterface.set({cubemap, &raco::user_types::CubeMap::uriLeft_}, (test_path() / "images" / "green_512_gray_16f.png").string());
	commandInterface.set({cubemap, &raco::user_types::CubeMap::uriRight_}, (test_path() / "images" / "green_512_gray_16f.png").string());
	commandInterface.set({cubemap, &raco::user_types::CubeMap::uriTop_}, (test_path() / "images" / "green_512_gray_16f.png").string());
	dispatch();

	checkTextureFormats(cubemap, {{ramses::ETextureFormat::R8, raco::core::ErrorLevel::ERROR},
									 {ramses::ETextureFormat::RG8, raco::core::ErrorLevel::ERROR},
									 {ramses::ETextureFormat::RGB8, raco::core::ErrorLevel::ERROR},
									 {ramses::ETextureFormat::RGBA8, raco::core::ErrorLevel::ERROR},
									 {ramses::ETextureFormat::RGB16F, raco::core::ErrorLevel::ERROR},
									 {ramses::ETextureFormat::RGBA16F, raco::core::ErrorLevel::ERROR},
									 {ramses::ETextureFormat::SRGB8, raco::core::ErrorLevel::ERROR},
									 {ramses::ETextureFormat::SRGB8_ALPHA8, raco::core::ErrorLevel::ERROR}});
}

TEST_F(CubeMapAdaptorFixture, textureFormatRG8) {
	auto cubemap = create<raco::user_types::CubeMap>("cubemap");

	commandInterface.set({cubemap, &raco::user_types::CubeMap::uriBack_}, (test_path() / "images" / "green_512_gray_alpha.png").string());
	commandInterface.set({cubemap, &raco::user_types::CubeMap::uriBottom_}, (test_path() / "images" / "green_512_gray_alpha.png").string());
	commandInterface.set({cubemap, &raco::user_types::CubeMap::uriFront_}, (test_path() / "images" / "green_512_gray_alpha.png").string());
	commandInterface.set({cubemap, &raco::user_types::CubeMap::uriLeft_}, (test_path() / "images" / "green_512_gray_alpha.png").string());
	commandInterface.set({cubemap, &raco::user_types::CubeMap::uriRight_}, (test_path() / "images" / "green_512_gray_alpha.png").string());
	commandInterface.set({cubemap, &raco::user_types::CubeMap::uriTop_}, (test_path() / "images" / "green_512_gray_alpha.png").string());
	dispatch();

	checkTextureFormats(cubemap, {{ramses::ETextureFormat::R8, raco::core::ErrorLevel::INFORMATION},
									 {ramses::ETextureFormat::RG8, raco::core::ErrorLevel::NONE},
									 {ramses::ETextureFormat::RGB8, raco::core::ErrorLevel::WARNING},
									 {ramses::ETextureFormat::RGBA8, raco::core::ErrorLevel::WARNING},
									 {ramses::ETextureFormat::RGB16F, raco::core::ErrorLevel::ERROR},
									 {ramses::ETextureFormat::RGBA16F, raco::core::ErrorLevel::ERROR},
									 {ramses::ETextureFormat::SRGB8, raco::core::ErrorLevel::WARNING},
									 {ramses::ETextureFormat::SRGB8_ALPHA8, raco::core::ErrorLevel::WARNING}});
}

TEST_F(CubeMapAdaptorFixture, textureFormatRGB8) {
	auto cubemap = create<raco::user_types::CubeMap>("cubemap");

	commandInterface.set({cubemap, &raco::user_types::CubeMap::uriBack_}, (test_path() / "images" / "text-back.png").string());
	commandInterface.set({cubemap, &raco::user_types::CubeMap::uriBottom_}, (test_path() / "images" / "text-back.png").string());
	commandInterface.set({cubemap, &raco::user_types::CubeMap::uriFront_}, (test_path() / "images" / "text-back.png").string());
	commandInterface.set({cubemap, &raco::user_types::CubeMap::uriLeft_}, (test_path() / "images" / "text-back.png").string());
	commandInterface.set({cubemap, &raco::user_types::CubeMap::uriRight_}, (test_path() / "images" / "text-back.png").string());
	commandInterface.set({cubemap, &raco::user_types::CubeMap::uriTop_}, (test_path() / "images" / "text-back.png").string());
	dispatch();

	checkTextureFormats(cubemap, {{ramses::ETextureFormat::R8, raco::core::ErrorLevel::INFORMATION},
									 {ramses::ETextureFormat::RG8, raco::core::ErrorLevel::INFORMATION},
									 {ramses::ETextureFormat::RGB8, raco::core::ErrorLevel::NONE},
									 {ramses::ETextureFormat::RGBA8, raco::core::ErrorLevel::WARNING},
									 {ramses::ETextureFormat::RGB16F, raco::core::ErrorLevel::ERROR},
									 {ramses::ETextureFormat::RGBA16F, raco::core::ErrorLevel::ERROR},
									 {ramses::ETextureFormat::SRGB8, raco::core::ErrorLevel::NONE},
									 {ramses::ETextureFormat::SRGB8_ALPHA8, raco::core::ErrorLevel::WARNING}});
}

TEST_F(CubeMapAdaptorFixture, textureFormatRGBA8) {
	auto cubemap = create<raco::user_types::CubeMap>("cubemap");

	commandInterface.set({cubemap, &raco::user_types::CubeMap::uriBack_}, (test_path() / "images" / "green_512.png").string());
	commandInterface.set({cubemap, &raco::user_types::CubeMap::uriBottom_}, (test_path() / "images" / "green_512.png").string());
	commandInterface.set({cubemap, &raco::user_types::CubeMap::uriFront_}, (test_path() / "images" / "green_512.png").string());
	commandInterface.set({cubemap, &raco::user_types::CubeMap::uriLeft_}, (test_path() / "images" / "green_512.png").string());
	commandInterface.set({cubemap, &raco::user_types::CubeMap::uriRight_}, (test_path() / "images" / "green_512.png").string());
	commandInterface.set({cubemap, &raco::user_types::CubeMap::uriTop_}, (test_path() / "images" / "green_512.png").string());
	dispatch();

	checkTextureFormats(cubemap, {{ramses::ETextureFormat::R8, raco::core::ErrorLevel::INFORMATION},
									 {ramses::ETextureFormat::RG8, raco::core::ErrorLevel::INFORMATION},
									 {ramses::ETextureFormat::RGB8, raco::core::ErrorLevel::INFORMATION},
									 {ramses::ETextureFormat::RGBA8, raco::core::ErrorLevel::NONE},
									 {ramses::ETextureFormat::RGB16F, raco::core::ErrorLevel::ERROR},
									 {ramses::ETextureFormat::RGBA16F, raco::core::ErrorLevel::ERROR},
									 {ramses::ETextureFormat::SRGB8, raco::core::ErrorLevel::INFORMATION},
									 {ramses::ETextureFormat::SRGB8_ALPHA8, raco::core::ErrorLevel::NONE}});
}

TEST_F(CubeMapAdaptorFixture, textureFormatRGB16) {
	auto cubemap = create<raco::user_types::CubeMap>("cubemap");

	commandInterface.set({cubemap, &raco::user_types::CubeMap::uriBack_}, (test_path() / "images" / "green_512_16f_no_alpha.png").string());
	commandInterface.set({cubemap, &raco::user_types::CubeMap::uriBottom_}, (test_path() / "images" / "green_512_16f_no_alpha.png").string());
	commandInterface.set({cubemap, &raco::user_types::CubeMap::uriFront_}, (test_path() / "images" / "green_512_16f_no_alpha.png").string());
	commandInterface.set({cubemap, &raco::user_types::CubeMap::uriLeft_}, (test_path() / "images" / "green_512_16f_no_alpha.png").string());
	commandInterface.set({cubemap, &raco::user_types::CubeMap::uriRight_}, (test_path() / "images" / "green_512_16f_no_alpha.png").string());
	commandInterface.set({cubemap, &raco::user_types::CubeMap::uriTop_}, (test_path() / "images" / "green_512_16f_no_alpha.png").string());
	dispatch();

	checkTextureFormats(cubemap, {{ramses::ETextureFormat::R8, raco::core::ErrorLevel::ERROR},
									 {ramses::ETextureFormat::RG8, raco::core::ErrorLevel::ERROR},
									 {ramses::ETextureFormat::RGB8, raco::core::ErrorLevel::ERROR},
									 {ramses::ETextureFormat::RGBA8, raco::core::ErrorLevel::ERROR},
									 {ramses::ETextureFormat::RGB16F, raco::core::ErrorLevel::NONE},
									 {ramses::ETextureFormat::RGBA16F, raco::core::ErrorLevel::WARNING},
									 {ramses::ETextureFormat::SRGB8, raco::core::ErrorLevel::ERROR},
									 {ramses::ETextureFormat::SRGB8_ALPHA8, raco::core::ErrorLevel::ERROR}});
}

TEST_F(CubeMapAdaptorFixture, textureFormatRGBA16From16i) {
	auto cubemap = create<raco::user_types::CubeMap>("cubemap");

	commandInterface.set({cubemap, &raco::user_types::CubeMap::uriBack_}, (test_path() / "images" / "green_512_16i.png").string());
	commandInterface.set({cubemap, &raco::user_types::CubeMap::uriBottom_}, (test_path() / "images" / "green_512_16i.png").string());
	commandInterface.set({cubemap, &raco::user_types::CubeMap::uriFront_}, (test_path() / "images" / "green_512_16i.png").string());
	commandInterface.set({cubemap, &raco::user_types::CubeMap::uriLeft_}, (test_path() / "images" / "green_512_16i.png").string());
	commandInterface.set({cubemap, &raco::user_types::CubeMap::uriRight_}, (test_path() / "images" / "green_512_16i.png").string());
	commandInterface.set({cubemap, &raco::user_types::CubeMap::uriTop_}, (test_path() / "images" / "green_512_16i.png").string());
	dispatch();

	checkTextureFormats(cubemap, {{ramses::ETextureFormat::R8, raco::core::ErrorLevel::ERROR},
									 {ramses::ETextureFormat::RG8, raco::core::ErrorLevel::ERROR},
									 {ramses::ETextureFormat::RGB8, raco::core::ErrorLevel::ERROR},
									 {ramses::ETextureFormat::RGBA8, raco::core::ErrorLevel::ERROR},
									 {ramses::ETextureFormat::RGB16F, raco::core::ErrorLevel::INFORMATION},
									 {ramses::ETextureFormat::RGBA16F, raco::core::ErrorLevel::NONE},
									 {ramses::ETextureFormat::SRGB8, raco::core::ErrorLevel::ERROR},
									 {ramses::ETextureFormat::SRGB8_ALPHA8, raco::core::ErrorLevel::ERROR}});
}

TEST_F(CubeMapAdaptorFixture, textureFormatRGBA16From16f) {
	auto cubemap = create<raco::user_types::CubeMap>("cubemap");

	commandInterface.set({cubemap, &raco::user_types::CubeMap::uriBack_}, (test_path() / "images" / "green_512_16f.png").string());
	commandInterface.set({cubemap, &raco::user_types::CubeMap::uriBottom_}, (test_path() / "images" / "green_512_16f.png").string());
	commandInterface.set({cubemap, &raco::user_types::CubeMap::uriFront_}, (test_path() / "images" / "green_512_16f.png").string());
	commandInterface.set({cubemap, &raco::user_types::CubeMap::uriLeft_}, (test_path() / "images" / "green_512_16f.png").string());
	commandInterface.set({cubemap, &raco::user_types::CubeMap::uriRight_}, (test_path() / "images" / "green_512_16f.png").string());
	commandInterface.set({cubemap, &raco::user_types::CubeMap::uriTop_}, (test_path() / "images" / "green_512_16f.png").string());
	dispatch();

	checkTextureFormats(cubemap, {{ramses::ETextureFormat::R8, raco::core::ErrorLevel::ERROR},
									 {ramses::ETextureFormat::RG8, raco::core::ErrorLevel::ERROR},
									 {ramses::ETextureFormat::RGB8, raco::core::ErrorLevel::ERROR},
									 {ramses::ETextureFormat::RGBA8, raco::core::ErrorLevel::ERROR},
									 {ramses::ETextureFormat::RGB16F, raco::core::ErrorLevel::INFORMATION},
									 {ramses::ETextureFormat::RGBA16F, raco::core::ErrorLevel::NONE},
									 {ramses::ETextureFormat::SRGB8, raco::core::ErrorLevel::ERROR},
									 {ramses::ETextureFormat::SRGB8_ALPHA8, raco::core::ErrorLevel::ERROR}});
}

TEST_F(CubeMapAdaptorFixture, textureFormatChangeValidToInvalid) {
	auto cubemap = create<raco::user_types::CubeMap>("cubemap");

	commandInterface.set({cubemap, &raco::user_types::CubeMap::uriBack_}, (test_path() / "images" / "green_512_16f.png").string());
	commandInterface.set({cubemap, &raco::user_types::CubeMap::uriBottom_}, (test_path() / "images" / "green_512_16f.png").string());
	commandInterface.set({cubemap, &raco::user_types::CubeMap::uriFront_}, (test_path() / "images" / "green_512_16f.png").string());
	commandInterface.set({cubemap, &raco::user_types::CubeMap::uriLeft_}, (test_path() / "images" / "green_512_16f.png").string());
	commandInterface.set({cubemap, &raco::user_types::CubeMap::uriRight_}, (test_path() / "images" / "green_512_16f.png").string());
	commandInterface.set({cubemap, &raco::user_types::CubeMap::uriTop_}, (test_path() / "images" / "green_512_16f.png").string());
	dispatch();

	// RGBA16
	commandInterface.set({cubemap, &raco::user_types::CubeMap::textureFormat_}, static_cast<int>(ramses::ETextureFormat::RGBA16F));
	dispatch();
	assertLevel1Errors(cubemap, raco::core::ErrorLevel::NONE);

	commandInterface.set({cubemap, &raco::user_types::CubeMap::uriBack_}, (test_path() / "images" / "green_512.png").string());
	commandInterface.set({cubemap, &raco::user_types::CubeMap::uriBottom_}, (test_path() / "images" / "green_512.png").string());
	commandInterface.set({cubemap, &raco::user_types::CubeMap::uriFront_}, (test_path() / "images" / "green_512.png").string());
	commandInterface.set({cubemap, &raco::user_types::CubeMap::uriLeft_}, (test_path() / "images" / "green_512.png").string());
	commandInterface.set({cubemap, &raco::user_types::CubeMap::uriRight_}, (test_path() / "images" / "green_512.png").string());
	commandInterface.set({cubemap, &raco::user_types::CubeMap::uriTop_}, (test_path() / "images" / "green_512.png").string());
	dispatch();
	assertLevel1Errors(cubemap, raco::core::ErrorLevel::ERROR);

	// R8
	commandInterface.set({cubemap, &raco::user_types::CubeMap::textureFormat_}, static_cast<int>(ramses::ETextureFormat::R8));
	dispatch();
	assertLevel1Errors(cubemap, raco::core::ErrorLevel::INFORMATION);

	commandInterface.set({cubemap, &raco::user_types::CubeMap::uriBack_}, (test_path() / "images" / "green_512_16f.png").string());
	commandInterface.set({cubemap, &raco::user_types::CubeMap::uriBottom_}, (test_path() / "images" / "green_512_16f.png").string());
	commandInterface.set({cubemap, &raco::user_types::CubeMap::uriFront_}, (test_path() / "images" / "green_512_16f.png").string());
	commandInterface.set({cubemap, &raco::user_types::CubeMap::uriLeft_}, (test_path() / "images" / "green_512_16f.png").string());
	commandInterface.set({cubemap, &raco::user_types::CubeMap::uriRight_}, (test_path() / "images" / "green_512_16f.png").string());
	commandInterface.set({cubemap, &raco::user_types::CubeMap::uriTop_}, (test_path() / "images" / "green_512_16f.png").string());
	dispatch();
	assertLevel1Errors(cubemap, raco::core::ErrorLevel::ERROR);

	// RGBA16
	commandInterface.set({cubemap, &raco::user_types::CubeMap::textureFormat_}, static_cast<int>(ramses::ETextureFormat::RGBA16F));
	dispatch();
	assertLevel1Errors(cubemap, raco::core::ErrorLevel::NONE);
}


TEST_F(CubeMapAdaptorFixture, textureBitdepthDifferentInSameLevel) {
	auto cubemap = create<raco::user_types::CubeMap>("cubemap");

	commandInterface.set({cubemap, &raco::user_types::CubeMap::uriBack_}, (test_path() / "images" / "green_512_16f.png").string());
	commandInterface.set({cubemap, &raco::user_types::CubeMap::uriBottom_}, (test_path() / "images" / "green_512_16f.png").string());
	commandInterface.set({cubemap, &raco::user_types::CubeMap::uriFront_}, (test_path() / "images" / "green_512_16f.png").string());
	commandInterface.set({cubemap, &raco::user_types::CubeMap::uriLeft_}, (test_path() / "images" / "green_512_16f.png").string());
	commandInterface.set({cubemap, &raco::user_types::CubeMap::uriRight_}, (test_path() / "images" / "green_512_16f.png").string());
	commandInterface.set({cubemap, &raco::user_types::CubeMap::uriTop_}, (test_path() / "images" / "green_512_16f.png").string());
	dispatch();

	// RGBA16
	commandInterface.set({cubemap, &raco::user_types::CubeMap::textureFormat_}, static_cast<int>(ramses::ETextureFormat::RGBA16F));
	dispatch();
	ASSERT_FALSE(commandInterface.errors().hasError({cubemap, &raco::user_types::CubeMap::textureFormat_}));

	commandInterface.set({cubemap, &raco::user_types::CubeMap::uriFront_}, (test_path() / "images" / "green_512.png").string());
	dispatch();
	ASSERT_TRUE(commandInterface.errors().hasError({cubemap, &raco::user_types::CubeMap::uriFront_}));
	ASSERT_EQ(commandInterface.errors().getError({cubemap, &raco::user_types::CubeMap::uriFront_}).level(), raco::core::ErrorLevel::ERROR);

	commandInterface.set({cubemap, &raco::user_types::CubeMap::uriFront_}, (test_path() / "images" / "green_512_16f.png").string());
	dispatch();
	ASSERT_FALSE(commandInterface.errors().hasError({cubemap, &raco::user_types::CubeMap::uriFront_}));
}

TEST_F(CubeMapAdaptorFixture, textureBitdepthDifferentInOtherLevel) {
	auto cubemap = create<raco::user_types::CubeMap>("cubemap");

	commandInterface.set({cubemap, &raco::user_types::CubeMap::uriBack_}, (test_path() / "images" / "blue_1024_16i.png").string());
	commandInterface.set({cubemap, &raco::user_types::CubeMap::uriBottom_}, (test_path() / "images" / "blue_1024_16i.png").string());
	commandInterface.set({cubemap, &raco::user_types::CubeMap::uriFront_}, (test_path() / "images" / "blue_1024_16i.png").string());
	commandInterface.set({cubemap, &raco::user_types::CubeMap::uriLeft_}, (test_path() / "images" / "blue_1024_16i.png").string());
	commandInterface.set({cubemap, &raco::user_types::CubeMap::uriRight_}, (test_path() / "images" / "blue_1024_16i.png").string());
	commandInterface.set({cubemap, &raco::user_types::CubeMap::uriTop_}, (test_path() / "images" / "blue_1024_16i.png").string());
	dispatch();

	// RGBA16
	commandInterface.set({cubemap, &raco::user_types::CubeMap::textureFormat_}, static_cast<int>(ramses::ETextureFormat::RGBA16F));
	dispatch();
	ASSERT_FALSE(commandInterface.errors().hasError({cubemap, &raco::user_types::CubeMap::textureFormat_}));

	commandInterface.set({cubemap, &raco::user_types::CubeMap::level2uriBack_}, (test_path() / "images" / "green_512.png").string());
	commandInterface.set({cubemap, &raco::user_types::CubeMap::level2uriBottom_}, (test_path() / "images" / "green_512.png").string());
	commandInterface.set({cubemap, &raco::user_types::CubeMap::level2uriFront_}, (test_path() / "images" / "green_512.png").string());
	commandInterface.set({cubemap, &raco::user_types::CubeMap::level2uriLeft_}, (test_path() / "images" / "green_512.png").string());
	commandInterface.set({cubemap, &raco::user_types::CubeMap::level2uriRight_}, (test_path() / "images" / "green_512.png").string());
	commandInterface.set({cubemap, &raco::user_types::CubeMap::level2uriTop_}, (test_path() / "images" / "green_512.png").string());
	commandInterface.set({cubemap, &raco::user_types::CubeMap::mipmapLevel_}, 2);
	dispatch();
	ASSERT_TRUE(commandInterface.errors().hasError({cubemap, &raco::user_types::CubeMap::level2uriBack_}));
	ASSERT_EQ(commandInterface.errors().getError({cubemap, &raco::user_types::CubeMap::level2uriBack_}).level(), raco::core::ErrorLevel::ERROR);

	ASSERT_TRUE(commandInterface.errors().hasError({cubemap, &raco::user_types::CubeMap::level2uriBottom_}));
	ASSERT_EQ(commandInterface.errors().getError({cubemap, &raco::user_types::CubeMap::level2uriBottom_}).level(), raco::core::ErrorLevel::ERROR);

	ASSERT_TRUE(commandInterface.errors().hasError({cubemap, &raco::user_types::CubeMap::level2uriFront_}));
	ASSERT_EQ(commandInterface.errors().getError({cubemap, &raco::user_types::CubeMap::level2uriFront_}).level(), raco::core::ErrorLevel::ERROR);

	ASSERT_TRUE(commandInterface.errors().hasError({cubemap, &raco::user_types::CubeMap::level2uriLeft_}));
	ASSERT_EQ(commandInterface.errors().getError({cubemap, &raco::user_types::CubeMap::level2uriLeft_}).level(), raco::core::ErrorLevel::ERROR);

	ASSERT_TRUE(commandInterface.errors().hasError({cubemap, &raco::user_types::CubeMap::level2uriRight_}));
	ASSERT_EQ(commandInterface.errors().getError({cubemap, &raco::user_types::CubeMap::level2uriRight_}).level(), raco::core::ErrorLevel::ERROR);

	ASSERT_TRUE(commandInterface.errors().hasError({cubemap, &raco::user_types::CubeMap::level2uriTop_}));
	ASSERT_EQ(commandInterface.errors().getError({cubemap, &raco::user_types::CubeMap::level2uriTop_}).level(), raco::core::ErrorLevel::ERROR);

	commandInterface.set({cubemap, &raco::user_types::CubeMap::level2uriBack_}, (test_path() / "images" / "green_512_16i.png").string());
	commandInterface.set({cubemap, &raco::user_types::CubeMap::level2uriBottom_}, (test_path() / "images" / "green_512_16i.png").string());
	commandInterface.set({cubemap, &raco::user_types::CubeMap::level2uriFront_}, (test_path() / "images" / "green_512_16i.png").string());
	commandInterface.set({cubemap, &raco::user_types::CubeMap::level2uriLeft_}, (test_path() / "images" / "green_512_16i.png").string());
	commandInterface.set({cubemap, &raco::user_types::CubeMap::level2uriRight_}, (test_path() / "images" / "green_512_16i.png").string());
	commandInterface.set({cubemap, &raco::user_types::CubeMap::level2uriTop_}, (test_path() / "images" / "green_512_16i.png").string());
	dispatch();
	ASSERT_FALSE(commandInterface.errors().hasError({cubemap, &raco::user_types::CubeMap::level2uriBack_}));
	ASSERT_FALSE(commandInterface.errors().hasError({cubemap, &raco::user_types::CubeMap::level2uriBottom_}));
	ASSERT_FALSE(commandInterface.errors().hasError({cubemap, &raco::user_types::CubeMap::level2uriFront_}));
	ASSERT_FALSE(commandInterface.errors().hasError({cubemap, &raco::user_types::CubeMap::level2uriLeft_}));
	ASSERT_FALSE(commandInterface.errors().hasError({cubemap, &raco::user_types::CubeMap::level2uriRight_}));
	ASSERT_FALSE(commandInterface.errors().hasError({cubemap, &raco::user_types::CubeMap::level2uriTop_}));
}

TEST_F(CubeMapAdaptorFixture, textureFormatDifferentInSameLevel) {
	auto cubemap = create<raco::user_types::CubeMap>("cubemap");

	commandInterface.set({cubemap, &raco::user_types::CubeMap::textureFormat_}, static_cast<int>(ramses::ETextureFormat::RGBA8));
	dispatch();

	commandInterface.set({cubemap, &raco::user_types::CubeMap::uriBack_}, (test_path() / "images" / "green_512.png").string());
	commandInterface.set({cubemap, &raco::user_types::CubeMap::uriBottom_}, (test_path() / "images" / "green_512.png").string());
	commandInterface.set({cubemap, &raco::user_types::CubeMap::uriFront_}, (test_path() / "images" / "green_512.png").string());
	commandInterface.set({cubemap, &raco::user_types::CubeMap::uriLeft_}, (test_path() / "images" / "green_512.png").string());
	commandInterface.set({cubemap, &raco::user_types::CubeMap::uriRight_}, (test_path() / "images" / "green_512.png").string());
	commandInterface.set({cubemap, &raco::user_types::CubeMap::uriTop_}, (test_path() / "images" / "green_512.png").string());
	dispatch();

	commandInterface.set({cubemap, &raco::user_types::CubeMap::uriFront_}, (test_path() / "images" / "green_512_gray.png").string());
	dispatch();
	ASSERT_TRUE(commandInterface.errors().hasError({cubemap, &raco::user_types::CubeMap::uriFront_}));
	ASSERT_EQ(commandInterface.errors().getError({cubemap, &raco::user_types::CubeMap::uriFront_}).level(), raco::core::ErrorLevel::WARNING);

	commandInterface.set({cubemap, &raco::user_types::CubeMap::uriFront_}, (test_path() / "images" / "green_512.png").string());
	dispatch();
	ASSERT_FALSE(commandInterface.errors().hasError({cubemap, &raco::user_types::CubeMap::uriFront_}));
}

TEST_F(CubeMapAdaptorFixture, textureFormatDifferentInOtherLevel) {
	auto cubemap = create<raco::user_types::CubeMap>("cubemap");

	commandInterface.set({cubemap, &raco::user_types::CubeMap::uriBack_}, (test_path() / "images" / "blue_1024.png").string());
	commandInterface.set({cubemap, &raco::user_types::CubeMap::uriBottom_}, (test_path() / "images" / "blue_1024.png").string());
	commandInterface.set({cubemap, &raco::user_types::CubeMap::uriFront_}, (test_path() / "images" / "blue_1024.png").string());
	commandInterface.set({cubemap, &raco::user_types::CubeMap::uriLeft_}, (test_path() / "images" / "blue_1024.png").string());
	commandInterface.set({cubemap, &raco::user_types::CubeMap::uriRight_}, (test_path() / "images" / "blue_1024.png").string());
	commandInterface.set({cubemap, &raco::user_types::CubeMap::uriTop_}, (test_path() / "images" / "blue_1024.png").string());
	dispatch();

	// RGBA8
	commandInterface.set({cubemap, &raco::user_types::CubeMap::textureFormat_}, static_cast<int>(ramses::ETextureFormat::RGBA8));
	dispatch();

	commandInterface.set({cubemap, &raco::user_types::CubeMap::level2uriBack_}, (test_path() / "images" / "green_512_gray.png").string());
	commandInterface.set({cubemap, &raco::user_types::CubeMap::level2uriBottom_}, (test_path() / "images" / "green_512_gray.png").string());
	commandInterface.set({cubemap, &raco::user_types::CubeMap::level2uriFront_}, (test_path() / "images" / "green_512_gray.png").string());
	commandInterface.set({cubemap, &raco::user_types::CubeMap::level2uriLeft_}, (test_path() / "images" / "green_512_gray.png").string());
	commandInterface.set({cubemap, &raco::user_types::CubeMap::level2uriRight_}, (test_path() / "images" / "green_512_gray.png").string());
	commandInterface.set({cubemap, &raco::user_types::CubeMap::level2uriTop_}, (test_path() / "images" / "green_512_gray.png").string());
	commandInterface.set({cubemap, &raco::user_types::CubeMap::mipmapLevel_}, 2);
	dispatch();
	ASSERT_TRUE(commandInterface.errors().hasError({cubemap, &raco::user_types::CubeMap::level2uriBack_}));
	ASSERT_EQ(commandInterface.errors().getError({cubemap, &raco::user_types::CubeMap::level2uriBack_}).level(), raco::core::ErrorLevel::WARNING);

	ASSERT_TRUE(commandInterface.errors().hasError({cubemap, &raco::user_types::CubeMap::level2uriBottom_}));
	ASSERT_EQ(commandInterface.errors().getError({cubemap, &raco::user_types::CubeMap::level2uriBottom_}).level(), raco::core::ErrorLevel::WARNING);

	ASSERT_TRUE(commandInterface.errors().hasError({cubemap, &raco::user_types::CubeMap::level2uriFront_}));
	ASSERT_EQ(commandInterface.errors().getError({cubemap, &raco::user_types::CubeMap::level2uriFront_}).level(), raco::core::ErrorLevel::WARNING);

	ASSERT_TRUE(commandInterface.errors().hasError({cubemap, &raco::user_types::CubeMap::level2uriLeft_}));
	ASSERT_EQ(commandInterface.errors().getError({cubemap, &raco::user_types::CubeMap::level2uriLeft_}).level(), raco::core::ErrorLevel::WARNING);

	ASSERT_TRUE(commandInterface.errors().hasError({cubemap, &raco::user_types::CubeMap::level2uriRight_}));
	ASSERT_EQ(commandInterface.errors().getError({cubemap, &raco::user_types::CubeMap::level2uriRight_}).level(), raco::core::ErrorLevel::WARNING);

	ASSERT_TRUE(commandInterface.errors().hasError({cubemap, &raco::user_types::CubeMap::level2uriTop_}));
	ASSERT_EQ(commandInterface.errors().getError({cubemap, &raco::user_types::CubeMap::level2uriTop_}).level(), raco::core::ErrorLevel::WARNING);

	commandInterface.set({cubemap, &raco::user_types::CubeMap::level2uriBack_}, (test_path() / "images" / "green_512.png").string());
	commandInterface.set({cubemap, &raco::user_types::CubeMap::level2uriBottom_}, (test_path() / "images" / "green_512.png").string());
	commandInterface.set({cubemap, &raco::user_types::CubeMap::level2uriFront_}, (test_path() / "images" / "green_512.png").string());
	commandInterface.set({cubemap, &raco::user_types::CubeMap::level2uriLeft_}, (test_path() / "images" / "green_512.png").string());
	commandInterface.set({cubemap, &raco::user_types::CubeMap::level2uriRight_}, (test_path() / "images" / "green_512.png").string());
	commandInterface.set({cubemap, &raco::user_types::CubeMap::level2uriTop_}, (test_path() / "images" / "green_512.png").string());
	dispatch();
	ASSERT_FALSE(commandInterface.errors().hasError({cubemap, &raco::user_types::CubeMap::level2uriBack_}));
	ASSERT_FALSE(commandInterface.errors().hasError({cubemap, &raco::user_types::CubeMap::level2uriBottom_}));
	ASSERT_FALSE(commandInterface.errors().hasError({cubemap, &raco::user_types::CubeMap::level2uriFront_}));
	ASSERT_FALSE(commandInterface.errors().hasError({cubemap, &raco::user_types::CubeMap::level2uriLeft_}));
	ASSERT_FALSE(commandInterface.errors().hasError({cubemap, &raco::user_types::CubeMap::level2uriRight_}));
	ASSERT_FALSE(commandInterface.errors().hasError({cubemap, &raco::user_types::CubeMap::level2uriTop_}));
}