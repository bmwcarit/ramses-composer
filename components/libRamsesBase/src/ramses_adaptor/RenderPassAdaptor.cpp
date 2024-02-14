/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "ramses_adaptor/RenderPassAdaptor.h"
#include "ramses_adaptor/OrthographicCameraAdaptor.h"
#include "ramses_adaptor/PerspectiveCameraAdaptor.h"
#include "ramses_adaptor/RenderTargetAdaptor.h"
#include "ramses_adaptor/RenderLayerAdaptor.h"
#include "ramses_base/RamsesHandles.h"

#include "user_types/OrthographicCamera.h"
#include "user_types/PerspectiveCamera.h"

namespace raco::ramses_adaptor {

RenderPassAdaptor::RenderPassAdaptor(SceneAdaptor* sceneAdaptor, std::shared_ptr<user_types::RenderPass> editorObject)
	: TypedObjectAdaptor(sceneAdaptor, editorObject, {}),
	  subscriptions_{
	sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{editorObject, &user_types::RenderPass::target_}, [this]() {
						 tagDirty();
					 }),
		  sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{editorObject, &user_types::RenderPass::camera_}, [this]() {
			  tagDirty();
		  }),
		  sceneAdaptor->dispatcher()->registerOnChildren(core::ValueHandle{editorObject, &user_types::RenderPass::layers_}, [this](auto) { tagDirty(); }),
		  sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{editorObject, &user_types::RenderPass::enabled_}, [this]() {
			  tagDirty();
		  }),
		  sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{editorObject, &user_types::RenderPass::renderOnce_}, [this]() {
			  tagDirty();
		  }),
		  sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{editorObject, &user_types::RenderPass::renderOrder_}, [this]() {
			  tagDirty();
		  }),
		  sceneAdaptor->dispatcher()->registerOnChildren(core::ValueHandle{editorObject, &user_types::RenderPass::clearColor_}, [this](auto) {
			  tagDirty();
		  }),
		  sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{editorObject, &user_types::RenderPass::enableClearColor_}, [this]() {
			  tagDirty();
		  }),
		  sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{editorObject, &user_types::RenderPass::enableClearDepth_}, [this]() {
			  tagDirty();
		  }),
		  sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{editorObject, &user_types::RenderPass::enableClearStencil_}, [this]() {
			  tagDirty();
		  })} {
}

bool RenderPassAdaptor::sync(core::Errors* errors) {
	errors->removeError({editorObject()->shared_from_this()});

	binding_.reset();

	ramses_base::RamsesRenderTarget ramsesTarget{};
	bool validTarget = true;
	if (auto target = *editorObject()->target_) {
		if (auto targetAdaptor = sceneAdaptor_->lookup<RenderTargetAdaptor>(target)) {
			ramsesTarget = targetAdaptor->getRamsesObjectPointer();
		}
		if (auto targetAdaptor = sceneAdaptor_->lookup<RenderTargetMSAdaptor>(target)) {
			ramsesTarget = targetAdaptor->getRamsesObjectPointer();
		}

		if (ramsesTarget == nullptr) {
			// ramsesTarget == nullptr can only happen if the RamsesTarget is invalid (e. g. because render buffers don't match in size).
			// Only reset render pass in this case and do not render anything.
			validTarget = false;
			const auto errorMsg = fmt::format("Render pass '{}' is not rendered due to invalid render target '{}'", editorObject()->objectName(), target->objectName());
			LOG_WARNING(log_system::RAMSES_ADAPTOR, errorMsg);
			errors->addError(core::ErrorCategory::PARSING, core::ErrorLevel::WARNING, {editorObject()->shared_from_this()}, errorMsg);
		}
	}

	auto camera = *editorObject()->camera_;
	if (validTarget && camera) {
		if (auto perspCamera = camera->as<user_types::OrthographicCamera>()) {
			if (auto cameraAdaptor = sceneAdaptor_->lookup<OrthographicCameraAdaptor>(perspCamera)) {
				auto newRamsesObject = ramses_base::ramsesRenderPass(sceneAdaptor_->scene(), cameraAdaptor->getRamsesObjectPointer(), ramsesTarget, editorObject()->objectName().c_str(), editorObject_->objectIDAsRamsesLogicID());
				reset(std::move(newRamsesObject));
			}
		} else if (auto orthoCamera = camera->as<user_types::PerspectiveCamera>()) {
			if (auto cameraAdaptor = sceneAdaptor_->lookup<PerspectiveCameraAdaptor>(orthoCamera)) {
				auto newRamsesObject = ramses_base::ramsesRenderPass(sceneAdaptor_->scene(), cameraAdaptor->getRamsesObjectPointer(), ramsesTarget, editorObject()->objectName().c_str(), editorObject_->objectIDAsRamsesLogicID());
				reset(std::move(newRamsesObject));
			}
		}
	} else {
		reset(nullptr);
	}

	if (getRamsesObjectPointer()) {
		ramsesObject().removeAllRenderGroups();

		for (auto layer : editorObject()->layers_->asVector<user_types::SRenderLayer>()) {
			if (auto layerAdaptor = sceneAdaptor_->lookup<RenderLayerAdaptor>(layer)) {
				ramsesObject().addRenderGroup(layerAdaptor->getRamsesObjectPointer());
			}
		}

		(*ramsesObject()).setEnabled(*editorObject()->enabled_);
		(*ramsesObject()).setRenderOnce(*editorObject()->renderOnce_);
		(*ramsesObject()).setRenderOrder(*editorObject()->renderOrder_);

		(*ramsesObject()).setClearColor({*editorObject()->clearColor_->x, *editorObject()->clearColor_->y, *editorObject()->clearColor_->z, *editorObject()->clearColor_->w});

		if (ramsesTarget != nullptr) {
			(*ramsesObject()).setClearFlags(
				(*editorObject()->enableClearColor_ ? ramses::EClearFlag::Color : ramses::EClearFlag::None) | 
				(*editorObject()->enableClearDepth_ ? ramses::EClearFlag::Depth : ramses::EClearFlag::None) | 
				(*editorObject()->enableClearStencil_ ? ramses::EClearFlag::Stencil : ramses::EClearFlag::None));
		} else {
			// Force no clear flags for a render pass rendering to the default framebuffer.
			// Otherwise the scene validation on export complains that the clear flags
			// will have no effect.
			(*ramsesObject()).setClearFlags(ramses::EClearFlag::None);
		}

		binding_ = ramses_base::ramsesRenderPassBinding(*ramsesObject(), &sceneAdaptor_->logicEngine(), editorObject()->objectName() + "_Binding", editorObject()->objectIDAsRamsesLogicID());
	}

	tagDirty(false);
	return true;
}

void RenderPassAdaptor::getLogicNodes(std::vector<ramses::LogicNode*>& logicNodes) const {
	if (binding_) {
		logicNodes.push_back(binding_.get());
	}
}

ramses::Property* RenderPassAdaptor::getProperty(const std::vector<std::string_view>& propertyNamesVector) {
	if (binding_ && propertyNamesVector.size() >= 1) {
		return binding_->getInputs()->getChild(propertyNamesVector[0]);
	}
	return nullptr;
}

void RenderPassAdaptor::onRuntimeError(core::Errors& errors, std::string const& message, core::ErrorLevel level) {
	core::ValueHandle const valueHandle{editorObject_};
	if (errors.hasError(valueHandle)) {
		return;
	}
	errors.addError(core::ErrorCategory::RAMSES_LOGIC_RUNTIME, level, valueHandle, message);
}

std::vector<ExportInformation> RenderPassAdaptor::getExportInformation() const {
	auto result = std::vector<ExportInformation>();
	if (getRamsesObjectPointer() != nullptr) {
		result.emplace_back(ramses::ERamsesObjectType::RenderPass, getRamsesObjectPointer()->getName());
	}

	if (binding_ != nullptr) {
		result.emplace_back("RenderPassBinding", binding_->getName());
	}

	return result;
}

};	// namespace raco::ramses_adaptor