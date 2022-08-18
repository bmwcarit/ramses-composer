/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "core/FileChangeCallback.h"

#include "core/Context.h"
#include "core/EditorObject.h"

namespace raco::core {
void FileChangeCallback::operator()() const {
	callback_();
	if (object_ && context_) {
		context_->callReferencedObjectChangedHandlers(object_);
	}
}

}  // namespace raco::core