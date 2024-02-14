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
#include "ramses_adaptor/CubeMapAdaptor.h"
#include "user_types/Enumerations.h"

using user_types::ETextureFormat;

class CubeMapAdaptorFixture : public RamsesBaseFixture<> {
public:
	void assertLevel1Errors(core::SEditorObject cubemap, core::ErrorLevel level) {
		if (level == core::ErrorLevel::NONE) {
			ASSERT_FALSE(commandInterface.errors().hasError({cubemap, &user_types::CubeMap::uriBack_}));
			ASSERT_FALSE(commandInterface.errors().hasError({cubemap, &user_types::CubeMap::uriBottom_}));
			ASSERT_FALSE(commandInterface.errors().hasError({cubemap, &user_types::CubeMap::uriFront_}));
			ASSERT_FALSE(commandInterface.errors().hasError({cubemap, &user_types::CubeMap::uriLeft_}));
			ASSERT_FALSE(commandInterface.errors().hasError({cubemap, &user_types::CubeMap::uriRight_}));
			ASSERT_FALSE(commandInterface.errors().hasError({cubemap, &user_types::CubeMap::uriTop_}));

		} else {
			ASSERT_TRUE(commandInterface.errors().hasError({cubemap, &user_types::CubeMap::uriBack_}));
			ASSERT_EQ(commandInterface.errors().getError({cubemap, &user_types::CubeMap::uriBack_}).level(), level);

			ASSERT_TRUE(commandInterface.errors().hasError({cubemap, &user_types::CubeMap::uriBottom_}));
			ASSERT_EQ(commandInterface.errors().getError({cubemap, &user_types::CubeMap::uriBottom_}).level(), level);

			ASSERT_TRUE(commandInterface.errors().hasError({cubemap, &user_types::CubeMap::uriFront_}));
			ASSERT_EQ(commandInterface.errors().getError({cubemap, &user_types::CubeMap::uriFront_}).level(), level);

			ASSERT_TRUE(commandInterface.errors().hasError({cubemap, &user_types::CubeMap::uriLeft_}));
			ASSERT_EQ(commandInterface.errors().getError({cubemap, &user_types::CubeMap::uriLeft_}).level(), level);

			ASSERT_TRUE(commandInterface.errors().hasError({cubemap, &user_types::CubeMap::uriRight_}));
			ASSERT_EQ(commandInterface.errors().getError({cubemap, &user_types::CubeMap::uriRight_}).level(), level);

			ASSERT_TRUE(commandInterface.errors().hasError({cubemap, &user_types::CubeMap::uriTop_}));
			ASSERT_EQ(commandInterface.errors().getError({cubemap, &user_types::CubeMap::uriTop_}).level(), level);
		}
	}

	void checkTextureFormats(core::SEditorObject cubemap, const std::map<ETextureFormat, core::ErrorLevel>& formats) {
		for (const auto& [format, level] : formats) {
			commandInterface.set({cubemap, &user_types::CubeMap::textureFormat_}, static_cast<int>(format));
			dispatch();
			assertLevel1Errors(cubemap, level);
		}
	}
};

TEST_F(CubeMapAdaptorFixture, cubeMapGenerationAtMultipleLevels) {
	auto cubeMap = create<user_types::CubeMap>("Cubemap");

	commandInterface.set({cubeMap, &user_types::CubeMap::uriBack_}, (test_path() / "images" / "blue_1024.png").string());
	commandInterface.set({cubeMap, &user_types::CubeMap::uriBottom_}, (test_path() / "images" / "blue_1024.png").string());
	commandInterface.set({cubeMap, &user_types::CubeMap::uriFront_}, (test_path() / "images" / "blue_1024.png").string());
	commandInterface.set({cubeMap, &user_types::CubeMap::uriLeft_}, (test_path() / "images" / "blue_1024.png").string());
	commandInterface.set({cubeMap, &user_types::CubeMap::uriRight_}, (test_path() / "images" / "blue_1024.png").string());
	commandInterface.set({cubeMap, &user_types::CubeMap::uriTop_}, (test_path() / "images" / "blue_1024.png").string());

	dispatch();

	commandInterface.set({cubeMap, &user_types::CubeMap::level2uriBack_}, (test_path() / "images" / "green_512.png").string());
	commandInterface.set({cubeMap, &user_types::CubeMap::level2uriBottom_}, (test_path() / "images" / "green_512.png").string());
	commandInterface.set({cubeMap, &user_types::CubeMap::level2uriFront_}, (test_path() / "images" / "green_512.png").string());
	commandInterface.set({cubeMap, &user_types::CubeMap::level2uriLeft_}, (test_path() / "images" / "green_512.png").string());
	commandInterface.set({cubeMap, &user_types::CubeMap::level2uriRight_}, (test_path() / "images" / "green_512.png").string());
	commandInterface.set({cubeMap, &user_types::CubeMap::level2uriTop_}, (test_path() / "images" / "green_512.png").string());

	dispatch();

	commandInterface.set({cubeMap, &user_types::CubeMap::level3uriBack_}, (test_path() / "images" / "yellow_256.png").string());
	commandInterface.set({cubeMap, &user_types::CubeMap::level3uriBottom_}, (test_path() / "images" / "yellow_256.png").string());
	commandInterface.set({cubeMap, &user_types::CubeMap::level3uriFront_}, (test_path() / "images" / "yellow_256.png").string());
	commandInterface.set({cubeMap, &user_types::CubeMap::level3uriLeft_}, (test_path() / "images" / "yellow_256.png").string());
	commandInterface.set({cubeMap, &user_types::CubeMap::level3uriRight_}, (test_path() / "images" / "yellow_256.png").string());
	commandInterface.set({cubeMap, &user_types::CubeMap::level3uriTop_}, (test_path() / "images" / "yellow_256.png").string());

	dispatch();

	commandInterface.set({cubeMap, &user_types::CubeMap::level4uriBack_}, (test_path() / "images" / "red_128.png").string());
	commandInterface.set({cubeMap, &user_types::CubeMap::level4uriBottom_}, (test_path() / "images" / "red_128.png").string());
	commandInterface.set({cubeMap, &user_types::CubeMap::level4uriFront_}, (test_path() / "images" / "red_128.png").string());
	commandInterface.set({cubeMap, &user_types::CubeMap::level4uriLeft_}, (test_path() / "images" / "red_128.png").string());
	commandInterface.set({cubeMap, &user_types::CubeMap::level4uriRight_}, (test_path() / "images" / "red_128.png").string());
	commandInterface.set({cubeMap, &user_types::CubeMap::level4uriTop_}, (test_path() / "images" / "red_128.png").string());

	dispatch();

	auto cubeMapStuff{select<ramses::TextureSampler>(*sceneContext.scene(), ramses::ERamsesObjectType::TextureSampler)};
	ASSERT_EQ(cubeMapStuff.size(), 1);

	commandInterface.set({cubeMap, &user_types::CubeMap::mipmapLevel_}, 2);
	dispatch();
	cubeMapStuff = select<ramses::TextureSampler>(*sceneContext.scene(), ramses::ERamsesObjectType::TextureSampler);
	ASSERT_EQ(cubeMapStuff.size(), 1);

	commandInterface.set({cubeMap, &user_types::CubeMap::mipmapLevel_}, 3);
	dispatch();
	cubeMapStuff = select<ramses::TextureSampler>(*sceneContext.scene(), ramses::ERamsesObjectType::TextureSampler);
	ASSERT_EQ(cubeMapStuff.size(), 1);

	commandInterface.set({cubeMap, &user_types::CubeMap::mipmapLevel_}, 4);
	dispatch();
	cubeMapStuff = select<ramses::TextureSampler>(*sceneContext.scene(), ramses::ERamsesObjectType::TextureSampler);
	ASSERT_EQ(cubeMapStuff.size(), 1);
}

TEST_F(CubeMapAdaptorFixture, wrongMipMapLevelImageSizes) {
	auto cubeMap = create<user_types::CubeMap>("Cubemap");

	commandInterface.set({cubeMap, &user_types::CubeMap::mipmapLevel_}, 4);
	dispatch();

	commandInterface.set({cubeMap, &user_types::CubeMap::uriBack_}, (test_path() / "images" / "blue_1024.png").string());
	commandInterface.set({cubeMap, &user_types::CubeMap::uriBottom_}, (test_path() / "images" / "blue_1024.png").string());
	commandInterface.set({cubeMap, &user_types::CubeMap::uriFront_}, (test_path() / "images" / "blue_1024.png").string());
	commandInterface.set({cubeMap, &user_types::CubeMap::uriLeft_}, (test_path() / "images" / "blue_1024.png").string());
	commandInterface.set({cubeMap, &user_types::CubeMap::uriRight_}, (test_path() / "images" / "blue_1024.png").string());
	commandInterface.set({cubeMap, &user_types::CubeMap::uriTop_}, (test_path() / "images" / "blue_1024.png").string());
	dispatch();

	commandInterface.set({cubeMap, &user_types::CubeMap::level2uriBack_}, (test_path() / "images" / "blue_1024.png").string());
	commandInterface.set({cubeMap, &user_types::CubeMap::level2uriBottom_}, (test_path() / "images" / "blue_1024.png").string());
	commandInterface.set({cubeMap, &user_types::CubeMap::level2uriFront_}, (test_path() / "images" / "blue_1024.png").string());
	commandInterface.set({cubeMap, &user_types::CubeMap::level2uriLeft_}, (test_path() / "images" / "blue_1024.png").string());
	commandInterface.set({cubeMap, &user_types::CubeMap::level2uriRight_}, (test_path() / "images" / "blue_1024.png").string());
	commandInterface.set({cubeMap, &user_types::CubeMap::level2uriTop_}, (test_path() / "images" / "blue_1024.png").string());
	dispatch();

	ASSERT_TRUE(commandInterface.errors().hasError({cubeMap, &user_types::CubeMap::level2uriBack_}));
	ASSERT_TRUE(commandInterface.errors().hasError({cubeMap, &user_types::CubeMap::level2uriBottom_}));
	ASSERT_TRUE(commandInterface.errors().hasError({cubeMap, &user_types::CubeMap::level2uriFront_}));
	ASSERT_TRUE(commandInterface.errors().hasError({cubeMap, &user_types::CubeMap::level2uriLeft_}));
	ASSERT_TRUE(commandInterface.errors().hasError({cubeMap, &user_types::CubeMap::level2uriRight_}));
	ASSERT_TRUE(commandInterface.errors().hasError({cubeMap, &user_types::CubeMap::level2uriTop_}));

	commandInterface.set({cubeMap, &user_types::CubeMap::level3uriBack_}, (test_path() / "images" / "blue_1024.png").string());
	commandInterface.set({cubeMap, &user_types::CubeMap::level3uriBottom_}, (test_path() / "images" / "blue_1024.png").string());
	commandInterface.set({cubeMap, &user_types::CubeMap::level3uriFront_}, (test_path() / "images" / "blue_1024.png").string());
	commandInterface.set({cubeMap, &user_types::CubeMap::level3uriLeft_}, (test_path() / "images" / "blue_1024.png").string());
	commandInterface.set({cubeMap, &user_types::CubeMap::level3uriRight_}, (test_path() / "images" / "blue_1024.png").string());
	commandInterface.set({cubeMap, &user_types::CubeMap::level3uriTop_}, (test_path() / "images" / "blue_1024.png").string());
	dispatch();

	ASSERT_TRUE(commandInterface.errors().hasError({cubeMap, &user_types::CubeMap::level3uriBack_}));
	ASSERT_TRUE(commandInterface.errors().hasError({cubeMap, &user_types::CubeMap::level3uriBottom_}));
	ASSERT_TRUE(commandInterface.errors().hasError({cubeMap, &user_types::CubeMap::level3uriFront_}));
	ASSERT_TRUE(commandInterface.errors().hasError({cubeMap, &user_types::CubeMap::level3uriLeft_}));
	ASSERT_TRUE(commandInterface.errors().hasError({cubeMap, &user_types::CubeMap::level3uriRight_}));
	ASSERT_TRUE(commandInterface.errors().hasError({cubeMap, &user_types::CubeMap::level3uriTop_}));

	commandInterface.set({cubeMap, &user_types::CubeMap::level4uriBack_}, (test_path() / "images" / "blue_1024.png").string());
	commandInterface.set({cubeMap, &user_types::CubeMap::level4uriBottom_}, (test_path() / "images" / "blue_1024.png").string());
	commandInterface.set({cubeMap, &user_types::CubeMap::level4uriFront_}, (test_path() / "images" / "blue_1024.png").string());
	commandInterface.set({cubeMap, &user_types::CubeMap::level4uriLeft_}, (test_path() / "images" / "blue_1024.png").string());
	commandInterface.set({cubeMap, &user_types::CubeMap::level4uriRight_}, (test_path() / "images" / "blue_1024.png").string());
	commandInterface.set({cubeMap, &user_types::CubeMap::level4uriTop_}, (test_path() / "images" / "blue_1024.png").string());
	dispatch();

	ASSERT_TRUE(commandInterface.errors().hasError({cubeMap, &user_types::CubeMap::level4uriBack_}));
	ASSERT_TRUE(commandInterface.errors().hasError({cubeMap, &user_types::CubeMap::level4uriBottom_}));
	ASSERT_TRUE(commandInterface.errors().hasError({cubeMap, &user_types::CubeMap::level4uriFront_}));
	ASSERT_TRUE(commandInterface.errors().hasError({cubeMap, &user_types::CubeMap::level4uriLeft_}));
	ASSERT_TRUE(commandInterface.errors().hasError({cubeMap, &user_types::CubeMap::level4uriRight_}));
	ASSERT_TRUE(commandInterface.errors().hasError({cubeMap, &user_types::CubeMap::level4uriTop_}));

	commandInterface.set({cubeMap, &user_types::CubeMap::mipmapLevel_}, 3);
	dispatch();

	ASSERT_FALSE(commandInterface.errors().hasError({cubeMap, &user_types::CubeMap::level4uriBack_}));
	ASSERT_FALSE(commandInterface.errors().hasError({cubeMap, &user_types::CubeMap::level4uriBottom_}));
	ASSERT_FALSE(commandInterface.errors().hasError({cubeMap, &user_types::CubeMap::level4uriFront_}));
	ASSERT_FALSE(commandInterface.errors().hasError({cubeMap, &user_types::CubeMap::level4uriLeft_}));
	ASSERT_FALSE(commandInterface.errors().hasError({cubeMap, &user_types::CubeMap::level4uriRight_}));
	ASSERT_FALSE(commandInterface.errors().hasError({cubeMap, &user_types::CubeMap::level4uriTop_}));

	commandInterface.set({cubeMap, &user_types::CubeMap::mipmapLevel_}, 2);
	dispatch();

	ASSERT_FALSE(commandInterface.errors().hasError({cubeMap, &user_types::CubeMap::level3uriBack_}));
	ASSERT_FALSE(commandInterface.errors().hasError({cubeMap, &user_types::CubeMap::level3uriBottom_}));
	ASSERT_FALSE(commandInterface.errors().hasError({cubeMap, &user_types::CubeMap::level3uriFront_}));
	ASSERT_FALSE(commandInterface.errors().hasError({cubeMap, &user_types::CubeMap::level3uriLeft_}));
	ASSERT_FALSE(commandInterface.errors().hasError({cubeMap, &user_types::CubeMap::level3uriRight_}));
	ASSERT_FALSE(commandInterface.errors().hasError({cubeMap, &user_types::CubeMap::level3uriTop_}));

	commandInterface.set({cubeMap, &user_types::CubeMap::mipmapLevel_}, 1);
	dispatch();

	ASSERT_FALSE(commandInterface.errors().hasError({cubeMap, &user_types::CubeMap::level2uriBack_}));
	ASSERT_FALSE(commandInterface.errors().hasError({cubeMap, &user_types::CubeMap::level2uriBottom_}));
	ASSERT_FALSE(commandInterface.errors().hasError({cubeMap, &user_types::CubeMap::level2uriFront_}));
	ASSERT_FALSE(commandInterface.errors().hasError({cubeMap, &user_types::CubeMap::level2uriLeft_}));
	ASSERT_FALSE(commandInterface.errors().hasError({cubeMap, &user_types::CubeMap::level2uriRight_}));
	ASSERT_FALSE(commandInterface.errors().hasError({cubeMap, &user_types::CubeMap::level2uriTop_}));
}

TEST_F(CubeMapAdaptorFixture, ramsesAutoMipMapGenerationWarningPersistsAfterChangingURI) {
	auto cubeMap = create<user_types::CubeMap>("Cubemap");

	commandInterface.set({cubeMap, &user_types::CubeMap::generateMipmaps_}, true);
	commandInterface.set({cubeMap, &user_types::CubeMap::mipmapLevel_}, 2);
	dispatch();
	ASSERT_TRUE(commandInterface.errors().hasError({cubeMap, &user_types::CubeMap::mipmapLevel_}));

	commandInterface.set({cubeMap, &user_types::CubeMap::uriBack_}, (test_path() / "images" / "blue_1024.png").string());
	commandInterface.set({cubeMap, &user_types::CubeMap::uriBottom_}, (test_path() / "images" / "blue_1024.png").string());
	commandInterface.set({cubeMap, &user_types::CubeMap::uriFront_}, (test_path() / "images" / "blue_1024.png").string());
	commandInterface.set({cubeMap, &user_types::CubeMap::uriLeft_}, (test_path() / "images" / "blue_1024.png").string());
	commandInterface.set({cubeMap, &user_types::CubeMap::uriRight_}, (test_path() / "images" / "blue_1024.png").string());
	commandInterface.set({cubeMap, &user_types::CubeMap::uriTop_}, (test_path() / "images" / "blue_1024.png").string());

	dispatch();
	ASSERT_TRUE(commandInterface.errors().hasError({cubeMap, &user_types::CubeMap::mipmapLevel_}));

	commandInterface.set({cubeMap, &user_types::CubeMap::level2uriBack_}, (test_path() / "images" / "green_512.png").string());
	commandInterface.set({cubeMap, &user_types::CubeMap::level2uriBottom_}, (test_path() / "images" / "green_512.png").string());
	commandInterface.set({cubeMap, &user_types::CubeMap::level2uriFront_}, (test_path() / "images" / "green_512.png").string());
	commandInterface.set({cubeMap, &user_types::CubeMap::level2uriLeft_}, (test_path() / "images" / "green_512.png").string());
	commandInterface.set({cubeMap, &user_types::CubeMap::level2uriRight_}, (test_path() / "images" / "green_512.png").string());
	commandInterface.set({cubeMap, &user_types::CubeMap::level2uriTop_}, (test_path() / "images" / "green_512.png").string());

	dispatch();
	ASSERT_TRUE(commandInterface.errors().hasError({cubeMap, &user_types::CubeMap::mipmapLevel_}));
}

TEST_F(CubeMapAdaptorFixture, textureFormat8BitPalette) {
	auto cubemap = create<user_types::CubeMap>("cubemap");

	commandInterface.set({cubemap, &user_types::CubeMap::uriBack_}, (test_path() / "images" / "text-back-palette.png").string());
	commandInterface.set({cubemap, &user_types::CubeMap::uriBottom_}, (test_path() / "images" / "text-back-palette.png").string());
	commandInterface.set({cubemap, &user_types::CubeMap::uriFront_}, (test_path() / "images" / "text-back-palette.png").string());
	commandInterface.set({cubemap, &user_types::CubeMap::uriLeft_}, (test_path() / "images" / "text-back-palette.png").string());
	commandInterface.set({cubemap, &user_types::CubeMap::uriRight_}, (test_path() / "images" / "text-back-palette.png").string());
	commandInterface.set({cubemap, &user_types::CubeMap::uriTop_}, (test_path() / "images" / "text-back-palette.png").string());
	dispatch();

	checkTextureFormats(cubemap, {{ETextureFormat::R8, core::ErrorLevel::INFORMATION},
									 {ETextureFormat::RG8, core::ErrorLevel::INFORMATION},
									 {ETextureFormat::RGB8, core::ErrorLevel::INFORMATION},
									 {ETextureFormat::RGBA8, core::ErrorLevel::INFORMATION},
									 {ETextureFormat::RGB16F, core::ErrorLevel::ERROR},
									 {ETextureFormat::RGBA16F, core::ErrorLevel::ERROR},
									 {ETextureFormat::SRGB8, core::ErrorLevel::INFORMATION},
									 {ETextureFormat::SRGB8_ALPHA8, core::ErrorLevel::INFORMATION}});
}

TEST_F(CubeMapAdaptorFixture, textureFormatR8) {
	auto cubemap = create<user_types::CubeMap>("cubemap");

	commandInterface.set({cubemap, &user_types::CubeMap::uriBack_}, (test_path() / "images" / "green_512_gray.png").string());
	commandInterface.set({cubemap, &user_types::CubeMap::uriBottom_}, (test_path() / "images" / "green_512_gray.png").string());
	commandInterface.set({cubemap, &user_types::CubeMap::uriFront_}, (test_path() / "images" / "green_512_gray.png").string());
	commandInterface.set({cubemap, &user_types::CubeMap::uriLeft_}, (test_path() / "images" / "green_512_gray.png").string());
	commandInterface.set({cubemap, &user_types::CubeMap::uriRight_}, (test_path() / "images" / "green_512_gray.png").string());
	commandInterface.set({cubemap, &user_types::CubeMap::uriTop_}, (test_path() / "images" / "green_512_gray.png").string());
	dispatch();

	checkTextureFormats(cubemap, {{ETextureFormat::R8, core::ErrorLevel::NONE},
									 {ETextureFormat::RG8, core::ErrorLevel::WARNING},
									 {ETextureFormat::RGB8, core::ErrorLevel::WARNING},
									 {ETextureFormat::RGBA8, core::ErrorLevel::WARNING},
									 {ETextureFormat::RGB16F, core::ErrorLevel::ERROR},
									 {ETextureFormat::RGBA16F, core::ErrorLevel::ERROR},
									 {ETextureFormat::SRGB8, core::ErrorLevel::WARNING},
									 {ETextureFormat::SRGB8_ALPHA8, core::ErrorLevel::WARNING}});
}

TEST_F(CubeMapAdaptorFixture, textureFormatRG16) {
	auto cubemap = create<user_types::CubeMap>("cubemap");

	commandInterface.set({cubemap, &user_types::CubeMap::uriBack_}, (test_path() / "images" / "green_512_gray_16f.png").string());
	commandInterface.set({cubemap, &user_types::CubeMap::uriBottom_}, (test_path() / "images" / "green_512_gray_16f.png").string());
	commandInterface.set({cubemap, &user_types::CubeMap::uriFront_}, (test_path() / "images" / "green_512_gray_16f.png").string());
	commandInterface.set({cubemap, &user_types::CubeMap::uriLeft_}, (test_path() / "images" / "green_512_gray_16f.png").string());
	commandInterface.set({cubemap, &user_types::CubeMap::uriRight_}, (test_path() / "images" / "green_512_gray_16f.png").string());
	commandInterface.set({cubemap, &user_types::CubeMap::uriTop_}, (test_path() / "images" / "green_512_gray_16f.png").string());
	dispatch();

	checkTextureFormats(cubemap, {{ETextureFormat::R8, core::ErrorLevel::ERROR},
									 {ETextureFormat::RG8, core::ErrorLevel::ERROR},
									 {ETextureFormat::RGB8, core::ErrorLevel::ERROR},
									 {ETextureFormat::RGBA8, core::ErrorLevel::ERROR},
									 {ETextureFormat::RGB16F, core::ErrorLevel::ERROR},
									 {ETextureFormat::RGBA16F, core::ErrorLevel::ERROR},
									 {ETextureFormat::SRGB8, core::ErrorLevel::ERROR},
									 {ETextureFormat::SRGB8_ALPHA8, core::ErrorLevel::ERROR}});
}

TEST_F(CubeMapAdaptorFixture, textureFormatRG8) {
	auto cubemap = create<user_types::CubeMap>("cubemap");

	commandInterface.set({cubemap, &user_types::CubeMap::uriBack_}, (test_path() / "images" / "green_512_gray_alpha.png").string());
	commandInterface.set({cubemap, &user_types::CubeMap::uriBottom_}, (test_path() / "images" / "green_512_gray_alpha.png").string());
	commandInterface.set({cubemap, &user_types::CubeMap::uriFront_}, (test_path() / "images" / "green_512_gray_alpha.png").string());
	commandInterface.set({cubemap, &user_types::CubeMap::uriLeft_}, (test_path() / "images" / "green_512_gray_alpha.png").string());
	commandInterface.set({cubemap, &user_types::CubeMap::uriRight_}, (test_path() / "images" / "green_512_gray_alpha.png").string());
	commandInterface.set({cubemap, &user_types::CubeMap::uriTop_}, (test_path() / "images" / "green_512_gray_alpha.png").string());
	dispatch();

	checkTextureFormats(cubemap, {{ETextureFormat::R8, core::ErrorLevel::INFORMATION},
									 {ETextureFormat::RG8, core::ErrorLevel::NONE},
									 {ETextureFormat::RGB8, core::ErrorLevel::WARNING},
									 {ETextureFormat::RGBA8, core::ErrorLevel::WARNING},
									 {ETextureFormat::RGB16F, core::ErrorLevel::ERROR},
									 {ETextureFormat::RGBA16F, core::ErrorLevel::ERROR},
									 {ETextureFormat::SRGB8, core::ErrorLevel::WARNING},
									 {ETextureFormat::SRGB8_ALPHA8, core::ErrorLevel::WARNING}});
}

TEST_F(CubeMapAdaptorFixture, textureFormatRGB8) {
	auto cubemap = create<user_types::CubeMap>("cubemap");

	commandInterface.set({cubemap, &user_types::CubeMap::uriBack_}, (test_path() / "images" / "text-back.png").string());
	commandInterface.set({cubemap, &user_types::CubeMap::uriBottom_}, (test_path() / "images" / "text-back.png").string());
	commandInterface.set({cubemap, &user_types::CubeMap::uriFront_}, (test_path() / "images" / "text-back.png").string());
	commandInterface.set({cubemap, &user_types::CubeMap::uriLeft_}, (test_path() / "images" / "text-back.png").string());
	commandInterface.set({cubemap, &user_types::CubeMap::uriRight_}, (test_path() / "images" / "text-back.png").string());
	commandInterface.set({cubemap, &user_types::CubeMap::uriTop_}, (test_path() / "images" / "text-back.png").string());
	dispatch();

	checkTextureFormats(cubemap, {{ETextureFormat::R8, core::ErrorLevel::INFORMATION},
									 {ETextureFormat::RG8, core::ErrorLevel::INFORMATION},
									 {ETextureFormat::RGB8, core::ErrorLevel::NONE},
									 {ETextureFormat::RGBA8, core::ErrorLevel::WARNING},
									 {ETextureFormat::RGB16F, core::ErrorLevel::ERROR},
									 {ETextureFormat::RGBA16F, core::ErrorLevel::ERROR},
									 {ETextureFormat::SRGB8, core::ErrorLevel::NONE},
									 {ETextureFormat::SRGB8_ALPHA8, core::ErrorLevel::WARNING}});
}

TEST_F(CubeMapAdaptorFixture, textureFormatRGBA8) {
	auto cubemap = create<user_types::CubeMap>("cubemap");

	commandInterface.set({cubemap, &user_types::CubeMap::uriBack_}, (test_path() / "images" / "green_512.png").string());
	commandInterface.set({cubemap, &user_types::CubeMap::uriBottom_}, (test_path() / "images" / "green_512.png").string());
	commandInterface.set({cubemap, &user_types::CubeMap::uriFront_}, (test_path() / "images" / "green_512.png").string());
	commandInterface.set({cubemap, &user_types::CubeMap::uriLeft_}, (test_path() / "images" / "green_512.png").string());
	commandInterface.set({cubemap, &user_types::CubeMap::uriRight_}, (test_path() / "images" / "green_512.png").string());
	commandInterface.set({cubemap, &user_types::CubeMap::uriTop_}, (test_path() / "images" / "green_512.png").string());
	dispatch();

	checkTextureFormats(cubemap, {{ETextureFormat::R8, core::ErrorLevel::INFORMATION},
									 {ETextureFormat::RG8, core::ErrorLevel::INFORMATION},
									 {ETextureFormat::RGB8, core::ErrorLevel::INFORMATION},
									 {ETextureFormat::RGBA8, core::ErrorLevel::NONE},
									 {ETextureFormat::RGB16F, core::ErrorLevel::ERROR},
									 {ETextureFormat::RGBA16F, core::ErrorLevel::ERROR},
									 {ETextureFormat::SRGB8, core::ErrorLevel::INFORMATION},
									 {ETextureFormat::SRGB8_ALPHA8, core::ErrorLevel::NONE}});
}

TEST_F(CubeMapAdaptorFixture, textureFormatRGB16) {
	auto cubemap = create<user_types::CubeMap>("cubemap");

	commandInterface.set({cubemap, &user_types::CubeMap::uriBack_}, (test_path() / "images" / "green_512_16f_no_alpha.png").string());
	commandInterface.set({cubemap, &user_types::CubeMap::uriBottom_}, (test_path() / "images" / "green_512_16f_no_alpha.png").string());
	commandInterface.set({cubemap, &user_types::CubeMap::uriFront_}, (test_path() / "images" / "green_512_16f_no_alpha.png").string());
	commandInterface.set({cubemap, &user_types::CubeMap::uriLeft_}, (test_path() / "images" / "green_512_16f_no_alpha.png").string());
	commandInterface.set({cubemap, &user_types::CubeMap::uriRight_}, (test_path() / "images" / "green_512_16f_no_alpha.png").string());
	commandInterface.set({cubemap, &user_types::CubeMap::uriTop_}, (test_path() / "images" / "green_512_16f_no_alpha.png").string());
	dispatch();

	checkTextureFormats(cubemap, {{ETextureFormat::R8, core::ErrorLevel::ERROR},
									 {ETextureFormat::RG8, core::ErrorLevel::ERROR},
									 {ETextureFormat::RGB8, core::ErrorLevel::ERROR},
									 {ETextureFormat::RGBA8, core::ErrorLevel::ERROR},
									 {ETextureFormat::RGB16F, core::ErrorLevel::NONE},
									 {ETextureFormat::RGBA16F, core::ErrorLevel::WARNING},
									 {ETextureFormat::SRGB8, core::ErrorLevel::ERROR},
									 {ETextureFormat::SRGB8_ALPHA8, core::ErrorLevel::ERROR}});
}

TEST_F(CubeMapAdaptorFixture, textureFormatRGBA16From16i) {
	auto cubemap = create<user_types::CubeMap>("cubemap");

	commandInterface.set({cubemap, &user_types::CubeMap::uriBack_}, (test_path() / "images" / "green_512_16i.png").string());
	commandInterface.set({cubemap, &user_types::CubeMap::uriBottom_}, (test_path() / "images" / "green_512_16i.png").string());
	commandInterface.set({cubemap, &user_types::CubeMap::uriFront_}, (test_path() / "images" / "green_512_16i.png").string());
	commandInterface.set({cubemap, &user_types::CubeMap::uriLeft_}, (test_path() / "images" / "green_512_16i.png").string());
	commandInterface.set({cubemap, &user_types::CubeMap::uriRight_}, (test_path() / "images" / "green_512_16i.png").string());
	commandInterface.set({cubemap, &user_types::CubeMap::uriTop_}, (test_path() / "images" / "green_512_16i.png").string());
	dispatch();

	checkTextureFormats(cubemap, {{ETextureFormat::R8, core::ErrorLevel::ERROR},
									 {ETextureFormat::RG8, core::ErrorLevel::ERROR},
									 {ETextureFormat::RGB8, core::ErrorLevel::ERROR},
									 {ETextureFormat::RGBA8, core::ErrorLevel::ERROR},
									 {ETextureFormat::RGB16F, core::ErrorLevel::INFORMATION},
									 {ETextureFormat::RGBA16F, core::ErrorLevel::NONE},
									 {ETextureFormat::SRGB8, core::ErrorLevel::ERROR},
									 {ETextureFormat::SRGB8_ALPHA8, core::ErrorLevel::ERROR}});
}

TEST_F(CubeMapAdaptorFixture, textureFormatRGBA16From16f) {
	auto cubemap = create<user_types::CubeMap>("cubemap");

	commandInterface.set({cubemap, &user_types::CubeMap::uriBack_}, (test_path() / "images" / "green_512_16f.png").string());
	commandInterface.set({cubemap, &user_types::CubeMap::uriBottom_}, (test_path() / "images" / "green_512_16f.png").string());
	commandInterface.set({cubemap, &user_types::CubeMap::uriFront_}, (test_path() / "images" / "green_512_16f.png").string());
	commandInterface.set({cubemap, &user_types::CubeMap::uriLeft_}, (test_path() / "images" / "green_512_16f.png").string());
	commandInterface.set({cubemap, &user_types::CubeMap::uriRight_}, (test_path() / "images" / "green_512_16f.png").string());
	commandInterface.set({cubemap, &user_types::CubeMap::uriTop_}, (test_path() / "images" / "green_512_16f.png").string());
	dispatch();

	checkTextureFormats(cubemap, {{ETextureFormat::R8, core::ErrorLevel::ERROR},
									 {ETextureFormat::RG8, core::ErrorLevel::ERROR},
									 {ETextureFormat::RGB8, core::ErrorLevel::ERROR},
									 {ETextureFormat::RGBA8, core::ErrorLevel::ERROR},
									 {ETextureFormat::RGB16F, core::ErrorLevel::INFORMATION},
									 {ETextureFormat::RGBA16F, core::ErrorLevel::NONE},
									 {ETextureFormat::SRGB8, core::ErrorLevel::ERROR},
									 {ETextureFormat::SRGB8_ALPHA8, core::ErrorLevel::ERROR}});
}

TEST_F(CubeMapAdaptorFixture, textureFormatChangeValidToInvalid) {
	auto cubemap = create<user_types::CubeMap>("cubemap");

	commandInterface.set({cubemap, &user_types::CubeMap::uriBack_}, (test_path() / "images" / "green_512_16f.png").string());
	commandInterface.set({cubemap, &user_types::CubeMap::uriBottom_}, (test_path() / "images" / "green_512_16f.png").string());
	commandInterface.set({cubemap, &user_types::CubeMap::uriFront_}, (test_path() / "images" / "green_512_16f.png").string());
	commandInterface.set({cubemap, &user_types::CubeMap::uriLeft_}, (test_path() / "images" / "green_512_16f.png").string());
	commandInterface.set({cubemap, &user_types::CubeMap::uriRight_}, (test_path() / "images" / "green_512_16f.png").string());
	commandInterface.set({cubemap, &user_types::CubeMap::uriTop_}, (test_path() / "images" / "green_512_16f.png").string());
	dispatch();

	// RGBA16
	commandInterface.set({cubemap, &user_types::CubeMap::textureFormat_}, static_cast<int>(ETextureFormat::RGBA16F));
	dispatch();
	assertLevel1Errors(cubemap, core::ErrorLevel::NONE);

	commandInterface.set({cubemap, &user_types::CubeMap::uriBack_}, (test_path() / "images" / "green_512.png").string());
	commandInterface.set({cubemap, &user_types::CubeMap::uriBottom_}, (test_path() / "images" / "green_512.png").string());
	commandInterface.set({cubemap, &user_types::CubeMap::uriFront_}, (test_path() / "images" / "green_512.png").string());
	commandInterface.set({cubemap, &user_types::CubeMap::uriLeft_}, (test_path() / "images" / "green_512.png").string());
	commandInterface.set({cubemap, &user_types::CubeMap::uriRight_}, (test_path() / "images" / "green_512.png").string());
	commandInterface.set({cubemap, &user_types::CubeMap::uriTop_}, (test_path() / "images" / "green_512.png").string());
	dispatch();
	assertLevel1Errors(cubemap, core::ErrorLevel::ERROR);

	// R8
	commandInterface.set({cubemap, &user_types::CubeMap::textureFormat_}, static_cast<int>(ETextureFormat::R8));
	dispatch();
	assertLevel1Errors(cubemap, core::ErrorLevel::INFORMATION);

	commandInterface.set({cubemap, &user_types::CubeMap::uriBack_}, (test_path() / "images" / "green_512_16f.png").string());
	commandInterface.set({cubemap, &user_types::CubeMap::uriBottom_}, (test_path() / "images" / "green_512_16f.png").string());
	commandInterface.set({cubemap, &user_types::CubeMap::uriFront_}, (test_path() / "images" / "green_512_16f.png").string());
	commandInterface.set({cubemap, &user_types::CubeMap::uriLeft_}, (test_path() / "images" / "green_512_16f.png").string());
	commandInterface.set({cubemap, &user_types::CubeMap::uriRight_}, (test_path() / "images" / "green_512_16f.png").string());
	commandInterface.set({cubemap, &user_types::CubeMap::uriTop_}, (test_path() / "images" / "green_512_16f.png").string());
	dispatch();
	assertLevel1Errors(cubemap, core::ErrorLevel::ERROR);

	// RGBA16
	commandInterface.set({cubemap, &user_types::CubeMap::textureFormat_}, static_cast<int>(ETextureFormat::RGBA16F));
	dispatch();
	assertLevel1Errors(cubemap, core::ErrorLevel::NONE);
}


TEST_F(CubeMapAdaptorFixture, textureBitdepthDifferentInSameLevel) {
	auto cubemap = create<user_types::CubeMap>("cubemap");

	commandInterface.set({cubemap, &user_types::CubeMap::uriBack_}, (test_path() / "images" / "green_512_16f.png").string());
	commandInterface.set({cubemap, &user_types::CubeMap::uriBottom_}, (test_path() / "images" / "green_512_16f.png").string());
	commandInterface.set({cubemap, &user_types::CubeMap::uriFront_}, (test_path() / "images" / "green_512_16f.png").string());
	commandInterface.set({cubemap, &user_types::CubeMap::uriLeft_}, (test_path() / "images" / "green_512_16f.png").string());
	commandInterface.set({cubemap, &user_types::CubeMap::uriRight_}, (test_path() / "images" / "green_512_16f.png").string());
	commandInterface.set({cubemap, &user_types::CubeMap::uriTop_}, (test_path() / "images" / "green_512_16f.png").string());
	dispatch();

	// RGBA16
	commandInterface.set({cubemap, &user_types::CubeMap::textureFormat_}, static_cast<int>(ETextureFormat::RGBA16F));
	dispatch();
	ASSERT_FALSE(commandInterface.errors().hasError({cubemap, &user_types::CubeMap::textureFormat_}));

	commandInterface.set({cubemap, &user_types::CubeMap::uriFront_}, (test_path() / "images" / "green_512.png").string());
	dispatch();
	ASSERT_TRUE(commandInterface.errors().hasError({cubemap, &user_types::CubeMap::uriFront_}));
	ASSERT_EQ(commandInterface.errors().getError({cubemap, &user_types::CubeMap::uriFront_}).level(), core::ErrorLevel::ERROR);

	commandInterface.set({cubemap, &user_types::CubeMap::uriFront_}, (test_path() / "images" / "green_512_16f.png").string());
	dispatch();
	ASSERT_FALSE(commandInterface.errors().hasError({cubemap, &user_types::CubeMap::uriFront_}));
}

TEST_F(CubeMapAdaptorFixture, textureBitdepthDifferentInOtherLevel) {
	auto cubemap = create<user_types::CubeMap>("cubemap");

	commandInterface.set({cubemap, &user_types::CubeMap::uriBack_}, (test_path() / "images" / "blue_1024_16i.png").string());
	commandInterface.set({cubemap, &user_types::CubeMap::uriBottom_}, (test_path() / "images" / "blue_1024_16i.png").string());
	commandInterface.set({cubemap, &user_types::CubeMap::uriFront_}, (test_path() / "images" / "blue_1024_16i.png").string());
	commandInterface.set({cubemap, &user_types::CubeMap::uriLeft_}, (test_path() / "images" / "blue_1024_16i.png").string());
	commandInterface.set({cubemap, &user_types::CubeMap::uriRight_}, (test_path() / "images" / "blue_1024_16i.png").string());
	commandInterface.set({cubemap, &user_types::CubeMap::uriTop_}, (test_path() / "images" / "blue_1024_16i.png").string());
	dispatch();

	// RGBA16
	commandInterface.set({cubemap, &user_types::CubeMap::textureFormat_}, static_cast<int>(ETextureFormat::RGBA16F));
	dispatch();
	ASSERT_FALSE(commandInterface.errors().hasError({cubemap, &user_types::CubeMap::textureFormat_}));

	commandInterface.set({cubemap, &user_types::CubeMap::level2uriBack_}, (test_path() / "images" / "green_512.png").string());
	commandInterface.set({cubemap, &user_types::CubeMap::level2uriBottom_}, (test_path() / "images" / "green_512.png").string());
	commandInterface.set({cubemap, &user_types::CubeMap::level2uriFront_}, (test_path() / "images" / "green_512.png").string());
	commandInterface.set({cubemap, &user_types::CubeMap::level2uriLeft_}, (test_path() / "images" / "green_512.png").string());
	commandInterface.set({cubemap, &user_types::CubeMap::level2uriRight_}, (test_path() / "images" / "green_512.png").string());
	commandInterface.set({cubemap, &user_types::CubeMap::level2uriTop_}, (test_path() / "images" / "green_512.png").string());
	commandInterface.set({cubemap, &user_types::CubeMap::mipmapLevel_}, 2);
	dispatch();
	ASSERT_TRUE(commandInterface.errors().hasError({cubemap, &user_types::CubeMap::level2uriBack_}));
	ASSERT_EQ(commandInterface.errors().getError({cubemap, &user_types::CubeMap::level2uriBack_}).level(), core::ErrorLevel::ERROR);

	ASSERT_TRUE(commandInterface.errors().hasError({cubemap, &user_types::CubeMap::level2uriBottom_}));
	ASSERT_EQ(commandInterface.errors().getError({cubemap, &user_types::CubeMap::level2uriBottom_}).level(), core::ErrorLevel::ERROR);

	ASSERT_TRUE(commandInterface.errors().hasError({cubemap, &user_types::CubeMap::level2uriFront_}));
	ASSERT_EQ(commandInterface.errors().getError({cubemap, &user_types::CubeMap::level2uriFront_}).level(), core::ErrorLevel::ERROR);

	ASSERT_TRUE(commandInterface.errors().hasError({cubemap, &user_types::CubeMap::level2uriLeft_}));
	ASSERT_EQ(commandInterface.errors().getError({cubemap, &user_types::CubeMap::level2uriLeft_}).level(), core::ErrorLevel::ERROR);

	ASSERT_TRUE(commandInterface.errors().hasError({cubemap, &user_types::CubeMap::level2uriRight_}));
	ASSERT_EQ(commandInterface.errors().getError({cubemap, &user_types::CubeMap::level2uriRight_}).level(), core::ErrorLevel::ERROR);

	ASSERT_TRUE(commandInterface.errors().hasError({cubemap, &user_types::CubeMap::level2uriTop_}));
	ASSERT_EQ(commandInterface.errors().getError({cubemap, &user_types::CubeMap::level2uriTop_}).level(), core::ErrorLevel::ERROR);

	commandInterface.set({cubemap, &user_types::CubeMap::level2uriBack_}, (test_path() / "images" / "green_512_16i.png").string());
	commandInterface.set({cubemap, &user_types::CubeMap::level2uriBottom_}, (test_path() / "images" / "green_512_16i.png").string());
	commandInterface.set({cubemap, &user_types::CubeMap::level2uriFront_}, (test_path() / "images" / "green_512_16i.png").string());
	commandInterface.set({cubemap, &user_types::CubeMap::level2uriLeft_}, (test_path() / "images" / "green_512_16i.png").string());
	commandInterface.set({cubemap, &user_types::CubeMap::level2uriRight_}, (test_path() / "images" / "green_512_16i.png").string());
	commandInterface.set({cubemap, &user_types::CubeMap::level2uriTop_}, (test_path() / "images" / "green_512_16i.png").string());
	dispatch();
	ASSERT_FALSE(commandInterface.errors().hasError({cubemap, &user_types::CubeMap::level2uriBack_}));
	ASSERT_FALSE(commandInterface.errors().hasError({cubemap, &user_types::CubeMap::level2uriBottom_}));
	ASSERT_FALSE(commandInterface.errors().hasError({cubemap, &user_types::CubeMap::level2uriFront_}));
	ASSERT_FALSE(commandInterface.errors().hasError({cubemap, &user_types::CubeMap::level2uriLeft_}));
	ASSERT_FALSE(commandInterface.errors().hasError({cubemap, &user_types::CubeMap::level2uriRight_}));
	ASSERT_FALSE(commandInterface.errors().hasError({cubemap, &user_types::CubeMap::level2uriTop_}));
}

TEST_F(CubeMapAdaptorFixture, textureFormatDifferentInSameLevel) {
	auto cubemap = create<user_types::CubeMap>("cubemap");

	commandInterface.set({cubemap, &user_types::CubeMap::textureFormat_}, static_cast<int>(ETextureFormat::RGBA8));
	dispatch();

	commandInterface.set({cubemap, &user_types::CubeMap::uriBack_}, (test_path() / "images" / "green_512.png").string());
	commandInterface.set({cubemap, &user_types::CubeMap::uriBottom_}, (test_path() / "images" / "green_512.png").string());
	commandInterface.set({cubemap, &user_types::CubeMap::uriFront_}, (test_path() / "images" / "green_512.png").string());
	commandInterface.set({cubemap, &user_types::CubeMap::uriLeft_}, (test_path() / "images" / "green_512.png").string());
	commandInterface.set({cubemap, &user_types::CubeMap::uriRight_}, (test_path() / "images" / "green_512.png").string());
	commandInterface.set({cubemap, &user_types::CubeMap::uriTop_}, (test_path() / "images" / "green_512.png").string());
	dispatch();

	commandInterface.set({cubemap, &user_types::CubeMap::uriFront_}, (test_path() / "images" / "green_512_gray.png").string());
	dispatch();
	ASSERT_TRUE(commandInterface.errors().hasError({cubemap, &user_types::CubeMap::uriFront_}));
	ASSERT_EQ(commandInterface.errors().getError({cubemap, &user_types::CubeMap::uriFront_}).level(), core::ErrorLevel::WARNING);

	commandInterface.set({cubemap, &user_types::CubeMap::uriFront_}, (test_path() / "images" / "green_512.png").string());
	dispatch();
	ASSERT_FALSE(commandInterface.errors().hasError({cubemap, &user_types::CubeMap::uriFront_}));
}

TEST_F(CubeMapAdaptorFixture, textureFormatDifferentInOtherLevel) {
	auto cubemap = create<user_types::CubeMap>("cubemap");

	commandInterface.set({cubemap, &user_types::CubeMap::uriBack_}, (test_path() / "images" / "blue_1024.png").string());
	commandInterface.set({cubemap, &user_types::CubeMap::uriBottom_}, (test_path() / "images" / "blue_1024.png").string());
	commandInterface.set({cubemap, &user_types::CubeMap::uriFront_}, (test_path() / "images" / "blue_1024.png").string());
	commandInterface.set({cubemap, &user_types::CubeMap::uriLeft_}, (test_path() / "images" / "blue_1024.png").string());
	commandInterface.set({cubemap, &user_types::CubeMap::uriRight_}, (test_path() / "images" / "blue_1024.png").string());
	commandInterface.set({cubemap, &user_types::CubeMap::uriTop_}, (test_path() / "images" / "blue_1024.png").string());
	dispatch();

	// RGBA8
	commandInterface.set({cubemap, &user_types::CubeMap::textureFormat_}, static_cast<int>(ETextureFormat::RGBA8));
	dispatch();

	commandInterface.set({cubemap, &user_types::CubeMap::level2uriBack_}, (test_path() / "images" / "green_512_gray.png").string());
	commandInterface.set({cubemap, &user_types::CubeMap::level2uriBottom_}, (test_path() / "images" / "green_512_gray.png").string());
	commandInterface.set({cubemap, &user_types::CubeMap::level2uriFront_}, (test_path() / "images" / "green_512_gray.png").string());
	commandInterface.set({cubemap, &user_types::CubeMap::level2uriLeft_}, (test_path() / "images" / "green_512_gray.png").string());
	commandInterface.set({cubemap, &user_types::CubeMap::level2uriRight_}, (test_path() / "images" / "green_512_gray.png").string());
	commandInterface.set({cubemap, &user_types::CubeMap::level2uriTop_}, (test_path() / "images" / "green_512_gray.png").string());
	commandInterface.set({cubemap, &user_types::CubeMap::mipmapLevel_}, 2);
	dispatch();
	ASSERT_TRUE(commandInterface.errors().hasError({cubemap, &user_types::CubeMap::level2uriBack_}));
	ASSERT_EQ(commandInterface.errors().getError({cubemap, &user_types::CubeMap::level2uriBack_}).level(), core::ErrorLevel::WARNING);

	ASSERT_TRUE(commandInterface.errors().hasError({cubemap, &user_types::CubeMap::level2uriBottom_}));
	ASSERT_EQ(commandInterface.errors().getError({cubemap, &user_types::CubeMap::level2uriBottom_}).level(), core::ErrorLevel::WARNING);

	ASSERT_TRUE(commandInterface.errors().hasError({cubemap, &user_types::CubeMap::level2uriFront_}));
	ASSERT_EQ(commandInterface.errors().getError({cubemap, &user_types::CubeMap::level2uriFront_}).level(), core::ErrorLevel::WARNING);

	ASSERT_TRUE(commandInterface.errors().hasError({cubemap, &user_types::CubeMap::level2uriLeft_}));
	ASSERT_EQ(commandInterface.errors().getError({cubemap, &user_types::CubeMap::level2uriLeft_}).level(), core::ErrorLevel::WARNING);

	ASSERT_TRUE(commandInterface.errors().hasError({cubemap, &user_types::CubeMap::level2uriRight_}));
	ASSERT_EQ(commandInterface.errors().getError({cubemap, &user_types::CubeMap::level2uriRight_}).level(), core::ErrorLevel::WARNING);

	ASSERT_TRUE(commandInterface.errors().hasError({cubemap, &user_types::CubeMap::level2uriTop_}));
	ASSERT_EQ(commandInterface.errors().getError({cubemap, &user_types::CubeMap::level2uriTop_}).level(), core::ErrorLevel::WARNING);

	commandInterface.set({cubemap, &user_types::CubeMap::level2uriBack_}, (test_path() / "images" / "green_512.png").string());
	commandInterface.set({cubemap, &user_types::CubeMap::level2uriBottom_}, (test_path() / "images" / "green_512.png").string());
	commandInterface.set({cubemap, &user_types::CubeMap::level2uriFront_}, (test_path() / "images" / "green_512.png").string());
	commandInterface.set({cubemap, &user_types::CubeMap::level2uriLeft_}, (test_path() / "images" / "green_512.png").string());
	commandInterface.set({cubemap, &user_types::CubeMap::level2uriRight_}, (test_path() / "images" / "green_512.png").string());
	commandInterface.set({cubemap, &user_types::CubeMap::level2uriTop_}, (test_path() / "images" / "green_512.png").string());
	dispatch();
	ASSERT_FALSE(commandInterface.errors().hasError({cubemap, &user_types::CubeMap::level2uriBack_}));
	ASSERT_FALSE(commandInterface.errors().hasError({cubemap, &user_types::CubeMap::level2uriBottom_}));
	ASSERT_FALSE(commandInterface.errors().hasError({cubemap, &user_types::CubeMap::level2uriFront_}));
	ASSERT_FALSE(commandInterface.errors().hasError({cubemap, &user_types::CubeMap::level2uriLeft_}));
	ASSERT_FALSE(commandInterface.errors().hasError({cubemap, &user_types::CubeMap::level2uriRight_}));
	ASSERT_FALSE(commandInterface.errors().hasError({cubemap, &user_types::CubeMap::level2uriTop_}));
}