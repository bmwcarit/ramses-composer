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
	binding_.reset();
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

		binding_ = ramses_base::ramsesRenderBufferBinding(buffer_, &sceneAdaptor_->logicEngine(), editorObject()->objectName() + "_Binding", editorObject()->objectIDAsRamsesLogicID());
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
	std::vector<ExportInformation> result;
	if (getRamsesObjectPointer() != nullptr) {
		result.emplace_back(ramsesObject().getType(), ramsesObject().getName());
	}
	if (buffer_) {
		result.emplace_back(buffer_->getType(), buffer_->getName());
	}
	if (binding_) {
		result.emplace_back("RenderBufferBinding", binding_->getName());
	}
	return result;
}

void RenderBufferMSAdaptor::getLogicNodes(std::vector<ramses::LogicNode*>& logicNodes) const {
	if (binding_) {
		logicNodes.push_back(binding_.get());
	}
}

ramses::Property* RenderBufferMSAdaptor::getProperty(const std::vector<std::string_view>& propertyNamesVector) {
	if (binding_ && propertyNamesVector.size() >= 1) {
		return binding_->getInputs()->getChild(propertyNamesVector[0]);
	}
	return nullptr;
}

void RenderBufferMSAdaptor::onRuntimeError(core::Errors& errors, std::string const& message, core::ErrorLevel level) {
	core::ValueHandle const valueHandle{editorObject_};
	if (errors.hasError(valueHandle)) {
		return;
	}
	errors.addError(core::ErrorCategory::RAMSES_LOGIC_RUNTIME, level, valueHandle, message);
}

}  // namespace raco::ramses_adaptor
