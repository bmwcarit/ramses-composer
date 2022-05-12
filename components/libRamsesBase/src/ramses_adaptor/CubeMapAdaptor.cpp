/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "ramses_adaptor/CubeMapAdaptor.h"
#include "core/CoreFormatter.h"
#include "lodepng.h"
#include "ramses_adaptor/SceneAdaptor.h"
#include "ramses_adaptor/TextureSamplerAdaptor.h"
#include "ramses_base/RamsesHandles.h"
#include "ramses_base/Utils.h"
#include "user_types/CubeMap.h"
#include "user_types/Enumerations.h"
#include "utils/FileUtils.h"

#include <ramses-client-api/MipLevelData.h>

#include <fstream>
#include <vector>

namespace raco::ramses_adaptor {

CubeMapAdaptor::CubeMapAdaptor(SceneAdaptor* sceneAdaptor, std::shared_ptr<user_types::CubeMap> editorObject)
	: TypedObjectAdaptor(sceneAdaptor, editorObject, {}),
	  subscriptions_{sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{editorObject, &user_types::CubeMap::wrapUMode_}, [this]() {
						 tagDirty();
					 }),
		  sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{editorObject, &user_types::CubeMap::wrapVMode_}, [this]() {
			  tagDirty();
		  }),
		  sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{editorObject, &user_types::CubeMap::minSamplingMethod_}, [this]() {
			  tagDirty();
		  }),
		  sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{editorObject, &user_types::CubeMap::magSamplingMethod_}, [this]() {
			  tagDirty();
		  }),
		  sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{editorObject, &user_types::CubeMap::anisotropy_}, [this]() {
			  tagDirty();
		  }),
		  sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{editorObject, &user_types::CubeMap::generateMipmaps_}, [this]() {
			  tagDirty();
		  }),
		  sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{editorObject, &user_types::CubeMap::mipmapLevel_}, [this]() {
			  tagDirty();
		  }),
		  sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{editorObject, &user_types::CubeMap::textureFormat_}, [this]() {
			  tagDirty();
		  }),
		  sceneAdaptor_->dispatcher()->registerOnPreviewDirty(editorObject, [this]() {
			  tagDirty();
		  })} {}

raco::ramses_base::RamsesTextureCube CubeMapAdaptor::createTexture(core::Errors* errors) {
	if (*editorObject()->mipmapLevel_ < 1 || *editorObject()->mipmapLevel_ > 4) {
		return fallbackCube();
	}

	auto width = -1;
	auto height = -1;
	auto bitdepth = -1;
	auto colorType = -1;

	std::vector<std::map<std::string, std::vector<unsigned char>>> rawMipDatas;
	std::vector<ramses::CubeMipLevelData> mipDatas;

	auto mipMapsOk = true;
	for (int level = 1; level <= *editorObject()->mipmapLevel_; ++level) {
		auto& mipData = rawMipDatas.emplace_back(generateMipmapData(errors, level, width, height, bitdepth));
		if (mipData.empty()) {
			mipMapsOk = false;
		}
	}

	if (!mipMapsOk) {
		return fallbackCube();
	}

	for (auto& mipData : rawMipDatas) {
		// Order: +X, -X, +Y, -Y, +Z, -Z
		mipDatas.emplace_back(ramses::CubeMipLevelData(static_cast<uint32_t>(mipData["uriRight"].size()),
			mipData["uriRight"].data(),
			mipData["uriLeft"].data(),
			mipData["uriTop"].data(),
			mipData["uriBottom"].data(),
			mipData["uriFront"].data(),
			mipData["uriBack"].data()));
	}

	auto selectedTextureFormat = static_cast<ramses::ETextureFormat>((*editorObject()->textureFormat_));

	std::string infoText = "CubeMap information\n\n";
	infoText.append(fmt::format("Width: {} px\n", width));
	infoText.append(fmt::format("Height: {} px\n\n", height));
	infoText.append(fmt::format("Format: {}", raco::ramses_base::ramsesTextureFormatToString(selectedTextureFormat)));

	errors->addError(core::ErrorCategory::GENERAL, core::ErrorLevel::INFORMATION, {editorObject()->shared_from_this()}, infoText);

	return raco::ramses_base::ramsesTextureCube(sceneAdaptor_->scene(), selectedTextureFormat, width, *editorObject()->mipmapLevel_, mipDatas.data(), *editorObject()->generateMipmaps_, {}, ramses::ResourceCacheFlag_DoNotCache);
}

raco::ramses_base::RamsesTextureCube CubeMapAdaptor::fallbackCube() {
	std::map<std::string, std::vector<unsigned char>> data;

	data["uriRight"] = TextureSamplerAdaptor::getFallbackTextureData(false);
	data["uriLeft"] = TextureSamplerAdaptor::getFallbackTextureData(false);
	data["uriTop"] = TextureSamplerAdaptor::getFallbackTextureData(false);
	data["uriBottom"] = TextureSamplerAdaptor::getFallbackTextureData(false);
	data["uriFront"] = TextureSamplerAdaptor::getFallbackTextureData(false);
	data["uriBack"] = TextureSamplerAdaptor::getFallbackTextureData(false);

	ramses::CubeMipLevelData mipData = ramses::CubeMipLevelData((uint32_t)data["uriRight"].size(),
		data["uriRight"].data(),
		data["uriLeft"].data(),
		data["uriTop"].data(),
		data["uriBottom"].data(),
		data["uriFront"].data(),
		data["uriBack"].data());

	return raco::ramses_base::ramsesTextureCube(sceneAdaptor_->scene(), ramses::ETextureFormat::RGBA8, TextureSamplerAdaptor::FALLBACK_TEXTURE_SIZE_PX, 1u, &mipData, *editorObject()->generateMipmaps_, {}, ramses::ResourceCacheFlag_DoNotCache);
}

std::string CubeMapAdaptor::createDefaultTextureDataName() {
	return this->editorObject()->objectName() + "_TextureCube";
}

std::map<std::string, std::vector<unsigned char>> CubeMapAdaptor::generateMipmapData(core::Errors* errors, int level, int& width, int& height, int& bitdepth) {
	std::map<std::string, std::vector<unsigned char>> data;
	auto mipmapOk = true;

	auto getUriPropName = [](const auto& propName, int level) {
		if (level > 1) {
			return fmt::format("level{}{}", level, propName);
		}
		return propName;
	};

	for (std::string propName : {"uriRight", "uriLeft", "uriTop", "uriBottom", "uriFront", "uriBack", }) {
		std::string uri = editorObject()->get(getUriPropName(propName, level))->asString();
		if (!uri.empty()) {
			auto selectedTextureFormat = static_cast<ramses::ETextureFormat>((*editorObject()->textureFormat_));
			unsigned int curWidth;
			unsigned int curHeight;

			auto pngPath = raco::core::PathQueries::resolveUriPropertyToAbsolutePath(sceneAdaptor_->project(), {editorObject(), {getUriPropName(propName, level)}});
			auto rawBinaryData = raco::utils::file::readBinary(pngPath);
			lodepng::State pngImportState;
			pngImportState.decoder.color_convert = false;
			lodepng_inspect(&curWidth, &curHeight, &pngImportState, rawBinaryData.data(), rawBinaryData.size());

			auto& lodePngColorInfo = pngImportState.info_png.color;
			auto textureFormatCompatInfo = raco::ramses_base::validateTextureColorTypeAndBitDepth(selectedTextureFormat, lodePngColorInfo.colortype, lodePngColorInfo.bitdepth);

			auto ret = textureFormatCompatInfo.conversionNeeded
				? lodepng::decode(data[propName], curWidth, curHeight, rawBinaryData, static_cast<LodePNGColorType>(raco::ramses_base::ramsesTextureFormatToPngFormat(selectedTextureFormat)), lodePngColorInfo.bitdepth)
				: lodepng::decode(data[propName], curWidth, curHeight, pngImportState, rawBinaryData);

			if (ret == 0) {
				if (curWidth != curHeight) {
					LOG_ERROR(raco::log_system::RAMSES_ADAPTOR, "CubeMap '{}': non-square image '{}' for '{}'", editorObject()->objectName(), uri, getUriPropName(propName, level));
					errors->addError(core::ErrorCategory::PARSE_ERROR, core::ErrorLevel::ERROR, {editorObject()->shared_from_this(), {getUriPropName(propName, level)}},
									 fmt::format("Non-square image size {}x{}", curWidth, curHeight));
					mipmapOk = false;
				} else {
					auto curBitDepth = pngImportState.info_png.color.bitdepth;

					if (level == 1 && width == -1) {
						width = curWidth;
						height = curHeight;
						bitdepth = curBitDepth;
					}

					if ((level == 1 && (width != curWidth || height != curHeight)) || (curWidth != width * std::pow(0.5, level - 1) || curHeight != height * std::pow(0.5, level - 1))) {
						LOG_ERROR(raco::log_system::RAMSES_ADAPTOR, "CubeMap '{}': incompatible image sizes", editorObject()->objectName());
						auto errorMsg = (width == -1)
											? "Level 1 mipmap not defined"
											: fmt::format("Incompatible image size {}x{}, expected is {}x{}", curWidth, curHeight, static_cast<int>(width * std::pow(0.5, level - 1)), static_cast<int>(height * std::pow(0.5, level - 1)));
						errors->addError(core::ErrorCategory::PARSE_ERROR, core::ErrorLevel::ERROR, {editorObject()->shared_from_this(), {getUriPropName(propName, level)}}, errorMsg);
						mipmapOk = false;
					} else if (curBitDepth != bitdepth) {
						auto errorMsg = fmt::format("Incompatible image bit depth {}, expected is {}", curBitDepth, bitdepth);
						errors->addError(core::ErrorCategory::PARSE_ERROR, core::ErrorLevel::ERROR, {editorObject()->shared_from_this(), {getUriPropName(propName, level)}}, errorMsg);
						mipmapOk = false;
					} else if (textureFormatCompatInfo.errorLvl != raco::core::ErrorLevel::NONE) {
						errors->addError(core::ErrorCategory::PARSE_ERROR, textureFormatCompatInfo.errorLvl, {editorObject()->shared_from_this(), {getUriPropName(propName, level)}}, textureFormatCompatInfo.errorMsg);
						if (textureFormatCompatInfo.errorLvl == raco::core::ErrorLevel::ERROR) {
							mipmapOk = false;
						}
					} else {
						errors->removeError({editorObject()->shared_from_this(), {getUriPropName(propName, level)}});
					}
				}

				if (bitdepth == 16) {
					raco::ramses_base::normalize16BitColorData(data[propName]);
				} else if (selectedTextureFormat == ramses::ETextureFormat::RG8) {
					data[propName] = raco::ramses_base::generateColorDataWithoutBlueChannel(data[propName]);
				}

			} else {
				LOG_ERROR(raco::log_system::RAMSES_ADAPTOR, "CubeMap '{}': Couldn't load png file from '{}'", editorObject()->objectName(), uri);
				errors->addError(core::ErrorCategory::PARSE_ERROR, core::ErrorLevel::ERROR, {editorObject()->shared_from_this(), {getUriPropName(propName, level)}}, "Image file could not be loaded.");
				mipmapOk = false;
			}
		} else {
			mipmapOk = false;
		}
	}

	if (!mipmapOk) {
		data.clear();
	}

	return data;
}

bool CubeMapAdaptor::sync(core::Errors* errors) {
	errors->removeError({editorObject()->shared_from_this()});
	errors->removeError({editorObject()->shared_from_this(), &raco::user_types::CubeMap::textureFormat_});

	textureData_.reset();

	textureData_ = createTexture(errors);

	if (textureData_) {
		textureData_->setName(createDefaultTextureDataName().c_str());
		auto textureSampler = raco::ramses_base::ramsesTextureSampler(sceneAdaptor_->scene(),
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

}  // namespace raco::ramses_adaptor
