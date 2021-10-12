/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
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
	  subscriptions_{sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{editorObject, &user_types::RenderPass::target_}, [this]() {
						 tagDirty();
					 }),
		  sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{editorObject, &user_types::RenderPass::camera_}, [this]() {
			  tagDirty();
		  }),
		  sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{editorObject, &user_types::RenderPass::layer0_}, [this]() { tagDirty(); }),
		  sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{editorObject, &user_types::RenderPass::layer1_}, [this]() { tagDirty(); }),
		  sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{editorObject, &user_types::RenderPass::layer2_}, [this]() { tagDirty(); }),
		  sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{editorObject, &user_types::RenderPass::layer3_}, [this]() { tagDirty(); }),
		  sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{editorObject, &user_types::RenderPass::layer4_}, [this]() { tagDirty(); }),
		  sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{editorObject, &user_types::RenderPass::layer5_}, [this]() { tagDirty(); }),
		  sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{editorObject, &user_types::RenderPass::layer6_}, [this]() { tagDirty(); }),
		  sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{editorObject, &user_types::RenderPass::layer7_}, [this]() { tagDirty(); }),
		  sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{editorObject, &user_types::RenderPass::enabled_}, [this]() {
			  tagDirty();
		  }),
		  sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{editorObject, &user_types::RenderPass::order_}, [this]() {
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
	ramses_base::RamsesRenderTarget ramsesTarget{};
	if (auto target = *editorObject()->target_) {
		if (auto targetAdaptor = sceneAdaptor_->lookup<RenderTargetAdaptor>(target)) {
			ramsesTarget = targetAdaptor->getRamsesObjectPointer();
		}
	}

	auto camera = *editorObject()->camera_;
	if (camera) {
		if (auto perspCamera = camera->as<user_types::OrthographicCamera>()) {
			if (auto cameraAdaptor = sceneAdaptor_->lookup<OrthographicCameraAdaptor>(perspCamera)) {
				auto newRamsesObject = ramses_base::ramsesRenderPass(sceneAdaptor_->scene(), cameraAdaptor->getRamsesObjectPointer(), ramsesTarget, editorObject()->objectName().c_str());
				reset(std::move(newRamsesObject));
			}
		} else if (auto orthoCamera = camera->as<user_types::PerspectiveCamera>()) {
			if (auto cameraAdaptor = sceneAdaptor_->lookup<PerspectiveCameraAdaptor>(orthoCamera)) {
				auto newRamsesObject = ramses_base::ramsesRenderPass(sceneAdaptor_->scene(), cameraAdaptor->getRamsesObjectPointer(), ramsesTarget, editorObject()->objectName().c_str());
				reset(std::move(newRamsesObject));
			}
		}
	} else {
		reset(nullptr);
	}

	if (getRamsesObjectPointer()) {
		ramsesObject().removeAllRenderGroups();

		auto layers = {
			*editorObject().get()->layer0_,
			*editorObject().get()->layer1_,
			*editorObject().get()->layer2_,
			*editorObject().get()->layer3_,
			*editorObject().get()->layer4_,
			*editorObject().get()->layer5_,
			*editorObject().get()->layer6_,
			*editorObject().get()->layer7_
		};

		for (auto layer : layers) {
			if (auto layerAdaptor = sceneAdaptor_->lookup<RenderLayerAdaptor>(layer)) {
				ramsesObject().addRenderGroup(layerAdaptor->getRamsesObjectPointer());
			}
		}

		(*ramsesObject()).setEnabled(*editorObject()->enabled_);
		(*ramsesObject()).setRenderOrder(*editorObject()->order_);

		(*ramsesObject()).setClearColor(
			*editorObject()->clearColor_->x,
			*editorObject()->clearColor_->y,
			*editorObject()->clearColor_->z,
			*editorObject()->clearColor_->w);

		(*ramsesObject()).setClearFlags(
			(*editorObject()->enableClearColor_ ? ramses::EClearFlags_Color : 0) |
			(*editorObject()->enableClearDepth_ ? ramses::EClearFlags_Depth : 0) |
			(*editorObject()->enableClearStencil_ ? ramses::EClearFlags_Stencil : 0));
	}

	tagDirty(false);
	return true;
}

};	// namespace raco::ramses_adaptor