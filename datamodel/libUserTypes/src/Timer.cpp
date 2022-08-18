/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "user_types/Timer.h"

#include "core/Context.h"
#include "user_types/SyncTableWithEngineInterface.h"

namespace raco::user_types {

void Timer::onAfterValueChanged(BaseContext& context, ValueHandle const& value) {
	if (value == ValueHandle(shared_from_this(), &Timer::objectName_)) {
		context.updateBrokenLinkErrorsAttachedTo(shared_from_this());
	}
}

}  // namespace raco::user_types
