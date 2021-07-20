/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#pragma once

#include <functional>

#include "core/EditorObject.h"
#include "core/Context.h"

namespace raco::core {
class BaseContext;

class FileChangeCallback {
public:
	using Callback = std::function<void()>;

	FileChangeCallback(BaseContext* context, SEditorObject object, Callback callback) : context_(context), object_(object), callback_(callback) {}

	void operator()() const {
		callback_();
		if (object_ && context_) {
			context_->callReferencedObjectChangedHandlers(object_);
		}
	}

private:
	BaseContext* context_;
	SEditorObject object_{nullptr};
	Callback callback_;
};


}  // namespace raco::core