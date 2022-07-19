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

	std::vector<std::map<std::string, std::vector<unsigned char>>> rawMipDatas;
	std::vector<ramses::CubeMipLevelData> mipDatas;
	raco::ramses_base::PngDecodingInfo decodingInfo;

	auto mipMapsOk = true;
	for (auto level = 1; level <= *editorObject()->mipmapLevel_; ++level) {
		auto& mipData = rawMipDatas.emplace_back(generateMipmapData(errors, level, decodingInfo));
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
	infoText.append(fmt::format("Width: {} px\n", decodingInfo.width));
	infoText.append(fmt::format("Height: {} px\n\n", decodingInfo.height));
	infoText.append(fmt::format("PNG Bit depth: {}\n\n", decodingInfo.bitdepth));

	infoText.append(fmt::format("Color channel flow\n"));
	infoText.append(fmt::format("File -> Ramses -> Shader\n"));
	infoText.append(fmt::format("{} -> {} -> {}", decodingInfo.pngColorChannels, decodingInfo.ramsesColorChannels, decodingInfo.shaderColorChannels));

	errors->addError(core::ErrorCategory::GENERAL, core::ErrorLevel::INFORMATION, {editorObject()->shared_from_this()}, infoText);

	return raco::ramses_base::ramsesTextureCube(sceneAdaptor_->scene(), selectedTextureFormat, decodingInfo.width, *editorObject()->mipmapLevel_, mipDatas.data(), *editorObject()->generateMipmaps_, {}, ramses::ResourceCacheFlag_DoNotCache);
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

std::map<std::string, std::vector<unsigned char>> CubeMapAdaptor::generateMipmapData(core::Errors* errors, int level, raco::ramses_base::PngDecodingInfo& decodingInfo) {
	std::map<std::string, std::vector<unsigned char>> data;
	auto mipmapOk = true;

	for (const std::string &propName : {"uriRight", "uriLeft", "uriTop", "uriBottom", "uriFront", "uriBack", }) {
		auto uriPropName = (level > 1) ? fmt::format("level{}{}", level, propName) : propName;
		data[propName] = raco::ramses_base::decodeMipMapData(errors, sceneAdaptor_->project(), editorObject(), uriPropName, level, decodingInfo);

		if (data[propName].empty()) {
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
