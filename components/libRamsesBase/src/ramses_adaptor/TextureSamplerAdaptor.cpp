/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "ramses_adaptor/TextureSamplerAdaptor.h"
#include "core/ErrorItem.h"
#include "lodepng.h"
#include "ramses_adaptor/SceneAdaptor.h"
#include "ramses_base/RamsesHandles.h"
#include "user_types/Enumerations.h"
#include "user_types/Texture.h"
#include "utils/FileUtils.h"
#include <QDataStream>
#include <QFile>
#include <ramses-client-api/MipLevelData.h>

namespace raco::ramses_adaptor {

using namespace raco::ramses_base;

TextureSamplerAdaptor::TextureSamplerAdaptor(SceneAdaptor* sceneAdaptor, std::shared_ptr<user_types::Texture> editorObject)
	: TypedObjectAdaptor(sceneAdaptor, editorObject, {}),
	  subscriptions_{sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{editorObject, &user_types::Texture::wrapUMode_}, [this]() {
						 tagDirty();
					 }),
		  sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{editorObject, &user_types::Texture::wrapVMode_}, [this]() {
			  tagDirty();
		  }),
		  sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{editorObject, &user_types::Texture::minSamplingMethod_}, [this]() {
			  tagDirty();
		  }),
		  sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{editorObject, &user_types::Texture::magSamplingMethod_}, [this]() {
			  tagDirty();
		  }),
		  sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{editorObject, &user_types::Texture::anisotropy_}, [this]() {
			  tagDirty();
		  }),
		  sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{editorObject, &user_types::Texture::textureFormat_}, [this]() {
			  tagDirty();
		  }),
		  sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{editorObject, &user_types::Texture::flipTexture_}, [this]() {
			  tagDirty();
		  }),
		  sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{editorObject, &user_types::Texture::generateMipmaps_}, [this]() {
			  tagDirty();
		  }),
		  sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{editorObject, &user_types::Texture::mipmapLevel_}, [this]() {
			  tagDirty();
		  }),
		  sceneAdaptor_->dispatcher()->registerOnPreviewDirty(editorObject, [this]() {
			  tagDirty();
		  })} {}

bool TextureSamplerAdaptor::sync(core::Errors* errors) {
	errors->removeError({editorObject()->shared_from_this()});
	errors->removeError({editorObject()->shared_from_this(), &user_types::Texture::textureFormat_});

	raco::ramses_base::PngDecodingInfo decodingInfo;
	textureData_ = nullptr;
	std::string uri = editorObject()->uri_.asString();
	if (!uri.empty()) {
		// do not clear errors here, this is done earlier in Texture
		textureData_ = createTexture(errors, decodingInfo);
		if (!textureData_) {
			LOG_ERROR(raco::log_system::RAMSES_ADAPTOR, "Texture '{}': Couldn't load png file from '{}'", editorObject()->objectName(), uri);
			errors->addError(core::ErrorCategory::PARSING, core::ErrorLevel::ERROR, {editorObject()->shared_from_this(), &user_types::Texture::uri_}, "Image file could not be loaded.");
		}
	}

	if (!textureData_) {
		textureData_ = getFallbackTexture();
	} else {
		auto selectedTextureFormat = static_cast<user_types::ETextureFormat>((*editorObject()->textureFormat_));

		std::string infoText = "Texture information\n\n";
		infoText.append(fmt::format("Width: {} px\n", textureData_->getWidth()));
		infoText.append(fmt::format("Height: {} px\n\n", textureData_->getHeight()));
		infoText.append(fmt::format("PNG Bit depth: {}\n\n", decodingInfo.bitdepth));

		infoText.append(fmt::format("Color channel flow\n"));
		infoText.append(fmt::format("File -> Ramses -> Shader\n"));
		infoText.append(fmt::format("{} -> {} -> {}", decodingInfo.pngColorChannels, decodingInfo.ramsesColorChannels, decodingInfo.shaderColorChannels));
		errors->addError(core::ErrorCategory::GENERAL, core::ErrorLevel::INFORMATION, {editorObject()->shared_from_this()}, infoText);
	}

	if (textureData_) {
		textureData_->setName(createDefaultTextureDataName().c_str());

		auto wrapUMode = static_cast<user_types::ETextureAddressMode>(*editorObject()->wrapUMode_);
		auto ramsesWrapUMode = ramses_base::enumerationTranslationTextureAddressMode.at(wrapUMode);

		auto wrapVMode = static_cast<user_types::ETextureAddressMode>(*editorObject()->wrapVMode_);
		auto ramsesWrapVMode = ramses_base::enumerationTranslationTextureAddressMode.at(wrapVMode);

		auto minSamplMethod = static_cast<user_types::ETextureSamplingMethod>(*editorObject()->minSamplingMethod_);
		auto ramsesMinSamplMethod = ramses_base::enumerationTranslationTextureSamplingMethod.at(minSamplMethod);

		auto magSamplMethod = static_cast<user_types::ETextureSamplingMethod>(*editorObject()->magSamplingMethod_);
		auto ramsesMagSamplMethod = ramses_base::enumerationTranslationTextureSamplingMethod.at(magSamplMethod);

		auto textureSampler = ramsesTextureSampler(sceneAdaptor_->scene(),
			ramsesWrapUMode,
			ramsesWrapVMode,
			ramsesMinSamplMethod,
			ramsesMagSamplMethod,
			textureData_,
			(*editorObject()->anisotropy_ >= 1 ? *editorObject()->anisotropy_ : 1));
		reset(std::move(textureSampler));
	} else {
		reset(nullptr);
	}

	tagDirty(false);
	return true;
}

RamsesTexture2D TextureSamplerAdaptor::createTexture(core::Errors* errors, PngDecodingInfo& decodingInfo) {
	if (*editorObject()->mipmapLevel_ < 1 || *editorObject()->mipmapLevel_ > 4) {
		return getFallbackTexture();
	}

	std::vector<std::vector<unsigned char>> rawMipDatas;
	std::vector<ramses::MipLevelData> mipDatas;
	auto mipMapsOk = true;

	for (auto level = 1; level <= *editorObject()->mipmapLevel_; ++level) {
		auto uriPropName = (level > 1) ? fmt::format("level{}{}", level, "uri") : "uri";

		// Raw data is requested in swizzled texture format.
		auto levelMipData = decodeMipMapData(errors, sceneAdaptor_->project(), editorObject(), uriPropName, level, decodingInfo, true);
		if (levelMipData.empty()) {
			mipMapsOk = false;
		}

		rawMipDatas.emplace_back(levelMipData);
	}

	if (!mipMapsOk) {
		return getFallbackTexture();
	}

	// Swizzle is defined by original file format and user-selected texture format.
	const auto userTextureFormat = enumerationTranslationTextureFormat.at(static_cast<user_types::ETextureFormat>(*editorObject()->textureFormat_));

	// Get optimized texture format and swizzle describing how to interpret it.
	const auto& [info, swizzleTextureFormat, swizzle] = ramsesTextureFormatToSwizzleInfo(decodingInfo.originalPngFormat, userTextureFormat);

	for (auto i = 0; i < rawMipDatas.size(); ++i) {
		auto& rawMipData = rawMipDatas[i];

		// PNG has top left origin. Flip it vertically if required to match U/V origin
		if (*editorObject()->flipTexture_) {
			flipDecodedPicture(rawMipData, ramsesTextureFormatToChannelAmount(swizzleTextureFormat), decodingInfo.width * std::pow(0.5, i), decodingInfo.height * std::pow(0.5, i), decodingInfo.bitdepth);
		}
		mipDatas.emplace_back(static_cast<uint32_t>(rawMipData.size()), rawMipData.data());
	}

	return ramsesTexture2D(sceneAdaptor_->scene(), swizzleTextureFormat, decodingInfo.width, decodingInfo.height, *editorObject()->mipmapLevel_, mipDatas.data(), *editorObject()->generateMipmaps_, swizzle, ramses::ResourceCacheFlag_DoNotCache, nullptr);
}

RamsesTexture2D TextureSamplerAdaptor::getFallbackTexture() {
	auto& data = getFallbackTextureData(*editorObject()->flipTexture_);
	ramses::MipLevelData mipLevelData(static_cast<uint32_t>(data.size()), data.data());
	ramses::Texture2D* textureData = sceneAdaptor_->scene()->createTexture2D(ramses::ETextureFormat::RGBA8, FALLBACK_TEXTURE_SIZE_PX, FALLBACK_TEXTURE_SIZE_PX, 1, &mipLevelData, false, {}, ramses::ResourceCacheFlag_DoNotCache, nullptr);

	return {textureData, createRamsesObjectDeleter<ramses::Texture2D>(sceneAdaptor_->scene())};
}

std::string TextureSamplerAdaptor::createDefaultTextureDataName() {
	return this->editorObject()->objectName() + "_Texture2D";
}

void TextureSamplerAdaptor::flipDecodedPicture(std::vector<unsigned char>& rawPictureData, unsigned int availableChannels, unsigned int width, unsigned int height, unsigned int bitdepth) {
	unsigned lineSize = width * availableChannels * (bitdepth / 8);
	for (unsigned y = 0; y < height / 2; y++) {
		unsigned lineIndex = y * lineSize;
		unsigned swapIndex = (height - y - 1) * lineSize;
		for (unsigned x = 0; x < lineSize; x++) {
			unsigned char tmp = rawPictureData[lineIndex + x];
			rawPictureData[lineIndex + x] = rawPictureData[swapIndex + x];
			rawPictureData[swapIndex + x] = tmp;
		}
	}
}

std::vector<unsigned char>& TextureSamplerAdaptor::getFallbackTextureData(bool flipped) {
	QFile file(":fallbackTextureOpenGL");
	if (file.exists() && fallbackTextureData_.front().empty()) {
		auto size = file.size();
		file.open(QIODevice::ReadOnly);
		QDataStream in(&file);
		std::vector<unsigned char> sBuffer(size);

		for (auto i = 0; i < size; ++i) {
			in >> sBuffer[i];
		}

		file.close();

		unsigned int width;
		unsigned int height;
		lodepng::decode(fallbackTextureData_[0], width, height, sBuffer);
		fallbackTextureData_[1] = fallbackTextureData_[0];
		flipDecodedPicture(fallbackTextureData_[1], 4, width, height, 8);
	}

	return fallbackTextureData_[flipped];
}

std::vector<ExportInformation> TextureSamplerAdaptor::getExportInformation() const {
	if (textureData_ == nullptr) {
		return {};
	}

	return {
		ExportInformation{ramsesObject().getType(), ramsesObject().getName()},
		ExportInformation{textureData_->getType(), textureData_->getName()},
	};
}

}  // namespace raco::ramses_adaptor
