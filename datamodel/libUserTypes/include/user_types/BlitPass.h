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

#include "user_types/BaseObject.h"
#include "user_types/RenderBuffer.h"
#include "user_types/RenderBufferMS.h"

namespace raco::user_types {

class BlitPass : public BaseObject {
public:
	static inline const TypeDescriptor typeDescription = {"BlitPass", true};
	TypeDescriptor const& getTypeDescription() const override {
		return typeDescription;
	}

	BlitPass(BlitPass const& other)
		: BaseObject(other),
		  sourceRenderBuffer_(other.sourceRenderBuffer_),
		  targetRenderBuffer_(other.targetRenderBuffer_),
		  sourceX_(other.sourceX_),
		  sourceY_(other.sourceY_),
		  destinationX_(other.destinationX_),
		  destinationY_(other.destinationY_),
		  width_(other.width_),
		  height_(other.height_),
		  enabled_(other.enabled_),
		  renderOrder_(other.renderOrder_) {
		fillPropertyDescription();
	}

	BlitPass(const std::string& name, const std::string& id) : BaseObject(name, id) {
		fillPropertyDescription();
	}

	void fillPropertyDescription() {
		properties_.emplace_back("sourceRenderBuffer", &sourceRenderBuffer_);
		properties_.emplace_back("targetRenderBuffer", &targetRenderBuffer_);
		properties_.emplace_back("sourceRenderBufferMS", &sourceRenderBufferMS_);
		properties_.emplace_back("targetRenderBufferMS", &targetRenderBufferMS_);
		properties_.emplace_back("sourceX", &sourceX_);
		properties_.emplace_back("sourceY", &sourceY_);
		properties_.emplace_back("destinationX", &destinationX_);
		properties_.emplace_back("destinationY", &destinationY_);
		properties_.emplace_back("width", &width_);
		properties_.emplace_back("height", &height_);
		properties_.emplace_back("enabled", &enabled_);
		properties_.emplace_back("renderOrder", &renderOrder_);
	}

	void onAfterContextActivated(BaseContext& context) override;
	void onAfterValueChanged(BaseContext& context, ValueHandle const& value) override;
	void onAfterReferencedObjectChanged(BaseContext& context, ValueHandle const& changedObject) override;

	Property<SRenderBuffer, DisplayNameAnnotation, ExpectEmptyReference> sourceRenderBuffer_{{}, {"Source Render Buffer"}, {}};
	Property<SRenderBuffer, DisplayNameAnnotation, ExpectEmptyReference> targetRenderBuffer_{{}, {"Target Render Buffer"}, {}};
	Property<SRenderBufferMS, DisplayNameAnnotation, ExpectEmptyReference> sourceRenderBufferMS_{{}, {"Source Render Buffer (Multisampled)"}, {}};
	Property<SRenderBufferMS, DisplayNameAnnotation, ExpectEmptyReference> targetRenderBufferMS_{{}, {"Target Render Buffer (Multisampled)"}, {}};

	Property<int, RangeAnnotation<int>, DisplayNameAnnotation> sourceX_{0, {0, 7680}, {"Source Buffer Offset X"}};
	Property<int, RangeAnnotation<int>, DisplayNameAnnotation> sourceY_{0, {0, 7680}, {"Source Buffer Offset Y"}};
	Property<int, RangeAnnotation<int>, DisplayNameAnnotation> destinationX_{0, {0, 7680}, {"Destination Buffer Offset X"}};
	Property<int, RangeAnnotation<int>, DisplayNameAnnotation> destinationY_{0, {0, 7680}, {"Destination Buffer Offset Y"}};
	Property<int, RangeAnnotation<int>, DisplayNameAnnotation> width_{256, {0, 7680}, {"Blitting Region Width"}};
	Property<int, RangeAnnotation<int>, DisplayNameAnnotation> height_{256, {0, 7680}, {"Blitting Region Height"}};

	Property<bool, DisplayNameAnnotation> enabled_{true, {"Enabled"}};
	Property<int, DisplayNameAnnotation> renderOrder_{0, {"Render Order"}};

private:
	void validateBufferCompatibility(BaseContext& context);
};

using SBlitPass = std::shared_ptr<BlitPass>;
}  // namespace raco::user_types