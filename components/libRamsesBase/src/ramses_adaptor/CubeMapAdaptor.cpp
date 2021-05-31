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
#include "ramses-client-api/MipLevelData.h"
#include "ramses-utils.h"
#include "ramses_adaptor/SceneAdaptor.h"
#include "ramses_adaptor/TextureSamplerAdaptor.h"
#include "ramses_base/RamsesHandles.h"
#include "user_types/CubeMap.h"
#include "user_types/Enumerations.h"

#include <fstream>
#include <vector>

namespace raco::ramses_adaptor {

CubeMapAdaptor::CubeMapAdaptor(SceneAdaptor* sceneAdaptor, std::shared_ptr<user_types::CubeMap> editorObject)
	: TypedObjectAdaptor(sceneAdaptor, editorObject, {}),
	  subscriptions_{sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{editorObject}.get("wrapUMode"), [this]() {
						 tagDirty();
					 }),
		  sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{editorObject}.get("wrapVMode"), [this]() {
			  tagDirty();
		  }),
		  sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{editorObject}.get("minSamplingMethod"), [this]() {
			  tagDirty();
		  }),
		  sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{editorObject}.get("magSamplingMethod"), [this]() {
			  tagDirty();
		  }),
		  sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{editorObject}.get("anisotropy"), [this]() {
			  tagDirty();
		  }),
		  sceneAdaptor_->dispatcher()->registerOnPreviewDirty(editorObject, [this]() {
			  tagDirty();
		  })} {}

raco::ramses_base::RamsesTextureCube CubeMapAdaptor::createTexture(core::Errors* errors) {
	std::map<std::string, std::vector<unsigned char>> data;
	unsigned int width = -1;
	unsigned int height = -1;

	// check all URIs for error conditions to show them in the UI
	bool allImagesOk = true;
	for (const auto& propName : {"uriFront", "uriBack", "uriLeft", "uriRight", "uriTop", "uriBottom"}) {
		std::string uri = editorObject()->get(propName)->asString();
		if (!uri.empty()) {
			unsigned int curWidth;
			unsigned int curHeight;
			if (0 == lodepng::decode(data[propName], curWidth, curHeight, raco::core::PathQueries::resolveUriPropertyToAbsolutePath(sceneAdaptor_->project(), {editorObject(), {propName}}))) {
				if (curWidth != curHeight) {
					LOG_ERROR(raco::log_system::RAMSES_ADAPTOR, "CubeMap '{}': non-square image '{}' for '{}'", editorObject()->objectName(), uri, propName);
					errors->addError(core::ErrorCategory::PARSE_ERROR, core::ErrorLevel::ERROR, {editorObject()->shared_from_this(), {propName}},
						"None-square image " + std::to_string(curWidth) + "x" + std::to_string(curHeight));
					allImagesOk = false;
				} else {
					if (width == -1) {
						width = curWidth;
						height = curHeight;
					}
					if (width != curWidth || height != curHeight) {
						LOG_ERROR(raco::log_system::RAMSES_ADAPTOR, "CubeMap '{}': incompatible image sizes", editorObject()->objectName());
						errors->addError(core::ErrorCategory::PARSE_ERROR, core::ErrorLevel::ERROR, {editorObject()->shared_from_this(), {propName}},
							"Incompatible image size " + std::to_string(curWidth) + "x" + std::to_string(curHeight) + ", expected is " + std::to_string(width) + "x" + std::to_string(height));
						allImagesOk = false;
					} else {
						errors->removeError({editorObject()->shared_from_this(), {propName}});
					}
				}
			} else {
				LOG_ERROR(raco::log_system::RAMSES_ADAPTOR, "CubeMap '{}': Couldn't load png file from '{}'", editorObject()->objectName(), uri);
				errors->addError(core::ErrorCategory::PARSE_ERROR, core::ErrorLevel::ERROR, {editorObject()->shared_from_this(), {propName}}, "Image file could not be loaded.");
				allImagesOk = false;
			}
		} else {
			allImagesOk = false;
		}
	}
	if (!allImagesOk) {
		return fallbackCube();
	}

	// Order: +x, -X, +Y, -Y, +Z, -Z
	ramses::CubeMipLevelData mipData = ramses::CubeMipLevelData((uint32_t)data["uriRight"].size(),
		data["uriRight"].data(),
		data["uriLeft"].data(),
		data["uriTop"].data(),
		data["uriBottom"].data(),
		data["uriFront"].data(),
		data["uriBack"].data());

	return raco::ramses_base::ramsesTextureCube(sceneAdaptor_->scene(), ramses::ETextureFormat::RGBA8, width, 1u, &mipData, false, {}, ramses::ResourceCacheFlag_DoNotCache);
}

raco::ramses_base::RamsesTextureCube CubeMapAdaptor::fallbackCube() {
	std::map<std::string, std::vector<unsigned char>> data;

	unsigned int w, h;
	lodepng::decode(data["uriRight"], w, h, *TextureSamplerAdaptor::getFallbackTextureData(user_types::TEXTURE_ORIGIN_TOP));
	lodepng::decode(data["uriLeft"], w, h, *TextureSamplerAdaptor::getFallbackTextureData(user_types::TEXTURE_ORIGIN_TOP));
	lodepng::decode(data["uriTop"], w, h, *TextureSamplerAdaptor::getFallbackTextureData(user_types::TEXTURE_ORIGIN_TOP));
	lodepng::decode(data["uriBottom"], w, h, *TextureSamplerAdaptor::getFallbackTextureData(user_types::TEXTURE_ORIGIN_TOP));
	lodepng::decode(data["uriFront"], w, h, *TextureSamplerAdaptor::getFallbackTextureData(user_types::TEXTURE_ORIGIN_TOP));
	lodepng::decode(data["uriBack"], w, h, *TextureSamplerAdaptor::getFallbackTextureData(user_types::TEXTURE_ORIGIN_TOP));

	ramses::CubeMipLevelData mipData = ramses::CubeMipLevelData((uint32_t)data["uriRight"].size(),
		data["uriRight"].data(),
		data["uriLeft"].data(),
		data["uriTop"].data(),
		data["uriBottom"].data(),
		data["uriFront"].data(),
		data["uriBack"].data());

	return raco::ramses_base::ramsesTextureCube(sceneAdaptor_->scene(), ramses::ETextureFormat::RGBA8, w, 1u, &mipData, false, {}, ramses::ResourceCacheFlag_DoNotCache);
}

std::string CubeMapAdaptor::createDefaultTextureDataName() {
	return this->editorObject()->objectName() + "_TextureCube";
}

bool CubeMapAdaptor::sync(core::Errors* errors) {
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
