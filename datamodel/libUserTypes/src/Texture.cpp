/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "user_types/Texture.h"
#include "Validation.h"
#include "core/Context.h"
#include "core/Handles.h"
#include "log_system/log.h"
#include "user_types/SyncTableWithEngineInterface.h"
#include "utils/FileUtils.h"

namespace raco::user_types {

void Texture::onBeforeDeleteObject(Errors& errors) const {
	EditorObject::onBeforeDeleteObject(errors);
	uriListener_.reset();
}

void Texture::onAfterContextActivated(BaseContext& context) {
	context.errors().removeError({shared_from_this()});
	validateURI(context, {shared_from_this(), {"uri"}});

	uriListener_ = registerFileChangedHandler(context, {shared_from_this(), {"uri"}},
		[this, &context]() {
			context.changeMultiplexer().recordPreviewDirty(shared_from_this());
		});
	context.changeMultiplexer().recordPreviewDirty(shared_from_this());
}

void Texture::onAfterValueChanged(BaseContext& context, ValueHandle const& value) {
	ValueHandle uriHandle{shared_from_this(), {"uri"}};
	if (uriHandle == value) {
		context.errors().removeError({shared_from_this()});
		validateURI(context, uriHandle);

		uriListener_ = registerFileChangedHandler(context, {shared_from_this(), {"uri"}},
			[this, &context, uriHandle]() {
				validateURI(context, uriHandle);
				context.changeMultiplexer().recordPreviewDirty(shared_from_this());
			});
	}
	context.changeMultiplexer().recordPreviewDirty(shared_from_this());
}

}  // namespace raco::user_types
