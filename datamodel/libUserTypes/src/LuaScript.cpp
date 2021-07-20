/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "user_types/LuaScript.h"
#include "Validation.h"
#include "core/Context.h"
#include "core/Handles.h"
#include "core/PathQueries.h"
#include "core/Project.h"
#include "log_system/log.h"
#include "utils/FileUtils.h"

namespace raco::user_types {

void LuaScript::onBeforeDeleteObject(Errors& errors) const {
	EditorObject::onBeforeDeleteObject(errors);
	uriListener_.reset();
}

void LuaScript::onAfterContextActivated(BaseContext& context) {
	uriListener_ = registerFileChangedHandler(context, {shared_from_this(), {"uri"}}, [this, &context]() { this->syncLuaInterface(context); });
	syncLuaInterface(context);
}

void LuaScript::onAfterValueChanged(BaseContext& context, ValueHandle const& value) {
	ValueHandle uriHandle{shared_from_this(), {"uri"}};
	if (uriHandle == value) {
		uriListener_ = registerFileChangedHandler(context, uriHandle, [this, &context]() { this->syncLuaInterface(context); });
		syncLuaInterface(context);
	}
}

void LuaScript::syncLuaInterface(BaseContext& context) {
	std::string luaScript = utils::file::read(PathQueries::resolveUriPropertyToAbsolutePath(*context.project(), {shared_from_this(), {"uri"}}));
	PropertyInterfaceList inputs{};
	PropertyInterfaceList outputs{};
	std::string error{};
	if (!luaScript.empty())
		context.engineInterface().parseLuaScript(luaScript, inputs, outputs, error);

	context.errors().removeError({shared_from_this()});
	if (validateURI(context, {shared_from_this(), {"uri"}})) {
		if (error.size() > 0) {
			context.errors().addError(ErrorCategory::PARSE_ERROR, ErrorLevel::ERROR, shared_from_this(), error);
		}
	}

	syncTableWithEngineInterface(context, inputs, ValueHandle(shared_from_this(), {"luaInputs"}), cachedLuaInputValues_, false, true);
	OutdatedPropertiesStore dummyCache{};
	syncTableWithEngineInterface(context, outputs, ValueHandle(shared_from_this(), {"luaOutputs"}), dummyCache, true, false);
	context.changeMultiplexer().recordPreviewDirty(shared_from_this());
}

}  // namespace raco::user_types
