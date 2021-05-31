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
#include "ramses-client-api/MipLevelData.h"
#include "ramses-utils.h"
#include "ramses_adaptor/SceneAdaptor.h"
#include "ramses_base/RamsesHandles.h"
#include "user_types/Texture.h"
#include "user_types/Enumerations.h"
#include <QDataStream>
#include <QFile>
#include "lodepng.h"

namespace raco::ramses_adaptor {

using namespace raco::ramses_base;

TextureSamplerAdaptor::TextureSamplerAdaptor(SceneAdaptor* sceneAdaptor, std::shared_ptr<user_types::Texture> editorObject)
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

bool TextureSamplerAdaptor::sync(core::Errors* errors) {
	textureData_ = nullptr;
	std::string uri = editorObject()->uri_.asString();
	if (!uri.empty()) {
		// do not clear errors here, this is done earlier in Texture
		textureData_ = createTexture();
		if (!textureData_) {
			LOG_ERROR(raco::log_system::RAMSES_ADAPTOR, "Texture '{}': Couldn't load png file from '{}'", editorObject()->objectName(), uri);
			errors->addError(core::ErrorCategory::PARSE_ERROR, core::ErrorLevel::ERROR, {editorObject()->shared_from_this(), {"uri"}}, "Image file could not be loaded.");
		}
	}

	if (!textureData_) {
		textureData_ = getFallbackTexture();
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

RamsesTexture2D TextureSamplerAdaptor::createTexture() {
	std::string pngPath = raco::core::PathQueries::resolveUriPropertyToAbsolutePath(sceneAdaptor_->project(), {editorObject(), {"uri"}});

	unsigned int width = 0;
	unsigned int height = 0;
	std::vector<unsigned char> data;

	const unsigned int ret = lodepng::decode(data, width, height, pngPath.c_str());
	if (ret != 0) {
		return nullptr;
	}
	
	// PNG has top left origin. Flip it vertically if required to match U/V origin
	if (*editorObject()->origin_ == raco::user_types::TEXTURE_ORIGIN_BOTTOM) { 
		unsigned lineSize = width * 4;
		for (unsigned y = 0; y < height/2; y++) {
			unsigned lineIndex = y * lineSize;
			unsigned swapIndex = (height - y - 1) * lineSize;
			for (unsigned x = 0; x < lineSize; x++) {
				unsigned char tmp = data[lineIndex + x];
				data[lineIndex + x] = data[swapIndex + x];
				data[swapIndex + x] = tmp;
			}
		}
	} 
	
	ramses::MipLevelData mipLevelData(static_cast<uint32_t>(data.size()), data.data());
	ramses::Texture2D* textureData = sceneAdaptor_->scene()->createTexture2D(ramses::ETextureFormat::RGBA8, width, height, 1, &mipLevelData, false, {}, ramses::ResourceCacheFlag_DoNotCache, nullptr);

	return {textureData, createRamsesObjectDeleter<ramses::Texture2D>(sceneAdaptor_->scene())};
}

RamsesTexture2D TextureSamplerAdaptor::getFallbackTexture() {
	std::vector<unsigned char>* data = getFallbackTextureData(*editorObject()->origin_);
	if (data) {
		return ramsesTexture2DFromPngBuffer(sceneAdaptor_->scene(), *data);
	}

	return nullptr;
}

std::string TextureSamplerAdaptor::createDefaultTextureDataName() {
	return this->editorObject()->objectName() + "_Texture2D";
}

std::vector<unsigned char>* TextureSamplerAdaptor::getFallbackTextureData(int originMode) {
	if (originMode < user_types::TEXTURE_ORIGIN_BOTTOM || originMode > user_types::TEXTURE_ORIGIN_TOP) {
		return nullptr;
	}
	if (fallbackTextureData_[originMode].empty()) {
		QFile file( originMode ? ":fallbackTextureDirectX" : ":fallbackTextureOpenGL" );
		if (file.exists()) {
			auto size = file.size();
			file.open(QIODevice::ReadOnly);
			QDataStream in(&file);
			std::vector<char> sBuffer(size);
			in.readRawData(&sBuffer[0], size);
			fallbackTextureData_[originMode] = std::vector<unsigned char>(sBuffer.begin(), sBuffer.end());
			file.close();
		} else {
			return nullptr;
		}
	}
	return &fallbackTextureData_[originMode];
}

}  // namespace raco::ramses_adaptor
