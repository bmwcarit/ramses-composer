/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "user_types/LuaScriptModule.h"
#include "Validation.h"
#include "core/Context.h"
#include "core/EngineInterface.h"
#include "core/Handles.h"
#include "core/PathQueries.h"
#include "core/Project.h"
#include "utils/FileUtils.h"

namespace raco::user_types {

void LuaScriptModule::onBeforeDeleteObject(BaseContext& context) const {
	BaseObject::onBeforeDeleteObject(context);
	context.engineInterface().removeModuleFromCache(shared_from_this());
}

void LuaScriptModule::onAfterValueChanged(BaseContext& context, ValueHandle const& value) {
	// uri changed -> updateFromExternalFile
	// -> sync lua modules and parse/sync script
	BaseObject::onAfterValueChanged(context, value);

	ValueHandle stdModulesHandle(shared_from_this(), &LuaScriptModule::stdModules_);
	if (stdModulesHandle.contains(value)) {
		sync(context);
	}
}

void LuaScriptModule::updateFromExternalFile(BaseContext& context) {
	sync(context);
}

void LuaScriptModule::sync(BaseContext& context) {
	context.errors().removeError({shared_from_this()});

	if (validateURI(context, {shared_from_this(), &LuaScriptModule::uri_})) {
		std::string luaScript = utils::file::read(PathQueries::resolveUriPropertyToAbsolutePath(*context.project(), {shared_from_this(), &LuaScriptModule::uri_}));

		std::string error;
		isValid_ = context.engineInterface().parseLuaScriptModule(shared_from_this(), luaScript, objectName(), stdModules_->activeModules(), error);

		if (!isValid_) {
			context.errors().addError(ErrorCategory::PARSING, ErrorLevel::ERROR, shared_from_this(), error);
		}
		currentScriptContents_ = luaScript;
	} else {
		currentScriptContents_.clear();
		context.engineInterface().removeModuleFromCache(shared_from_this());
		isValid_ = false;
	}

	context.changeMultiplexer().recordPreviewDirty(shared_from_this());
}

bool LuaScriptModule::isValid() const {
	return isValid_;
}

const std::string& LuaScriptModule::currentScriptContents() const {
	return currentScriptContents_;
}

}  // namespace raco::user_types
