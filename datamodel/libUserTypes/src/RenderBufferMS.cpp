/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "user_types/RenderBufferMS.h"

namespace raco::user_types {

void RenderBufferMS::onAfterContextActivated(BaseContext& context) {
	BaseObject::onAfterContextActivated(context);

	validateSampleCount(context);
}

void RenderBufferMS::onAfterValueChanged(BaseContext& context, ValueHandle const& value) {
	BaseObject::onAfterValueChanged(context, value);

	if (value.isRefToProp(&RenderBufferMS::sampleCount_)) {
		validateSampleCount(context);
	}
}

void RenderBufferMS::validateSampleCount(BaseContext& context) {
	ValueHandle handle{shared_from_this(), &RenderBufferMS::sampleCount_};
	if (*sampleCount_ < SAMPLE_COUNT_MIN || *sampleCount_ > SAMPLE_COUNT_MAX) {
		context.errors().addError(ErrorCategory::GENERAL, ErrorLevel::ERROR, handle, "Invalid sample count.\nOnly values from 1 to 8 are allowed.");
	} else {
		context.errors().removeError(handle);
	}
}

}  // namespace raco::user_types