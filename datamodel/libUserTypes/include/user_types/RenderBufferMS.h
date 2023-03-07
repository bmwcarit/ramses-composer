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

#include "core/Context.h"
#include "core/EngineInterface.h"
#include "core/Errors.h"
#include "user_types/DefaultValues.h"
#include "user_types/BaseTexture.h"

namespace raco::user_types {

class RenderBufferMS : public BaseObject {
public:
	static inline auto SAMPLE_COUNT_MIN = 1;
	static inline auto SAMPLE_COUNT_MAX = 8;

	static inline const TypeDescriptor typeDescription = {"RenderBufferMS", true};
	TypeDescriptor const& getTypeDescription() const override {
		return typeDescription;
	}

	RenderBufferMS(RenderBufferMS const& other)
		: BaseObject(other), width_(other.width_), height_(other.height_), sampleCount_(other.sampleCount_) {
		fillPropertyDescription();
	}

	RenderBufferMS(const std::string& name, const std::string& id) : BaseObject(name, id) {
		fillPropertyDescription();
	}

	void fillPropertyDescription() {
		properties_.emplace_back("width", &width_);
		properties_.emplace_back("height", &height_);
		properties_.emplace_back("format", &format_);
		properties_.emplace_back("sampleCount", &sampleCount_);
	}

	void onAfterContextActivated(BaseContext& context) override;
	void onAfterValueChanged(BaseContext& context, ValueHandle const& value) override;

	Property<int, RangeAnnotation<int>, DisplayNameAnnotation> width_{256, {1, 7680}, {"Width"}};
	Property<int, RangeAnnotation<int>, DisplayNameAnnotation> height_{256, {1, 7680}, {"Height"}};
	Property<int, DisplayNameAnnotation, EnumerationAnnotation> format_{DEFAULT_VALUE_RENDER_BUFFER_FORMAT, DisplayNameAnnotation("Format"), EnumerationAnnotation{EUserTypeEnumerations::RenderBufferFormat}};

	Property<int, RangeAnnotation<int>, DisplayNameAnnotation> sampleCount_{SAMPLE_COUNT_MIN, {SAMPLE_COUNT_MIN, SAMPLE_COUNT_MAX}, {"Sample Count"}};

private:
	void validateSampleCount(BaseContext& context);

};

using SRenderBufferMS = std::shared_ptr<RenderBufferMS>;

}  // namespace raco::user_types
