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
	if (sampleCount < raco::user_types::RenderBufferMS::SAMPLE_COUNT_MIN || sampleCount > raco::user_types::RenderBufferMS::SAMPLE_COUNT_MAX) {
		reset(nullptr);
		tagDirty(false);
		return true;
	}

	ramses::ERenderBufferFormat format = static_cast<ramses::ERenderBufferFormat>(*editorObject()->format_);
	ramses::ERenderBufferType type = raco::ramses_base::ramsesRenderBufferTypeFromFormat(format);

	bool allValid = true;
	uint32_t clippedWidth = raco::ramses_base::clipAndCheckIntProperty({editorObject_, &raco::user_types::RenderBufferMS::width_}, errors, &allValid);
	uint32_t clippedHeight = raco::ramses_base::clipAndCheckIntProperty({editorObject_, &raco::user_types::RenderBufferMS::height_}, errors, &allValid);

	if (allValid) {
		buffer_ = raco::ramses_base::ramsesRenderBuffer(sceneAdaptor_->scene(),
			clippedWidth, clippedHeight,
			type,
			format,
			ramses::ERenderBufferAccessMode_ReadWrite,
			sampleCount,
			(editorObject()->objectName() + "_BufferMS").c_str());
	}

	if (buffer_) {
		auto textureSampler = ramses_base::ramsesTextureSamplerMS(sceneAdaptor_->scene(), buffer_, (editorObject()->objectName() + "_TextureSamplerMS").c_str());
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
