/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "user_types/LuaScriptModule.h"
#include "Validation.h"
#include "core/Context.h"
#include "core/Handles.h"
#include "core/PathQueries.h"
#include "core/Project.h"
#include "log_system/log.h"
#include "utils/FileUtils.h"

namespace raco::user_types {

void LuaScriptModule::onBeforeDeleteObject(Errors& errors) const {
	EditorObject::onBeforeDeleteObject(errors);
	uriListener_.reset();
}

void LuaScriptModule::onAfterContextActivated(BaseContext& context) {
	uriListener_ = registerFileChangedHandler(context, {shared_from_this(), &LuaScriptModule::uri_}, [this, &context]() { this->syncLuaInterface(context); });
	syncLuaInterface(context);
}

void LuaScriptModule::onAfterValueChanged(BaseContext& context, ValueHandle const& value) {
	if (value.isRefToProp(&LuaScriptModule::uri_)) {
		uriListener_ = registerFileChangedHandler(context, value, [this, &context]() { this->syncLuaInterface(context); });
		syncLuaInterface(context);
	}
}

void LuaScriptModule::syncLuaInterface(BaseContext& context) {
	std::string luaScript = utils::file::read(PathQueries::resolveUriPropertyToAbsolutePath(*context.project(), {shared_from_this(), &LuaScriptModule::uri_}));

	std::string error;
	if (!luaScript.empty()) {
		context.engineInterface().parseLuaScriptModule(luaScript, error);
	}

	context.errors().removeError({shared_from_this()});
	if (validateURI(context, {shared_from_this(), &LuaScriptModule::uri_})) {
		if (error.size() > 0) {
			context.errors().addError(ErrorCategory::PARSE_ERROR, ErrorLevel::ERROR, shared_from_this(), error);
		}
		currentScriptContents_ = luaScript;
	} else {
		currentScriptContents_.clear();
	}

	context.changeMultiplexer().recordPreviewDirty(shared_from_this());
}

}  // namespace raco::user_types
