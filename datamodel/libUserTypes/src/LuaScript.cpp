/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "user_types/LuaScript.h"
#include "LuaUtils.h"
#include "Validation.h"
#include "core/Context.h"
#include "core/Handles.h"
#include "core/PathQueries.h"
#include "core/Project.h"
#include "core/Queries.h"
#include "user_types/LuaScriptModule.h"
#include "utils/FileUtils.h"

namespace raco::user_types {

void LuaScript::onAfterReferencedObjectChanged(BaseContext& context, ValueHandle const& changedObject) {
	// module changed
	// -> only script parsing/sync
	syncLuaScript(context, false);
}

void LuaScript::onAfterValueChanged(BaseContext& context, ValueHandle const& value) {
	// uri changed -> updateFromExternalFile
	// -> sync lua modules and parse/sync script
	BaseObject::onAfterValueChanged(context, value);

	if (value.isRefToProp(&LuaScript::objectName_)) {
		context.updateBrokenLinkErrorsAttachedTo(shared_from_this());
	}

	ValueHandle stdModulesHandle(shared_from_this(), &LuaScript::stdModules_);
	if (stdModulesHandle.contains(value)) {
		syncLuaScript(context, true);
	}

	ValueHandle modulesHandle(shared_from_this(), &LuaScript::luaModules_);
	if (modulesHandle.contains(value)) {
		// -> only script parsing
		syncLuaScript(context, true);
	}
}

void LuaScript::updateFromExternalFile(BaseContext& context) {
	syncLuaScript(context, true);
}

void LuaScript::syncLuaScript(BaseContext& context, bool syncModules) {
	// TODO(error) try to avoid unnecessary error item updates
	context.errors().removeAll({shared_from_this()});

	PropertyInterfaceList inputs{};
	PropertyInterfaceList outputs{};

	if (validateURI(context, {shared_from_this(), &LuaScript::uri_})) {
		std::string luaScript = utils::file::read(PathQueries::resolveUriPropertyToAbsolutePath(*context.project(), {shared_from_this(), &LuaScript::uri_}));

		std::string error{};
		bool success = true;

		if (syncModules) {
			success = syncLuaModules(context, ValueHandle{shared_from_this(), &LuaScript::luaModules_}, luaScript, cachedModuleRefs_, error);
		}

		if (success) {
			success = checkLuaModules(ValueHandle{shared_from_this(), &LuaScript::luaModules_}, context.errors());
		}

		if (success) {
			success = context.engineInterface().parseLuaScript(luaScript, objectName(), stdModules_->activeModules(), *luaModules_, inputs, outputs, error);
		}

		if (!success && !error.empty()) {
			context.errors().addError(ErrorCategory::PARSING, ErrorLevel::ERROR, shared_from_this(), error);
		}
	}

	syncTableWithEngineInterface(context, inputs, ValueHandle(shared_from_this(), &LuaScript::inputs_), cachedLuaInputValues_, false, true);
	OutdatedPropertiesStore dummyCache{};
	syncTableWithEngineInterface(context, outputs, ValueHandle(shared_from_this(), &LuaScript::outputs_), dummyCache, true, false);

	context.updateBrokenLinkErrorsAttachedTo(shared_from_this());

	context.changeMultiplexer().recordPreviewDirty(shared_from_this());
}

}  // namespace raco::user_types
