/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#pragma once

#include "core/PrefabOperations.h"

#include "user_types/BaseCamera.h"
#include "user_types/BaseObject.h"
#include "user_types/OrthographicCamera.h"
#include "user_types/PerspectiveCamera.h"
#include "user_types/RenderLayer.h"
#include "user_types/RenderTarget.h"

namespace raco::user_types {

class RenderPass : public BaseObject {
public:
	static inline const TypeDescriptor typeDescription = {"RenderPass", true};
	TypeDescriptor const& getTypeDescription() const override {
		return typeDescription;
	}

	RenderPass(RenderPass const& other)
		: BaseObject(other),
		  target_(other.target_),
		  camera_(other.camera_),
		  layers_(other.layers_),
		  enabled_(other.enabled_),
		  renderOnce_(other.renderOnce_),
		  renderOrder_(other.renderOrder_),
		  clearColor_(other.clearColor_),
		  enableClearColor_(other.enableClearColor_),
		  enableClearDepth_(other.enableClearDepth_),
		  enableClearStencil_(other.enableClearStencil_) {
		fillPropertyDescription();
	}

	RenderPass(const std::string& name, const std::string& id) : BaseObject(name, id) {
		fillPropertyDescription();
		layers_->resize(1);
	}

	void fillPropertyDescription() {
		properties_.emplace_back("target", &target_);
		properties_.emplace_back("camera", &camera_);
		properties_.emplace_back("layers", &layers_);
		properties_.emplace_back("enabled", &enabled_);
		properties_.emplace_back("renderOnce", &renderOnce_);
		properties_.emplace_back("renderOrder", &renderOrder_);
		properties_.emplace_back("clearColor", &clearColor_);
		properties_.emplace_back("enableClearColor", &enableClearColor_);
		properties_.emplace_back("enableClearDepth", &enableClearDepth_);
		properties_.emplace_back("enableClearStencil", &enableClearStencil_);
	}

	bool isClearTargetProperty(ValueHandle const& handle) const;
	

	Property<SRenderTargetBase, DisplayNameAnnotation, ExpectEmptyReference> target_{{}, {"Target"}, {"Default Framebuffer"}};
	Property<SBaseCamera, DisplayNameAnnotation> camera_{{}, {"Camera"}};

	Property<Array<SRenderLayer>, DisplayNameAnnotation, ExpectEmptyReference, ResizableArray> layers_{{}, {"Layers"}, {}, {}};

	Property<bool, DisplayNameAnnotation, LinkEndAnnotation> enabled_{true, {"Enabled"}, {}};
	Property<bool, DisplayNameAnnotation, LinkEndAnnotation> renderOnce_{false, {"Render Once"}, {}};
	Property<int, DisplayNameAnnotation, LinkEndAnnotation> renderOrder_{1, {"Render Order"}, {}};

	Property<Vec4f, DisplayNameAnnotation, LinkEndAnnotation> clearColor_{{}, {"Clear Color"}, {}};

	Property<bool, DisplayNameAnnotation> enableClearColor_{true, {"Enable Clear Color"}};
	Property<bool, DisplayNameAnnotation> enableClearDepth_{true, {"Enable Clear Depth"}};
	Property<bool, DisplayNameAnnotation> enableClearStencil_{true, {"Enable Clear Stencil"}};
};

using SRenderPass = std::shared_ptr<RenderPass>;

}  // namespace raco::user_types
