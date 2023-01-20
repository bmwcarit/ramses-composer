/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "ramses_adaptor/RenderTargetAdaptor.h"
#include "ramses_adaptor/RenderBufferAdaptor.h"
#include "ramses_adaptor/RenderBufferMSAdaptor.h"
#include "ramses_base/RamsesHandles.h"


namespace raco::ramses_adaptor {

RenderTargetAdaptor::RenderTargetAdaptor(SceneAdaptor* sceneAdaptor, std::shared_ptr<user_types::RenderTarget> editorObject)
	: TypedObjectAdaptor(sceneAdaptor, editorObject, {}),
	  subscriptions_{sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{editorObject, &user_types::RenderTarget::buffer0_}, [this]() { tagDirty();  }),
		  sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{editorObject, &user_types::RenderTarget::buffer1_}, [this]() { tagDirty(); }),
		  sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{editorObject, &user_types::RenderTarget::buffer2_}, [this]() { tagDirty(); }),
		  sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{editorObject, &user_types::RenderTarget::buffer3_}, [this]() { tagDirty(); }),
		  sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{editorObject, &user_types::RenderTarget::buffer4_}, [this]() { tagDirty(); }),
		  sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{editorObject, &user_types::RenderTarget::buffer5_}, [this]() { tagDirty(); }),
		  sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{editorObject, &user_types::RenderTarget::buffer6_}, [this]() { tagDirty(); }),
		  sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{editorObject, &user_types::RenderTarget::buffer7_}, [this]() { tagDirty(); }),
		  sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{editorObject, &user_types::RenderTarget::bufferMS0_}, [this]() { tagDirty(); }),
		  sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{editorObject, &user_types::RenderTarget::bufferMS1_}, [this]() { tagDirty(); }),
		  sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{editorObject, &user_types::RenderTarget::bufferMS2_}, [this]() { tagDirty(); }),
		  sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{editorObject, &user_types::RenderTarget::bufferMS3_}, [this]() { tagDirty(); }),
		  sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{editorObject, &user_types::RenderTarget::bufferMS4_}, [this]() { tagDirty(); }),
		  sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{editorObject, &user_types::RenderTarget::bufferMS5_}, [this]() { tagDirty(); }),
		  sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{editorObject, &user_types::RenderTarget::bufferMS6_}, [this]() { tagDirty(); }),
		  sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{editorObject, &user_types::RenderTarget::bufferMS7_}, [this]() { tagDirty(); })} {
}

template <typename RenderBufferAdaptorClass>
bool RenderTargetAdaptor::collectBuffers(std::vector<ramses_base::RamsesRenderBuffer>& buffers, const std::initializer_list<raco::core::SEditorObject>& userTypeBuffers, ramses::RenderTargetDescription& rtDesc, core::Errors* errors) {
	bool hasEmptySlots = false;
	for (int bufferSlotIndex = 0; bufferSlotIndex < userTypeBuffers.size(); ++bufferSlotIndex) {
		const auto& buffer = userTypeBuffers.begin()[bufferSlotIndex];
		if (auto adaptor = sceneAdaptor_->lookup<RenderBufferAdaptorClass>(buffer)) {
			if (auto ramsesBuffer = adaptor->buffer()) {
				auto status = rtDesc.addRenderBuffer(*ramsesBuffer);
				if (status != ramses::StatusOK) {
					LOG_ERROR(raco::log_system::RAMSES_ADAPTOR, rtDesc.getStatusMessage(status));
					errors->addError(core::ErrorCategory::PARSING, core::ErrorLevel::ERROR, {editorObject()->shared_from_this()},
						rtDesc.getStatusMessage(status));
				} else {
					hasEmptySlots = buffers.size() != bufferSlotIndex;
					buffers.emplace_back(ramsesBuffer);
				}
			}
		}
	}
	return hasEmptySlots;
}

bool RenderTargetAdaptor::sync(core::Errors* errors) {
	errors->removeError({editorObject()});

	std::vector<ramses_base::RamsesRenderBuffer> buffers;
	std::vector<ramses_base::RamsesRenderBuffer> buffersMS;
	ramses::RenderTargetDescription rtDesc;

	std::initializer_list<raco::core::SEditorObject> usertypebuffers = {
		*editorObject().get()->user_types::RenderTarget::buffer0_,
		*editorObject().get()->user_types::RenderTarget::buffer1_,
		*editorObject().get()->user_types::RenderTarget::buffer2_,
		*editorObject().get()->user_types::RenderTarget::buffer3_,
		*editorObject().get()->user_types::RenderTarget::buffer4_,
		*editorObject().get()->user_types::RenderTarget::buffer5_,
		*editorObject().get()->user_types::RenderTarget::buffer6_,
		*editorObject().get()->user_types::RenderTarget::buffer7_};

	std::initializer_list<raco::core::SEditorObject> usertypebuffersMS = {
		*editorObject().get()->user_types::RenderTarget::bufferMS0_,
		*editorObject().get()->user_types::RenderTarget::bufferMS1_,
		*editorObject().get()->user_types::RenderTarget::bufferMS2_,
		*editorObject().get()->user_types::RenderTarget::bufferMS3_,
		*editorObject().get()->user_types::RenderTarget::bufferMS4_,
		*editorObject().get()->user_types::RenderTarget::bufferMS5_,
		*editorObject().get()->user_types::RenderTarget::bufferMS6_,
		*editorObject().get()->user_types::RenderTarget::bufferMS7_};

	// We cannot have any empty slots before color buffers in Ramses -
	// the RenderTargetDescription::addRenderBuffer does not allow for it.
	// But if we only add valid render buffers to the list of render buffers,
	// it will shift all later render buffers to the front, changing their index
	// (which is not what the shader will expect).
	// This is a Ramses bug - see https://github.com/bmwcarit/ramses/issues/52
	// If that occurs, refuse to create the render target to avoid any surprises for the user.

	bool haveNormalBuffers = std::any_of(usertypebuffers.begin(), usertypebuffers.end(), [](auto object) {
		return object != nullptr;
	});
	bool haveMSBuffers = std::any_of(usertypebuffersMS.begin(), usertypebuffersMS.end(), [](auto object) {
		return object != nullptr;
	});

	if (haveNormalBuffers && haveMSBuffers) {
		auto errorMsg = fmt::format("Cannot create render target '{}' - all buffers need to be either single-sample or multi-sample only.", editorObject()->objectName());
		reset(nullptr);
		LOG_ERROR(raco::log_system::RAMSES_ADAPTOR, errorMsg);
		errors->addError(core::ErrorCategory::GENERAL, core::ErrorLevel::ERROR, {editorObject()}, errorMsg);

		tagDirty(false);
		return true;
	}

	bool hasEmptySlotsNormal = collectBuffers<RenderBufferAdaptor>(buffers, usertypebuffers, rtDesc, errors);
	bool hasEmptySlotsMS = collectBuffers<RenderBufferMSAdaptor>(buffersMS, usertypebuffersMS, rtDesc, errors);
	bool hasEmptySlots = hasEmptySlotsNormal || hasEmptySlotsMS;

	buffers.insert(buffers.end(), buffersMS.begin(), buffersMS.end());

	if (!buffers.empty() && !hasEmptySlots) {
		reset(ramses_base::ramsesRenderTarget(sceneAdaptor_->scene(), rtDesc, buffers));
	} else if (!errors->hasError({editorObject()})) {
		std::string errorMsg;
		if (buffers.empty()) {
			errorMsg = fmt::format("Cannot create render target '{}' - its first buffer is not set or not valid.", editorObject()->objectName());			
		} else {
			errorMsg = fmt::format("Cannot create render target '{}' - all buffers in it must be consecutive and valid buffers.", editorObject()->objectName());					
		}
		reset(nullptr);
		LOG_ERROR(raco::log_system::RAMSES_ADAPTOR, errorMsg);
		errors->addError(core::ErrorCategory::GENERAL, core::ErrorLevel::ERROR, {editorObject()}, errorMsg);
	}

	tagDirty(false);
	return true;
}

std::vector<ExportInformation> RenderTargetAdaptor::getExportInformation() const {
	if (getRamsesObjectPointer() == nullptr) {
		return {};
	}

	return {ExportInformation{ramsesObject().getType(), ramsesObject().getName()}};
}

};	// namespace raco::ramses_adaptor