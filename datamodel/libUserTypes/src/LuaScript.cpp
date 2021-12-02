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
	uriListener_ = registerFileChangedHandler(context, {shared_from_this(), &LuaScript::uri_}, [this, &context]() { this->syncLuaInterface(context); });
	currentScriptValid_ = false;
	currentScriptContents_.clear();
	syncLuaInterface(context);
}

void LuaScript::onAfterValueChanged(BaseContext& context, ValueHandle const& value) {
	if (value.isRefToProp(&LuaScript::uri_)) {
		uriListener_ = registerFileChangedHandler(context, value, [this, &context]() { this->syncLuaInterface(context); });
		syncLuaInterface(context);
	}

	if (ValueHandle(shared_from_this(), &LuaScript::objectName_) == value) {
		context.updateBrokenLinkErrorsAttachedTo(shared_from_this());
	}
}

void LuaScript::syncLuaInterface(BaseContext& context) {
	std::string luaScript = utils::file::read(PathQueries::resolveUriPropertyToAbsolutePath(*context.project(), {shared_from_this(), &LuaScript::uri_}));
	if (currentScriptValid_ && luaScript == currentScriptContents_) {
		return;
	}
	currentScriptContents_ = luaScript;
	currentScriptValid_ = true;

	PropertyInterfaceList inputs{};
	PropertyInterfaceList outputs{};
	std::string error{};
	if (!luaScript.empty()) {
		context.engineInterface().parseLuaScript(luaScript, inputs, outputs, error);
	}

	context.errors().removeError({shared_from_this()});
	if (validateURI(context, {shared_from_this(), &LuaScript::uri_})) {
		if (error.size() > 0) {
			context.errors().addError(ErrorCategory::PARSE_ERROR, ErrorLevel::ERROR, shared_from_this(), error);
		}
	}

	if (std::find_if(inputs.begin(), inputs.end(), [](const PropertyInterface& intf) { return intf.type == EnginePrimitive::Int32 && intf.name == "time_ms"; }) != inputs.end()) {
		auto infoText = "Dear Animator,\n\n"
			"this LuaScript uses the 'time_ms'-based runtime hack which will be deprecated in a future version of Ramses Composer.\n"
			"Please prepare to transfer your timer-based animations to our new user types Animation and AnimationChannel.";
		context.errors().addError(raco::core::ErrorCategory::GENERAL, raco::core::ErrorLevel::WARNING, shared_from_this(), infoText);
	}

	syncTableWithEngineInterface(context, inputs, ValueHandle(shared_from_this(), &LuaScript::luaInputs_), cachedLuaInputValues_, false, true);
	OutdatedPropertiesStore dummyCache{};
	syncTableWithEngineInterface(context, outputs, ValueHandle(shared_from_this(), &LuaScript::luaOutputs_), dummyCache, true, false);

	context.updateBrokenLinkErrorsAttachedTo(shared_from_this());

	context.changeMultiplexer().recordPreviewDirty(shared_from_this());
}

}  // namespace raco::user_types
