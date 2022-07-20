/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "ramses_adaptor/TextureSamplerAdaptor.h"
#include "core/ErrorItem.h"
#include <ramses-client-api/MipLevelData.h>
#include "ramses_adaptor/SceneAdaptor.h"
#include "ramses_base/RamsesHandles.h"
#include "user_types/Texture.h"
#include "user_types/Enumerations.h"
#include <QDataStream>
#include <QFile>
#include "lodepng.h"
#include "utils/FileUtils.h"

namespace raco::ramses_adaptor {

using namespace raco::ramses_base;

TextureSamplerAdaptor::TextureSamplerAdaptor(SceneAdaptor* sceneAdaptor, std::shared_ptr<user_types::Texture> editorObject)
	: TypedObjectAdaptor(sceneAdaptor, editorObject, {}),
	  subscriptions_ {sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{editorObject, &user_types::Texture::wrapUMode_}, [this]() {
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
		  sceneAdaptor_->dispatcher()->registerOnPreviewDirty(editorObject, [this]() {
			  tagDirty();
		  })} {}

bool TextureSamplerAdaptor::sync(core::Errors* errors) {
	errors->removeError({editorObject()->shared_from_this()});
	errors->removeError({editorObject()->shared_from_this(), &user_types::Texture::uri_});
	errors->removeError({editorObject()->shared_from_this(), &user_types::Texture::textureFormat_});

	textureData_ = nullptr;
	std::string uri = editorObject()->uri_.asString();
	if (!uri.empty()) {
		// do not clear errors here, this is done earlier in Texture
		textureData_ = createTexture(errors);
		if (!textureData_) {
			LOG_ERROR(raco::log_system::RAMSES_ADAPTOR, "Texture '{}': Couldn't load png file from '{}'", editorObject()->objectName(), uri);
			errors->addError(core::ErrorCategory::PARSE_ERROR, core::ErrorLevel::ERROR, {editorObject()->shared_from_this(), &user_types::Texture::uri_}, "Image file could not be loaded.");
		}
	}

	if (!textureData_) {
		textureData_ = getFallbackTexture();
	} else {
		std::string infoText;

		infoText += "Texture information\n\n";

		infoText += fmt::format("Width: {} px\n", textureData_->getWidth());
		infoText += fmt::format("Height: {} px\n\n", textureData_->getHeight());
		infoText += fmt::format("Format: {}", ramsesTextureFormatToString(textureData_->getTextureFormat()));

		errors->addError(core::ErrorCategory::GENERAL, core::ErrorLevel::INFORMATION, {editorObject()->shared_from_this()}, infoText);
	}

	if (textureData_) {
		textureData_->setName(createDefaultTextureDataName().c_str());
		auto textureSampler = ramsesTextureSampler(sceneAdaptor_->scene(),
			static_cast<ramses::ETextureAddressMode>(*editorObject()->wrapUMode_),
			static_cast<ramses::ETextureAddressMode>(*editorObject()->wrapVMode_),
			static_cast<ramses::ETextureSamplingMethod>(*editorObject()->minSamplingMethod_),
			static_cast<ramses::ETextureSamplingMethod>(*editorObject()->magSamplingMethod_),
			textureData_,
			(*editorObject()->anisotropy_ >= 1 ? *editorObject()->anisotropy_ : 1));
		reset(std::move(textureSampler));
	} else {
		reset(nullptr);
	}

	tagDirty(false);
	return true;
}

RamsesTexture2D TextureSamplerAdaptor::createTexture(core::Errors* errors) {
	std::string pngPath = raco::core::PathQueries::resolveUriPropertyToAbsolutePath(sceneAdaptor_->project(), {editorObject(), &user_types::Texture::uri_});
	unsigned int width = 0;
	unsigned int height = 0;
	std::vector<unsigned char> data;

	auto rawBinaryData = raco::utils::file::readBinary(pngPath);

	lodepng::State pngImportState;
	pngImportState.decoder.color_convert = false;
	lodepng_inspect(&width, &height, &pngImportState, rawBinaryData.data(), rawBinaryData.size());

	auto& lodePngColorInfo = pngImportState.info_png.color;
	auto selectedTextureFormat = static_cast<ramses::ETextureFormat>((*editorObject()->textureFormat_));

	auto textureFormatCompatInfo = validateTextureColorTypeAndBitDepth(selectedTextureFormat, lodePngColorInfo.colortype, lodePngColorInfo.bitdepth);
	if (textureFormatCompatInfo.errorLvl != raco::core::ErrorLevel::NONE) {
		errors->addError(core::ErrorCategory::PARSE_ERROR, textureFormatCompatInfo.errorLvl, {editorObject()->shared_from_this(), &raco::user_types::Texture::textureFormat_}, textureFormatCompatInfo.errorMsg);
		if (textureFormatCompatInfo.errorLvl == raco::core::ErrorLevel::ERROR) {
			return nullptr;
		}
	}

	auto ret = textureFormatCompatInfo.conversionNeeded
		? lodepng::decode(data, width, height, rawBinaryData, static_cast<LodePNGColorType>(ramsesTextureFormatToPngFormat(selectedTextureFormat)), lodePngColorInfo.bitdepth)
		: lodepng::decode(data, width, height, pngImportState, rawBinaryData);
	if (ret != 0) {
		return nullptr;
	}

	if (selectedTextureFormat == ramses::ETextureFormat::RG8) {
		data = raco::ramses_base::generateColorDataWithoutBlueChannel(data);
	}

	// PNG has top left origin. Flip it vertically if required to match U/V origin
	if (*editorObject()->flipTexture_) {
		flipDecodedPicture(data, raco::ramses_base::ramsesTextureFormatToChannelAmount(selectedTextureFormat), width, height, lodePngColorInfo.bitdepth);
	}

	if (lodePngColorInfo.bitdepth == 16) {
		raco::ramses_base::normalize16BitColorData(data);
	}

	ramses::MipLevelData mipLevelData(static_cast<uint32_t>(data.size()), data.data());

	return ramses_base::ramsesTexture2D(sceneAdaptor_->scene(), selectedTextureFormat, width, height, 1, &mipLevelData, *editorObject()->generateMipmaps_, {}, ramses::ResourceCacheFlag_DoNotCache, nullptr);
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

        unsigned int width{0};
        unsigned int height{0};
		lodepng::decode(fallbackTextureData_[0], width, height, sBuffer);
		fallbackTextureData_[1] = fallbackTextureData_[0];
		flipDecodedPicture(fallbackTextureData_[1], 4, width, height, 8);
	}

	return fallbackTextureData_[flipped];
}

}  // namespace raco::ramses_adaptor
