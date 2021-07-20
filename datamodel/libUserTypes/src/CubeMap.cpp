/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "user_types/CubeMap.h"

#include "user_types/SyncTableWithEngineInterface.h"
#include "Validation.h"
#include "core/Context.h"
#include "log_system/log.h"
#include "utils/FileUtils.h"
#include <algorithm>


namespace raco::user_types {

void CubeMap::onBeforeDeleteObject(Errors& errors) const {
	EditorObject::onBeforeDeleteObject(errors);
	frontListener_.reset();
	backListener_.reset();
	leftListener_.reset();
	rightListener_.reset();
	topListener_.reset();
	bottomListener_.reset();
}

void CubeMap::onAfterValueChangedURI(BaseContext& context, ValueHandle const& value, std::string name, const std::string& uri, FileChangeMonitor::UniqueListener& listener) { 
	ValueHandle handle(shared_from_this(), {name});
	if (handle == value) {
		validateURI(context, handle);

		listener = registerFileChangedHandler(context, handle,
			[this, &context, handle]() {
				 validateURI(context, handle);
				 context.changeMultiplexer().recordPreviewDirty(shared_from_this());
			 });
		context.changeMultiplexer().recordPreviewDirty(shared_from_this());
	}
}

void CubeMap::onAfterValueChanged(BaseContext& context, ValueHandle const& value) {
	context.errors().removeError({shared_from_this()});
	onAfterValueChangedURI(context, value, "uriFront", *uriFront_, frontListener_);
	onAfterValueChangedURI(context, value, "uriBack", *uriBack_, backListener_);
	onAfterValueChangedURI(context, value, "uriLeft", *uriLeft_, leftListener_);
	onAfterValueChangedURI(context, value, "uriRight", *uriRight_, rightListener_);
	onAfterValueChangedURI(context, value, "uriTop", *uriTop_, topListener_);
	onAfterValueChangedURI(context, value, "uriBottom", *uriBottom_, bottomListener_);
}

void CubeMap::afterContextActivatedURI(BaseContext& context, std::string name, const std::string& uri, FileChangeMonitor::UniqueListener& listener) {
	validateURI(context, {shared_from_this(), {name}});
	listener = registerFileChangedHandler(context, {shared_from_this(), {name}},
		 [this, &context]() {
			 context.changeMultiplexer().recordPreviewDirty(shared_from_this());
		 });
}

void CubeMap::onAfterContextActivated(BaseContext& context) {
	afterContextActivatedURI(context, "uriFront", *uriFront_, frontListener_);
	afterContextActivatedURI(context, "uriBack", *uriBack_, backListener_);
	afterContextActivatedURI(context, "uriLeft", *uriLeft_, leftListener_);
	afterContextActivatedURI(context, "uriRight", *uriRight_, rightListener_);
	afterContextActivatedURI(context, "uriTop", *uriTop_, topListener_);
	afterContextActivatedURI(context, "uriBottom", *uriBottom_, bottomListener_);

	context.changeMultiplexer().recordPreviewDirty(shared_from_this());
}
}  // namespace raco::user_types
