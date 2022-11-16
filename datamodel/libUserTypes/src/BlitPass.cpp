/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "user_types/BlitPass.h"
#include "Validation.h"
#include "core/Context.h"
#include "core/Handles.h"
#include "core/PathQueries.h"
#include "core/Project.h"
#include "core/Queries.h"
#include "user_types/LuaScriptModule.h"
#include "utils/FileUtils.h"

namespace raco::user_types {

void BlitPass::onAfterContextActivated(BaseContext& context) {
	BaseObject::onAfterContextActivated(context);

	validateBufferCompatibility(context);
}

void BlitPass::onAfterValueChanged(BaseContext& context, ValueHandle const& value) {
	BaseObject::onAfterValueChanged(context, value);

	validateBufferCompatibility(context);

	context.changeMultiplexer().recordPreviewDirty(shared_from_this());
}

void BlitPass::onAfterReferencedObjectChanged(BaseContext& context, ValueHandle const& changedObject) {
	context.changeMultiplexer().recordPreviewDirty(shared_from_this());
}

void BlitPass::validateBufferCompatibility(BaseContext & context) {
	ValueHandle sourceBufferHandle{shared_from_this(), &user_types::BlitPass::sourceRenderBufferMS_};
	if (*sourceRenderBuffer_ && *sourceRenderBufferMS_) {
		context.errors().addError(raco::core::ErrorCategory::GENERAL, ErrorLevel::WARNING, sourceBufferHandle, "Single-sample and multi-sample source buffer selected: Single-sample source buffer will be preferred.");
	} else {
		context.errors().removeError(sourceBufferHandle);
	}

	ValueHandle targetBufferHandle{shared_from_this(), &user_types::BlitPass::targetRenderBufferMS_};
	if (*targetRenderBuffer_ && *targetRenderBufferMS_) {
		context.errors().addError(raco::core::ErrorCategory::GENERAL, ErrorLevel::WARNING, targetBufferHandle, "Single-sample and multi-sample target buffer selected: Single-sample target buffer will be preferred.");
	} else {
		context.errors().removeError(targetBufferHandle);
	}
}

}  // namespace raco::user_types
