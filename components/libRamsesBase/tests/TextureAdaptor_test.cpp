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
#include "ramses_adaptor/TextureSamplerAdaptor.h"
#include "testing/TestUtil.h"

class TextureAdaptorFixture : public RamsesBaseFixture<> {
public:
	void checkTextureFormats(raco::core::SEditorObject texture, const std::map<ramses::ETextureFormat, raco::core::ErrorLevel>& formats) {
		for (const auto& [format, level] : formats) {
			commandInterface.set({texture, &raco::user_types::Texture::textureFormat_}, static_cast<int>(format));
			dispatch();
			if (level == raco::core::ErrorLevel::NONE) {
				ASSERT_FALSE(commandInterface.errors().hasError({texture, &raco::user_types::Texture::uri_}));
			} else {
				ASSERT_TRUE(commandInterface.errors().hasError({texture, &raco::user_types::Texture::uri_}));
				ASSERT_EQ(commandInterface.errors().getError({texture, &raco::user_types::Texture::uri_}).level(), level);
			}
		}
	}

};

TEST_F(TextureAdaptorFixture, textureFormat8BitPalette) {
	auto texture = create<raco::user_types::Texture>("texture");

	commandInterface.set({texture, &raco::user_types::Texture::uri_}, (test_path() / "images" / "text-back-palette.png").string());
	dispatch();

	checkTextureFormats(texture, {{ramses::ETextureFormat::R8, raco::core::ErrorLevel::INFORMATION},
									 {ramses::ETextureFormat::RG8, raco::core::ErrorLevel::INFORMATION},
									 {ramses::ETextureFormat::RGB8, raco::core::ErrorLevel::INFORMATION},
									 {ramses::ETextureFormat::RGBA8, raco::core::ErrorLevel::INFORMATION},
									 {ramses::ETextureFormat::RGB16F, raco::core::ErrorLevel::ERROR},
									 {ramses::ETextureFormat::RGBA16F, raco::core::ErrorLevel::ERROR},
									 {ramses::ETextureFormat::SRGB8, raco::core::ErrorLevel::INFORMATION},
									 {ramses::ETextureFormat::SRGB8_ALPHA8, raco::core::ErrorLevel::INFORMATION}});
}

TEST_F(TextureAdaptorFixture, textureFormatR8) {
	auto texture = create<raco::user_types::Texture>("texture");

	commandInterface.set({texture, &raco::user_types::Texture::uri_}, (test_path() / "images" / "green_512_gray.png").string());
	dispatch();

	checkTextureFormats(texture, {{ramses::ETextureFormat::R8, raco::core::ErrorLevel::NONE},
									 // Swizzling results in R8 ramses texture format, being compatible with GRAY image. No warnings expected.
									 {ramses::ETextureFormat::RG8, raco::core::ErrorLevel::NONE},
									 {ramses::ETextureFormat::RGB8, raco::core::ErrorLevel::NONE},
									 {ramses::ETextureFormat::RGBA8, raco::core::ErrorLevel::NONE},
									 // Swizzling results in R16F ramses format. It is incompatible with 8 bit image.
									 {ramses::ETextureFormat::RGB16F, raco::core::ErrorLevel::ERROR},
									 {ramses::ETextureFormat::RGBA16F, raco::core::ErrorLevel::ERROR},
									 // Swizzling results in R8 ramses texture format, being compatible with GRAY image. No warnings expected.
									 {ramses::ETextureFormat::SRGB8, raco::core::ErrorLevel::NONE},
									 {ramses::ETextureFormat::SRGB8_ALPHA8, raco::core::ErrorLevel::NONE}});
}

TEST_F(TextureAdaptorFixture, textureFormatRG16) {
	auto texture = create<raco::user_types::Texture>("texture");

	commandInterface.set({texture, &raco::user_types::Texture::uri_}, (test_path() / "images" / "green_512_gray_16f.png").string());
	dispatch();

	checkTextureFormats(texture, {{ramses::ETextureFormat::R8, raco::core::ErrorLevel::ERROR},
									 {ramses::ETextureFormat::RG8, raco::core::ErrorLevel::ERROR},
									 {ramses::ETextureFormat::RGB8, raco::core::ErrorLevel::ERROR},
									 {ramses::ETextureFormat::RGBA8, raco::core::ErrorLevel::ERROR},
									 {ramses::ETextureFormat::RGB16F, raco::core::ErrorLevel::ERROR},
									 {ramses::ETextureFormat::RGBA16F, raco::core::ErrorLevel::ERROR},
									 {ramses::ETextureFormat::SRGB8, raco::core::ErrorLevel::ERROR},
									 {ramses::ETextureFormat::SRGB8_ALPHA8, raco::core::ErrorLevel::ERROR}});
}

TEST_F(TextureAdaptorFixture, textureFormatRG8) {
	auto texture = create<raco::user_types::Texture>("texture");

	commandInterface.set({texture, &raco::user_types::Texture::uri_}, (test_path() / "images" / "green_512_gray_alpha.png").string());
	dispatch();

	checkTextureFormats(texture, {{ramses::ETextureFormat::R8, raco::core::ErrorLevel::INFORMATION},
									 {ramses::ETextureFormat::RG8, raco::core::ErrorLevel::NONE},
									 // Swizzling results in R8 ramses texture format, must be info same as above.
									 {ramses::ETextureFormat::RGB8, raco::core::ErrorLevel::INFORMATION},
									 // Swizzling results in RG8 ramses texture format, compatible with image, no errors.
									 {ramses::ETextureFormat::RGBA8, raco::core::ErrorLevel::NONE},
									 {ramses::ETextureFormat::RGB16F, raco::core::ErrorLevel::ERROR},
									 {ramses::ETextureFormat::RGBA16F, raco::core::ErrorLevel::ERROR},
									 // Swizzling results in R8 ramses texture format, must be info same as above.
									 {ramses::ETextureFormat::SRGB8, raco::core::ErrorLevel::INFORMATION},
									 // Swizzling results in RG8 ramses texture format, compatible with image, no errors.
									 {ramses::ETextureFormat::SRGB8_ALPHA8, raco::core::ErrorLevel::NONE}});
}

TEST_F(TextureAdaptorFixture, textureFormatRGB8) {
	auto texture = create<raco::user_types::Texture>("texture");

	commandInterface.set({texture, &raco::user_types::Texture::uri_}, (test_path() / "images" / "text-back.png").string());
	dispatch();

	checkTextureFormats(texture, {{ramses::ETextureFormat::R8, raco::core::ErrorLevel::INFORMATION},
									 {ramses::ETextureFormat::RG8, raco::core::ErrorLevel::INFORMATION},
									 {ramses::ETextureFormat::RGB8, raco::core::ErrorLevel::NONE},
									 // Swizzle format is RGB8, compatible with file, no issues.
									 {ramses::ETextureFormat::RGBA8, raco::core::ErrorLevel::NONE},
									 {ramses::ETextureFormat::RGB16F, raco::core::ErrorLevel::ERROR},
									 {ramses::ETextureFormat::RGBA16F, raco::core::ErrorLevel::ERROR},
									 {ramses::ETextureFormat::SRGB8, raco::core::ErrorLevel::NONE},
									 // Swizzle format is RGB8, compatible with file, no issues.
									 {ramses::ETextureFormat::SRGB8_ALPHA8, raco::core::ErrorLevel::NONE}});
}

TEST_F(TextureAdaptorFixture, textureFormatRGBA8) {
	auto texture = create<raco::user_types::Texture>("texture");

	commandInterface.set({texture, &raco::user_types::Texture::uri_}, (test_path() / "images" / "green_512.png").string());
	dispatch();

	checkTextureFormats(texture, {{ramses::ETextureFormat::R8, raco::core::ErrorLevel::INFORMATION},
									 {ramses::ETextureFormat::RG8, raco::core::ErrorLevel::INFORMATION},
									 {ramses::ETextureFormat::RGB8, raco::core::ErrorLevel::INFORMATION},
									 {ramses::ETextureFormat::RGBA8, raco::core::ErrorLevel::NONE},
									 {ramses::ETextureFormat::RGB16F, raco::core::ErrorLevel::ERROR},
									 {ramses::ETextureFormat::RGBA16F, raco::core::ErrorLevel::ERROR},
									 {ramses::ETextureFormat::SRGB8, raco::core::ErrorLevel::INFORMATION},
									 {ramses::ETextureFormat::SRGB8_ALPHA8, raco::core::ErrorLevel::NONE}});
}

TEST_F(TextureAdaptorFixture, textureFormatRGBA8Flipped) {
	auto texture = create<raco::user_types::Texture>("texture");

	commandInterface.set({texture, &raco::user_types::Texture::uri_}, (test_path() / "images" / "green_512.png").string());
	commandInterface.set({texture, &raco::user_types::Texture::flipTexture_}, true);
	dispatch();

	commandInterface.set({texture, &raco::user_types::Texture::textureFormat_}, static_cast<int>(ramses::ETextureFormat::RGBA8));
	ASSERT_NO_THROW(dispatch());

	commandInterface.set({texture, &raco::user_types::Texture::textureFormat_}, static_cast<int>(ramses::ETextureFormat::RGB8));
	ASSERT_NO_THROW(dispatch());

	commandInterface.set({texture, &raco::user_types::Texture::textureFormat_}, static_cast<int>(ramses::ETextureFormat::RG8));
	ASSERT_NO_THROW(dispatch());

	commandInterface.set({texture, &raco::user_types::Texture::textureFormat_}, static_cast<int>(ramses::ETextureFormat::R8));
	ASSERT_NO_THROW(dispatch());

	commandInterface.set({texture, &raco::user_types::Texture::textureFormat_}, static_cast<int>(ramses::ETextureFormat::SRGB8));
	ASSERT_NO_THROW(dispatch());

	commandInterface.set({texture, &raco::user_types::Texture::textureFormat_}, static_cast<int>(ramses::ETextureFormat::SRGB8_ALPHA8));
	ASSERT_NO_THROW(dispatch());
}

TEST_F(TextureAdaptorFixture, textureFormatRGB16) {
	auto texture = create<raco::user_types::Texture>("texture");

	commandInterface.set({texture, &raco::user_types::Texture::uri_}, (test_path() / "images" / "green_512_16f_no_alpha.png").string());
	dispatch();

	checkTextureFormats(texture, {{ramses::ETextureFormat::R8, raco::core::ErrorLevel::ERROR},
									 {ramses::ETextureFormat::RG8, raco::core::ErrorLevel::ERROR},
									 {ramses::ETextureFormat::RGB8, raco::core::ErrorLevel::ERROR},
									 {ramses::ETextureFormat::RGBA8, raco::core::ErrorLevel::ERROR},
									 {ramses::ETextureFormat::RGB16F, raco::core::ErrorLevel::NONE},
									 // Swizzled format RGB16F is compatible with image data. No warning.
									 {ramses::ETextureFormat::RGBA16F, raco::core::ErrorLevel::NONE},
									 {ramses::ETextureFormat::SRGB8, raco::core::ErrorLevel::ERROR},
									 {ramses::ETextureFormat::SRGB8_ALPHA8, raco::core::ErrorLevel::ERROR}});
}

TEST_F(TextureAdaptorFixture, textureFormatRGBA16From16i) {
	auto texture = create<raco::user_types::Texture>("texture");

	commandInterface.set({texture, &raco::user_types::Texture::uri_}, (test_path() / "images" / "green_512_16i.png").string());
	dispatch();

	checkTextureFormats(texture, {{ramses::ETextureFormat::R8, raco::core::ErrorLevel::ERROR},
									 {ramses::ETextureFormat::RG8, raco::core::ErrorLevel::ERROR},
									 {ramses::ETextureFormat::RGB8, raco::core::ErrorLevel::ERROR},
									 {ramses::ETextureFormat::RGBA8, raco::core::ErrorLevel::ERROR},
									 {ramses::ETextureFormat::RGB16F, raco::core::ErrorLevel::INFORMATION},
									 {ramses::ETextureFormat::RGBA16F, raco::core::ErrorLevel::NONE},
									 {ramses::ETextureFormat::SRGB8, raco::core::ErrorLevel::ERROR},
									 {ramses::ETextureFormat::SRGB8_ALPHA8, raco::core::ErrorLevel::ERROR}});
}

TEST_F(TextureAdaptorFixture, textureFormatRGBA16From16f) {
	auto texture = create<raco::user_types::Texture>("texture");

	commandInterface.set({texture, &raco::user_types::Texture::uri_}, (test_path() / "images" / "green_512_16f.png").string());
	dispatch();

	checkTextureFormats(texture, {{ramses::ETextureFormat::R8, raco::core::ErrorLevel::ERROR},
									 {ramses::ETextureFormat::RG8, raco::core::ErrorLevel::ERROR},
									 {ramses::ETextureFormat::RGB8, raco::core::ErrorLevel::ERROR},
									 {ramses::ETextureFormat::RGBA8, raco::core::ErrorLevel::ERROR},
									 {ramses::ETextureFormat::RGB16F, raco::core::ErrorLevel::INFORMATION},
									 {ramses::ETextureFormat::RGBA16F, raco::core::ErrorLevel::NONE},
									 {ramses::ETextureFormat::SRGB8, raco::core::ErrorLevel::ERROR},
									 {ramses::ETextureFormat::SRGB8_ALPHA8, raco::core::ErrorLevel::ERROR}});
}

TEST_F(TextureAdaptorFixture, textureFormatRGBA16Flipped) {
	auto texture = create<raco::user_types::Texture>("texture");

	commandInterface.set({texture, &raco::user_types::Texture::uri_}, (test_path() / "images" / "green_512_16f.png").string());
	commandInterface.set({texture, &raco::user_types::Texture::flipTexture_}, true);
	dispatch();

	commandInterface.set({texture, &raco::user_types::Texture::textureFormat_}, static_cast<int>(ramses::ETextureFormat::RGBA16F));
	ASSERT_NO_THROW(dispatch());

	commandInterface.set({texture, &raco::user_types::Texture::textureFormat_}, static_cast<int>(ramses::ETextureFormat::RGB16F));
	ASSERT_NO_THROW(dispatch());
}

TEST_F(TextureAdaptorFixture, textureFormatChangeValidToInvalid) {
	auto texture = create<raco::user_types::Texture>("texture");

	commandInterface.set({texture, &raco::user_types::Texture::uri_}, (test_path() / "images" / "green_512_16f.png").string());
	dispatch();

	// RGBA16
	commandInterface.set({texture, &raco::user_types::Texture::textureFormat_}, static_cast<int>(ramses::ETextureFormat::RGBA16F));
	dispatch();
	ASSERT_FALSE(commandInterface.errors().hasError({texture, &raco::user_types::Texture::uri_}));

	commandInterface.set({texture, &raco::user_types::Texture::uri_}, (test_path() / "images" / "green_512.png").string());
	dispatch();
	ASSERT_TRUE(commandInterface.errors().hasError({texture, &raco::user_types::Texture::uri_}));
	ASSERT_EQ(commandInterface.errors().getError({texture, &raco::user_types::Texture::uri_}).level(), raco::core::ErrorLevel::ERROR);

	// R8
	commandInterface.set({texture, &raco::user_types::Texture::textureFormat_}, static_cast<int>(ramses::ETextureFormat::R8));
	dispatch();
	ASSERT_TRUE(commandInterface.errors().hasError({texture, &raco::user_types::Texture::uri_}));
	ASSERT_EQ(commandInterface.errors().getError({texture, &raco::user_types::Texture::uri_}).level(), raco::core::ErrorLevel::INFORMATION);

	commandInterface.set({texture, &raco::user_types::Texture::uri_}, (test_path() / "images" / "green_512_16f.png").string());
	dispatch();
	ASSERT_TRUE(commandInterface.errors().hasError({texture, &raco::user_types::Texture::uri_}));
	ASSERT_EQ(commandInterface.errors().getError({texture, &raco::user_types::Texture::uri_}).level(), raco::core::ErrorLevel::ERROR);

	// RGBA16
	commandInterface.set({texture, &raco::user_types::Texture::textureFormat_}, static_cast<int>(ramses::ETextureFormat::RGBA16F));
	dispatch();
	ASSERT_FALSE(commandInterface.errors().hasError({texture, &raco::user_types::Texture::uri_}));
}

TEST_F(TextureAdaptorFixture, textureGenerationAtMultipleLevels) {
	auto texture = create<raco::user_types::Texture>("Texture");

	commandInterface.set({texture, &raco::user_types::Texture::uri_}, (test_path() / "images" / "blue_1024.png").string());

	dispatch();

	commandInterface.set({texture, &raco::user_types::Texture::level2uri_}, (test_path() / "images" / "green_512.png").string());

	dispatch();

	commandInterface.set({texture, &raco::user_types::Texture::level3uri_}, (test_path() / "images" / "yellow_256.png").string());

	dispatch();

	commandInterface.set({texture, &raco::user_types::Texture::level4uri_}, (test_path() / "images" / "red_128.png").string());

	dispatch();

	auto textureStuff{select<ramses::TextureSampler>(*sceneContext.scene(), ramses::ERamsesObjectType::ERamsesObjectType_TextureSampler)};
	ASSERT_EQ(textureStuff.size(), 1);

	commandInterface.set({texture, &raco::user_types::Texture::mipmapLevel_}, 2);
	dispatch();
	textureStuff = select<ramses::TextureSampler>(*sceneContext.scene(), ramses::ERamsesObjectType::ERamsesObjectType_TextureSampler);
	ASSERT_EQ(textureStuff.size(), 1);

	commandInterface.set({texture, &raco::user_types::Texture::mipmapLevel_}, 3);
	dispatch();
	textureStuff = select<ramses::TextureSampler>(*sceneContext.scene(), ramses::ERamsesObjectType::ERamsesObjectType_TextureSampler);
	ASSERT_EQ(textureStuff.size(), 1);

	commandInterface.set({texture, &raco::user_types::Texture::mipmapLevel_}, 4);
	dispatch();
	textureStuff = select<ramses::TextureSampler>(*sceneContext.scene(), ramses::ERamsesObjectType::ERamsesObjectType_TextureSampler);
	ASSERT_EQ(textureStuff.size(), 1);
}

TEST_F(TextureAdaptorFixture, textureGenerationAtMultipleLevelsWithFlip) {
	auto texture = create<raco::user_types::Texture>("Texture");

	commandInterface.set({texture, &raco::user_types::Texture::uri_}, (test_path() / "images" / "blue_1024.png").string());
	dispatch();

	commandInterface.set({texture, &raco::user_types::Texture::level2uri_}, (test_path() / "images" / "green_512.png").string());
	dispatch();

	commandInterface.set({texture, &raco::user_types::Texture::level3uri_}, (test_path() / "images" / "yellow_256.png").string());
	dispatch();

	commandInterface.set({texture, &raco::user_types::Texture::level4uri_}, (test_path() / "images" / "red_128.png").string());
	dispatch();

	commandInterface.set({texture, &raco::user_types::Texture::mipmapLevel_}, 4);
	dispatch();

	commandInterface.set({texture, &raco::user_types::Texture::flipTexture_}, true);
	dispatch();
	auto textureStuff = select<ramses::TextureSampler>(*sceneContext.scene(), ramses::ERamsesObjectType::ERamsesObjectType_TextureSampler);
	ASSERT_EQ(textureStuff.size(), 1);
}

TEST_F(TextureAdaptorFixture, wrongMipMapLevelImageSizes) {
	auto texture = create<raco::user_types::Texture>("Texture");

	commandInterface.set({texture, &raco::user_types::Texture::mipmapLevel_}, 4);
	dispatch();

	commandInterface.set({texture, &raco::user_types::Texture::uri_}, (test_path() / "images" / "blue_1024.png").string());
	dispatch();

	commandInterface.set({texture, &raco::user_types::Texture::level2uri_}, (test_path() / "images" / "blue_1024.png").string());
	dispatch();

	ASSERT_TRUE(commandInterface.errors().hasError({texture, &raco::user_types::Texture::level2uri_}));

	commandInterface.set({texture, &raco::user_types::Texture::level3uri_}, (test_path() / "images" / "blue_1024.png").string());
	dispatch();

	ASSERT_TRUE(commandInterface.errors().hasError({texture, &raco::user_types::Texture::level3uri_}));

	commandInterface.set({texture, &raco::user_types::Texture::level4uri_}, (test_path() / "images" / "blue_1024.png").string());
	dispatch();

	ASSERT_TRUE(commandInterface.errors().hasError({texture, &raco::user_types::Texture::level4uri_}));

	commandInterface.set({texture, &raco::user_types::Texture::mipmapLevel_}, 3);
	dispatch();

	ASSERT_FALSE(commandInterface.errors().hasError({texture, &raco::user_types::Texture::level4uri_}));

	commandInterface.set({texture, &raco::user_types::Texture::mipmapLevel_}, 2);
	dispatch();

	ASSERT_FALSE(commandInterface.errors().hasError({texture, &raco::user_types::Texture::level3uri_}));

	commandInterface.set({texture, &raco::user_types::Texture::mipmapLevel_}, 1);
	dispatch();

	ASSERT_FALSE(commandInterface.errors().hasError({texture, &raco::user_types::Texture::level2uri_}));
}

TEST_F(TextureAdaptorFixture, ramsesAutoMipMapGenerationWarningPersistsAfterChangingURI) {
	auto texture = create<raco::user_types::Texture>("Texture");

	commandInterface.set({texture, &raco::user_types::Texture::generateMipmaps_}, true);
	commandInterface.set({texture, &raco::user_types::Texture::mipmapLevel_}, 2);
	dispatch();
	ASSERT_TRUE(commandInterface.errors().hasError({texture, &raco::user_types::Texture::mipmapLevel_}));

	commandInterface.set({texture, &raco::user_types::Texture::uri_}, (test_path() / "images" / "blue_1024.png").string());

	dispatch();
	ASSERT_TRUE(commandInterface.errors().hasError({texture, &raco::user_types::Texture::mipmapLevel_}));

	commandInterface.set({texture, &raco::user_types::Texture::level2uri_}, (test_path() / "images" / "green_512.png").string());

	dispatch();
	ASSERT_TRUE(commandInterface.errors().hasError({texture, &raco::user_types::Texture::mipmapLevel_}));
}

TEST_F(TextureAdaptorFixture, textureBitdepthDifferentInSameLevel) {
	auto texture = create<raco::user_types::Texture>("texture");

	commandInterface.set({texture, &raco::user_types::Texture::uri_}, (test_path() / "images" / "green_512_16f.png").string());
	dispatch();

	// RGBA16
	commandInterface.set({texture, &raco::user_types::Texture::textureFormat_}, static_cast<int>(ramses::ETextureFormat::RGBA16F));
	dispatch();
	ASSERT_FALSE(commandInterface.errors().hasError({texture, &raco::user_types::Texture::textureFormat_}));

	commandInterface.set({texture, &raco::user_types::Texture::uri_}, (test_path() / "images" / "green_512.png").string());
	dispatch();
	ASSERT_TRUE(commandInterface.errors().hasError({texture, &raco::user_types::Texture::uri_}));
	ASSERT_EQ(commandInterface.errors().getError({texture, &raco::user_types::Texture::uri_}).level(), raco::core::ErrorLevel::ERROR);

	commandInterface.set({texture, &raco::user_types::Texture::uri_}, (test_path() / "images" / "green_512_16f.png").string());
	dispatch();
	ASSERT_FALSE(commandInterface.errors().hasError({texture, &raco::user_types::Texture::uri_}));
}

TEST_F(TextureAdaptorFixture, textureBitdepthDifferentInOtherLevel) {
	auto texture = create<raco::user_types::Texture>("texture");

	commandInterface.set({texture, &raco::user_types::Texture::uri_}, (test_path() / "images" / "blue_1024_16i.png").string());
	dispatch();

	// RGBA16
	commandInterface.set({texture, &raco::user_types::Texture::textureFormat_}, static_cast<int>(ramses::ETextureFormat::RGBA16F));
	dispatch();
	ASSERT_FALSE(commandInterface.errors().hasError({texture, &raco::user_types::Texture::textureFormat_}));

	commandInterface.set({texture, &raco::user_types::Texture::level2uri_}, (test_path() / "images" / "green_512.png").string());
	commandInterface.set({texture, &raco::user_types::Texture::mipmapLevel_}, 2);
	dispatch();
	ASSERT_TRUE(commandInterface.errors().hasError({texture, &raco::user_types::Texture::level2uri_}));
	ASSERT_EQ(commandInterface.errors().getError({texture, &raco::user_types::Texture::level2uri_}).level(), raco::core::ErrorLevel::ERROR);

	commandInterface.set({texture, &raco::user_types::Texture::level2uri_}, (test_path() / "images" / "green_512_16i.png").string());
	dispatch();
	ASSERT_FALSE(commandInterface.errors().hasError({texture, &raco::user_types::Texture::level2uri_}));
}

TEST_F(TextureAdaptorFixture, textureFormatDifferentInOtherLevel) {
	auto texture = create<raco::user_types::Texture>("texture");

	commandInterface.set({texture, &raco::user_types::Texture::uri_}, (test_path() / "images" / "blue_1024.png").string());
	dispatch();

	// RGBA8
	commandInterface.set({texture, &raco::user_types::Texture::textureFormat_}, static_cast<int>(ramses::ETextureFormat::RGBA8));
	dispatch();

	// R8
	commandInterface.set({texture, &raco::user_types::Texture::level2uri_}, (test_path() / "images" / "green_512_gray.png").string());
	commandInterface.set({texture, &raco::user_types::Texture::mipmapLevel_}, 2);
	dispatch();

	// Swizzled format is R8, compatible with image. No issues.
	ASSERT_FALSE(commandInterface.errors().hasError({texture, &raco::user_types::Texture::level2uri_}));

	// RGBA8
	commandInterface.set({texture, &raco::user_types::Texture::level2uri_}, (test_path() / "images" / "green_512.png").string());
	dispatch();

	// Swizzled format is RGBA8, matching image format.
	ASSERT_FALSE(commandInterface.errors().hasError({texture, &raco::user_types::Texture::level2uri_}));
}


TEST_F(TextureAdaptorFixture, level1UriWarning) {
	auto texture = create<raco::user_types::Texture>("texture");

	dispatch();

	ASSERT_TRUE(commandInterface.errors().hasError({texture, &raco::user_types::Texture::uri_}));
	ASSERT_EQ(commandInterface.errors().getError({texture, &raco::user_types::Texture::uri_}).level(), raco::core::ErrorLevel::WARNING);
}

TEST_F(TextureAdaptorFixture, gitLfsPlaceholderFileInUri) {
	std::string path = test_path().append("images/gitLfsPlaceholderFile.png").string();
	raco::createGitLfsPlaceholderFile(path);

	auto texture = create <raco::user_types::Texture>("texture");
	commandInterface.set({texture, &raco::user_types::Texture::uri_}, path);
	dispatch();
	ASSERT_TRUE(commandInterface.errors().hasError({texture, &raco::user_types::Texture::uri_}));
	ASSERT_TRUE(commandInterface.errors().getError({texture, &raco::user_types::Texture::uri_}).message().find("Git LFS Placeholder"));
}
