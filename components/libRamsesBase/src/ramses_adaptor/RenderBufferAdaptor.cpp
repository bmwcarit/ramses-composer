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
#include "core/BasicAnnotations.h"
#include "ramses_base/RamsesHandles.h"
#include "ramses_base/Utils.h"

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
	binding_.reset();
	buffer_.reset();

	auto format = static_cast<user_types::ERenderBufferFormat>(*editorObject()->format_);
	ramses::ERenderBufferFormat ramsesFormat = ramses_base::enumerationTranslationRenderBufferFormat.at(format);

	bool allValid = true;
	uint32_t clippedWidth = ramses_base::clipAndCheckIntProperty({editorObject_, &user_types::RenderBuffer::width_}, errors, &allValid);
	uint32_t clippedHeight = ramses_base::clipAndCheckIntProperty({editorObject_, &user_types::RenderBuffer::height_}, errors, &allValid);

	if (allValid) {
		buffer_ = ramses_base::ramsesRenderBuffer(sceneAdaptor_->scene(),
			clippedWidth, clippedHeight,
			ramsesFormat,
			ramses::ERenderBufferAccessMode::ReadWrite,
			0U,
			(editorObject()->objectName() + "_Buffer").c_str(), 
			editorObject_->objectIDAsRamsesLogicID());
	}

	if (buffer_) {
		// For depth buffers, the UI does not display the sampler parameters - so force them to default.
		// Using depth buffers directly as textures is not recommended.
		ramses_base::RamsesTextureSampler textureSampler;

		auto wrapUMode = static_cast<user_types::ETextureAddressMode>(*editorObject()->wrapUMode_);
		auto ramsesWrapUMode = ramses_base::enumerationTranslationTextureAddressMode.at(wrapUMode);

		auto wrapVMode = static_cast<user_types::ETextureAddressMode>(*editorObject()->wrapVMode_);
		auto ramsesWrapVMode = ramses_base::enumerationTranslationTextureAddressMode.at(wrapVMode);

		auto minSamplMethod = static_cast<user_types::ETextureSamplingMethod>(*editorObject()->minSamplingMethod_);
		auto ramsesMinSamplMethod = ramses_base::enumerationTranslationTextureSamplingMethod.at(minSamplMethod);

		auto magSamplMethod = static_cast<user_types::ETextureSamplingMethod>(*editorObject()->magSamplingMethod_);
		auto ramsesMagSamplMethod = ramses_base::enumerationTranslationTextureSamplingMethod.at(magSamplMethod);

		if (ramsesFormat == ramses::ERenderBufferFormat::Depth24 || ramsesFormat == ramses::ERenderBufferFormat::Depth24_Stencil8 ||
			ramsesFormat == ramses::ERenderBufferFormat::Depth16 || ramsesFormat == ramses::ERenderBufferFormat::Depth32) {
			textureSampler = ramses_base::ramsesTextureSampler(sceneAdaptor_->scene(),
				ramses::ETextureAddressMode::Clamp,
				ramses::ETextureAddressMode::Clamp,
				ramses::ETextureSamplingMethod::Nearest,
				ramses::ETextureSamplingMethod::Nearest,
				buffer_,
				1, 
				{},
				editorObject()->objectIDAsRamsesLogicID());
		} else {
			textureSampler = ramses_base::ramsesTextureSampler(sceneAdaptor_->scene(),
				ramsesWrapUMode,
				ramsesWrapVMode,
				ramsesMinSamplMethod,
				ramsesMagSamplMethod,
				buffer_,
				(*editorObject()->anisotropy_ >= 1 ? *editorObject()->anisotropy_ : 1),
				{},
				editorObject()->objectIDAsRamsesLogicID());
		}

		reset(std::move(textureSampler));

		binding_ = ramses_base::ramsesRenderBufferBinding(buffer_, &sceneAdaptor_->logicEngine(), editorObject()->objectName() + "_Binding", editorObject()->objectIDAsRamsesLogicID());
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

void RenderBufferAdaptor::getLogicNodes(std::vector<ramses::LogicNode*>& logicNodes) const {
	if (binding_) {
		logicNodes.push_back(binding_.get());
	}
}

ramses::Property* RenderBufferAdaptor::getProperty(const std::vector<std::string_view>& propertyNamesVector) {
	if (binding_ && propertyNamesVector.size() >= 1) {
		return binding_->getInputs()->getChild(propertyNamesVector[0]);
	}
	return nullptr;
}

void RenderBufferAdaptor::onRuntimeError(core::Errors& errors, std::string const& message, core::ErrorLevel level) {
	core::ValueHandle const valueHandle{editorObject_};
	if (errors.hasError(valueHandle)) {
		return;
	}
	errors.addError(core::ErrorCategory::RAMSES_LOGIC_RUNTIME, level, valueHandle, message);
}

}  // namespace raco::ramses_adaptor
