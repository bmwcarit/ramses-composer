/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "ramses_adaptor/BlitPassAdaptor.h"

#include "ramses_adaptor/RenderBufferAdaptor.h"
#include "ramses_adaptor/RenderBufferMSAdaptor.h"
#include "ramses_adaptor/SceneAdaptor.h"
#include "ramses_base/Utils.h"

namespace raco::ramses_adaptor {

BlitPassAdaptor::BlitPassAdaptor(SceneAdaptor* sceneAdaptor, user_types::SBlitPass blitPass)
	: UserTypeObjectAdaptor{sceneAdaptor, blitPass},
	  subscriptions_{sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{blitPass, &user_types::BlitPass::sourceRenderBuffer_}, [this]() {
						 recreate_ = true;
						 tagDirty();
					 }),
		  sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{blitPass, &user_types::BlitPass::targetRenderBuffer_}, [this]() {
			  recreate_ = true;
			  tagDirty();
		  }),
		  sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{blitPass, &user_types::BlitPass::sourceRenderBufferMS_}, [this]() {
			  recreate_ = true;
			  tagDirty();
		  }),
		  sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{blitPass, &user_types::BlitPass::targetRenderBufferMS_}, [this]() {
			  recreate_ = true;
			  tagDirty();
		  }),
		  sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{blitPass, &user_types::BlitPass::sourceX_}, [this]() {
			  update_ = true;
			  tagDirty();
		  }),
		  sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{blitPass, &user_types::BlitPass::sourceY_}, [this]() {
			  update_ = true;
			  tagDirty();
		  }),
		  sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{blitPass, &user_types::BlitPass::destinationX_}, [this]() {
			  update_ = true;
			  tagDirty();
		  }),
		  sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{blitPass, &user_types::BlitPass::destinationY_}, [this]() {
			  update_ = true;
			  tagDirty();
		  }),
		  sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{blitPass, &user_types::BlitPass::width_}, [this]() {
			  update_ = true;
			  tagDirty();
		  }),
		  sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{blitPass, &user_types::BlitPass::height_}, [this]() {
			  update_ = true;
			  tagDirty();
		  }),
		  sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{blitPass, &user_types::BlitPass::enabled_}, [this]() {
			  update_ = true;
			  tagDirty();
		  }),
		  sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{blitPass, &user_types::BlitPass::renderOrder_}, [this]() {
			  update_ = true;
			  tagDirty();
		  }),
		  sceneAdaptor_->dispatcher()->registerOnPreviewDirty(blitPass, [this]() {
			  recreate_ = true;
			  tagDirty();
		  })} {
	recreate_ = true;
	update_ = false;
}

bool BlitPassAdaptor::sync(core::Errors* errors) {
	ObjectAdaptor::sync(errors);

	if (recreate_) {
		errors->removeError({editorObject()->shared_from_this()});
		blitPass_.reset();
		ramses_base::RamsesRenderBuffer startBuffer;
		ramses_base::RamsesRenderBuffer endBuffer;
		if (auto srcBuffer = sceneAdaptor_->lookup<ramses_adaptor::RenderBufferAdaptor>(*editorObject_->sourceRenderBuffer_)) {
			startBuffer = srcBuffer->buffer();
		} else if (auto srcBufferMS = sceneAdaptor_->lookup<ramses_adaptor::RenderBufferMSAdaptor>(*editorObject_->sourceRenderBufferMS_)) {
			startBuffer = srcBufferMS->buffer();
		}

		if (auto destBuffer = sceneAdaptor_->lookup<ramses_adaptor::RenderBufferAdaptor>(*editorObject_->targetRenderBuffer_)) {
			endBuffer = destBuffer->buffer();
		} else if (auto destBufferMS = sceneAdaptor_->lookup<ramses_adaptor::RenderBufferMSAdaptor>(*editorObject_->targetRenderBufferMS_)) {
			endBuffer = destBufferMS->buffer();
		}

		if (startBuffer && endBuffer) {
			blitPass_ = ramses_base::ramsesBlitPass(sceneAdaptor_->scene(), startBuffer, endBuffer, editorObject_->objectName().c_str(), editorObject_->objectIDAsRamsesLogicID());

			if (!blitPass_) {
				errors->addError(core::ErrorCategory::GENERAL, core::ErrorLevel::ERROR, {editorObject()->shared_from_this()},
					"BlitPass could not be created, check Ramses logs.");
			}
		}
		recreate_ = false;
		update_ = true;
	}

	if (blitPass_ && update_) {
		errors->removeError({editorObject()->shared_from_this()});
		blitPass_->setEnabled(*editorObject()->enabled_);
		blitPass_->setRenderOrder(*editorObject()->renderOrder_);

		bool allValid = true;
		uint32_t clippedSourceX = ramses_base::clipAndCheckIntProperty({editorObject_, &user_types::BlitPass::sourceX_}, errors, &allValid);
		uint32_t clippedSourceY = ramses_base::clipAndCheckIntProperty({editorObject_, &user_types::BlitPass::sourceY_}, errors, &allValid);
		uint32_t clippedDestinationX = ramses_base::clipAndCheckIntProperty({editorObject_, &user_types::BlitPass::destinationX_}, errors, &allValid);
		uint32_t clippedDestinationY = ramses_base::clipAndCheckIntProperty({editorObject_, &user_types::BlitPass::destinationY_}, errors, &allValid);
		uint32_t clippedWidth = ramses_base::clipAndCheckIntProperty({editorObject_, &user_types::BlitPass::width_}, errors, &allValid);
		uint32_t clippedHeight = ramses_base::clipAndCheckIntProperty({editorObject_, &user_types::BlitPass::height_}, errors, &allValid);

		if (allValid) {
			if (!blitPass_->setBlittingRegion(clippedSourceX, clippedSourceY, clippedDestinationX, clippedDestinationY, clippedWidth, clippedHeight)) {
				errors->addError(core::ErrorCategory::GENERAL, core::ErrorLevel::ERROR, {editorObject()->shared_from_this()},
					sceneAdaptor_->scene()->getRamsesClient().getRamsesFramework().getLastError().value().message);
			}
		}
	}
	update_ = false;

	return true;
}

std::vector<ExportInformation> BlitPassAdaptor::getExportInformation() const {
	auto result = std::vector<ExportInformation>();
	if (blitPass_ != nullptr) {
		result.emplace_back(blitPass_->getType(), blitPass_->getName());
	}

	return result;
}

}  // namespace raco::ramses_adaptor
