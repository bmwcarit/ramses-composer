/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "ramses_adaptor/RenderBufferMSAdaptor.h"
#include "ramses_base/RamsesHandles.h"
#include "ramses_base/EnumerationTranslations.h"
#include "core/BasicAnnotations.h"

namespace raco::ramses_adaptor {

RenderBufferMSAdaptor::RenderBufferMSAdaptor(SceneAdaptor* sceneAdaptor, std::shared_ptr<user_types::RenderBufferMS> editorObject)
	: TypedObjectAdaptor(sceneAdaptor, editorObject, {}),
	  subscriptions_{
		  sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{editorObject, &user_types::RenderBufferMS::width_}, [this]() {
			  tagDirty();
		  }),
		  sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{editorObject, &user_types::RenderBufferMS::height_}, [this]() {
			  tagDirty();
		  }),
		  sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{editorObject, &user_types::RenderBufferMS::format_}, [this]() {
			  tagDirty();
		  }),
		  sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{editorObject, &user_types::RenderBufferMS::sampleCount_}, [this]() {
			  tagDirty();
		  })} {
}

bool RenderBufferMSAdaptor::sync(core::Errors* errors) {
	buffer_.reset();

	auto sampleCount = *editorObject()->sampleCount_;
	if (sampleCount < user_types::RenderBufferMS::SAMPLE_COUNT_MIN || sampleCount > user_types::RenderBufferMS::SAMPLE_COUNT_MAX) {
		reset(nullptr);
		tagDirty(false);
		return true;
	}

	auto format = static_cast<user_types::ERenderBufferFormat>(*editorObject()->format_);
	ramses::ERenderBufferFormat ramsesFormat = ramses_base::enumerationTranslationRenderBufferFormat.at(format);

	bool allValid = true;
	uint32_t clippedWidth = ramses_base::clipAndCheckIntProperty({editorObject_, &user_types::RenderBufferMS::width_}, errors, &allValid);
	uint32_t clippedHeight = ramses_base::clipAndCheckIntProperty({editorObject_, &user_types::RenderBufferMS::height_}, errors, &allValid);

	if (allValid) {
		buffer_ = ramses_base::ramsesRenderBuffer(sceneAdaptor_->scene(),
			clippedWidth, clippedHeight,
			ramsesFormat,
			ramses::ERenderBufferAccessMode::ReadWrite,
			sampleCount,
			(editorObject()->objectName() + "_BufferMS").c_str(),
			editorObject_->objectIDAsRamsesLogicID());
	}

	if (buffer_) {
		auto textureSampler = ramses_base::ramsesTextureSamplerMS(sceneAdaptor_->scene(), buffer_, (editorObject()->objectName() + "_TextureSamplerMS").c_str(), editorObject()->objectIDAsRamsesLogicID());
		reset(std::move(textureSampler));
	} else {
		reset(nullptr);
	}

	tagDirty(false);
	return true;
}

ramses_base::RamsesRenderBuffer RenderBufferMSAdaptor::buffer() const {
	return buffer_;
}

std::vector<ExportInformation> RenderBufferMSAdaptor::getExportInformation() const {
	if (buffer_ == nullptr) {
		return {};
	}

	return {
		ExportInformation{ramsesObject().getType(), ramsesObject().getName()},
		ExportInformation{buffer_->getType(), buffer_->getName()},
	};
}

}  // namespace raco::ramses_adaptor
