/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "ramses_adaptor/RenderBufferAdaptor.h"
#include "ramses_base/RamsesHandles.h"
#include "ramses_base/Utils.h"
#include "core/BasicAnnotations.h"

namespace raco::ramses_adaptor {

RenderBufferAdaptor::RenderBufferAdaptor(SceneAdaptor* sceneAdaptor, std::shared_ptr<user_types::RenderBuffer> editorObject)
	: TypedObjectAdaptor(sceneAdaptor, editorObject, {}),
	  subscriptions_{sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{editorObject, &user_types::RenderBuffer::wrapUMode_}, [this]() {
						 tagDirty();
					 }),
		  sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{editorObject, &user_types::RenderBuffer::wrapVMode_}, [this]() {
			  tagDirty();
		  }),
		  sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{editorObject, &user_types::RenderBuffer::minSamplingMethod_}, [this]() {
			  tagDirty();
		  }),
		  sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{editorObject, &user_types::RenderBuffer::magSamplingMethod_}, [this]() {
			  tagDirty();
		  }),
		  sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{editorObject, &user_types::RenderBuffer::anisotropy_}, [this]() {
			  tagDirty();
		  }),
		  sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{editorObject, &user_types::RenderBuffer::width_}, [this]() {
			  tagDirty();
		  }),
		  sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{editorObject, &user_types::RenderBuffer::height_}, [this]() {
			  tagDirty();
		  }),
		  sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{editorObject, &user_types::RenderBuffer::format_}, [this]() {
			  tagDirty();
		  })} {
}

bool RenderBufferAdaptor::sync(core::Errors* errors) {
	buffer_.reset();

	ramses::ERenderBufferFormat format = static_cast<ramses::ERenderBufferFormat>(*editorObject()->format_);
	ramses::ERenderBufferType type = raco::ramses_base::ramsesRenderBufferTypeFromFormat(format);

	bool allValid = true;
	uint32_t clippedWidth = raco::ramses_base::clipAndCheckIntProperty({editorObject_, &raco::user_types::RenderBuffer::width_}, errors, &allValid);
	uint32_t clippedHeight = raco::ramses_base::clipAndCheckIntProperty({editorObject_, &raco::user_types::RenderBuffer::height_}, errors, &allValid);

	if (allValid) {
		buffer_ = raco::ramses_base::ramsesRenderBuffer(sceneAdaptor_->scene(),
			clippedWidth, clippedHeight,
			type,
			format,
			ramses::ERenderBufferAccessMode_ReadWrite,
			0U,
			(editorObject()->objectName() + "_Buffer").c_str());
	}

	if (buffer_) {
		// For depth buffers, the UI does not display the sampler parameters - so force them to default.
		// Using depth buffers directly as textures is not recommended.
		ramses_base::RamsesTextureSampler textureSampler;
		if (type == ramses::ERenderBufferType_Color) {
			textureSampler = ramses_base::ramsesTextureSampler(sceneAdaptor_->scene(),
				static_cast<ramses::ETextureAddressMode>(*editorObject()->wrapUMode_),
				static_cast<ramses::ETextureAddressMode>(*editorObject()->wrapVMode_),
				static_cast<ramses::ETextureSamplingMethod>(*editorObject()->minSamplingMethod_),
				static_cast<ramses::ETextureSamplingMethod>(*editorObject()->magSamplingMethod_),
				buffer_,
				(*editorObject()->anisotropy_ >= 1 ? *editorObject()->anisotropy_ : 1));			
		} else {
			textureSampler = ramses_base::ramsesTextureSampler(sceneAdaptor_->scene(),
				ramses::ETextureAddressMode_Clamp,
				ramses::ETextureAddressMode_Clamp,
				ramses::ETextureSamplingMethod_Nearest,
				ramses::ETextureSamplingMethod_Nearest,
				buffer_,
				1);			
		}

		reset(std::move(textureSampler));
	} else {
		reset(nullptr);
	}

	tagDirty(false);
	return true;
}

ramses_base::RamsesRenderBuffer RenderBufferAdaptor::buffer() const {
	return buffer_;
}

std::vector<ExportInformation> RenderBufferAdaptor::getExportInformation() const {
	if (buffer_ == nullptr) {
		return {};
	}

	return {
		ExportInformation{ramsesObject().getType(), ramsesObject().getName()},
		ExportInformation{buffer_->getType(), buffer_->getName()},
	};
}

}  // namespace raco::ramses_adaptor
