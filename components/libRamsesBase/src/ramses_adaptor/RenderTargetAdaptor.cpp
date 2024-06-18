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
	
template <typename RenderTargetClass, typename RenderBufferClass, typename RenderBufferAdaptorClass>
RenderTargetAdaptorT<RenderTargetClass, RenderBufferClass, RenderBufferAdaptorClass>::RenderTargetAdaptorT(SceneAdaptor* sceneAdaptor, std::shared_ptr<RenderTargetClass> editorObject)
	: TypedObjectAdaptor<RenderTargetClass, ramses::RenderTarget>(sceneAdaptor, editorObject, {}),
	  subscription_{sceneAdaptor->dispatcher()->registerOnChildren(core::ValueHandle{editorObject, &RenderTargetClass::buffers_}, [this](auto) { this->tagDirty(); })} {
}

template <typename RenderTargetClass, typename RenderBufferClass, typename RenderBufferAdaptorClass>
bool RenderTargetAdaptorT<RenderTargetClass, RenderBufferClass, RenderBufferAdaptorClass>::collectBuffers(std::vector<ramses_base::RamsesRenderBuffer>& buffers, const std::vector<std::shared_ptr<RenderBufferClass>>& userTypeBuffers, ramses::RenderTargetDescription& rtDesc, core::Errors* errors) {
	bool hasEmptySlots = false;
	for (int bufferSlotIndex = 0; bufferSlotIndex < userTypeBuffers.size(); ++bufferSlotIndex) {
		const auto& buffer = userTypeBuffers.begin()[bufferSlotIndex];
		if (auto adaptor = this->sceneAdaptor_->template lookup<RenderBufferAdaptorClass>(buffer)) {
			if (auto ramsesBuffer = adaptor->buffer()) {
				std::string errorMsg;
				if (!rtDesc.addRenderBuffer(*ramsesBuffer, &errorMsg)) {
					LOG_ERROR(log_system::RAMSES_ADAPTOR, errorMsg);
					errors->addError(core::ErrorCategory::PARSING, core::ErrorLevel::ERROR, {this->editorObject()->shared_from_this()}, errorMsg);
				} else {
					hasEmptySlots = buffers.size() != bufferSlotIndex;
					buffers.emplace_back(ramsesBuffer);
				}
			}
		}
	}
	return hasEmptySlots;
}

template <typename RenderTargetClass, typename RenderBufferClass, typename RenderBufferAdaptorClass>
bool RenderTargetAdaptorT<RenderTargetClass, RenderBufferClass, RenderBufferAdaptorClass>::sync(core::Errors* errors) {
	errors->removeError({this->editorObject()});

	std::vector<ramses_base::RamsesRenderBuffer> buffers;
	ramses::RenderTargetDescription rtDesc;

	// We cannot have any empty slots before color buffers in Ramses -
	// the RenderTargetDescription::addRenderBuffer does not allow for it.
	// But if we only add valid render buffers to the list of render buffers,
	// it will shift all later render buffers to the front, changing their index
	// (which is not what the shader will expect).
	// This is a Ramses bug - see https://github.com/bmwcarit/ramses/issues/52
	// If that occurs, refuse to create the render target to avoid any surprises for the user.

	auto userTypeBuffers = this->editorObject()->buffers_->template asVector<std::shared_ptr<RenderBufferClass>>();
	bool hasEmptySlots = collectBuffers(buffers, userTypeBuffers, rtDesc, errors);

	if (!buffers.empty() && !hasEmptySlots) {
		this->reset(ramses_base::ramsesRenderTarget(this->sceneAdaptor_->scene(), rtDesc, buffers, this->editorObject()->objectIDAsRamsesLogicID()));
	} else if (!errors->hasError({this->editorObject()})) {
		std::string errorMsg;
		if (buffers.empty()) {
			errorMsg = fmt::format("Cannot create render target '{}' - its first buffer is not set or not valid.", this->editorObject()->objectName());
		} else {
			errorMsg = fmt::format("Cannot create render target '{}' - all buffers in it must be consecutive and valid buffers.", this->editorObject()->objectName());
		}
		this->reset(nullptr);
		LOG_ERROR(log_system::RAMSES_ADAPTOR, errorMsg);
		errors->addError(core::ErrorCategory::GENERAL, core::ErrorLevel::ERROR, {this->editorObject()}, errorMsg);
	}

	this->tagDirty(false);
	return true;
}

template <typename RenderTargetClass, typename RenderBufferClass, typename RenderBufferAdaptorClass>
std::vector<ExportInformation> RenderTargetAdaptorT<RenderTargetClass, RenderBufferClass, RenderBufferAdaptorClass>::getExportInformation() const {
	if (this->getRamsesObjectPointer() == nullptr) {
		return {};
	}

	return {ExportInformation{this->ramsesObject().getType(), this->ramsesObject().getName()}};
}

template class RenderTargetAdaptorT<user_types::RenderTarget, user_types::RenderBuffer, RenderBufferAdaptor>;
template class RenderTargetAdaptorT<user_types::RenderTargetMS, user_types::RenderBufferMS, RenderBufferMSAdaptor>;

};	// namespace raco::ramses_adaptor