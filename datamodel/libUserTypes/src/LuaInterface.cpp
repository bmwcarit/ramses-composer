/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "user_types/LuaInterface.h"
#include "Validation.h"
#include "core/Context.h"
#include "core/EngineInterface.h"
#include "core/Handles.h"
#include "core/PathQueries.h"
#include "core/Project.h"
#include "core/Queries.h"
#include "utils/FileUtils.h"
#include "LuaUtils.h"

namespace raco::user_types {

void LuaInterface::onAfterReferencedObjectChanged(BaseContext& context, ValueHandle const& changedObject) {
	// module changed
	// -> only script parsing/sync
	syncLuaScript(context, false);
}

void LuaInterface::onAfterValueChanged(BaseContext& context, ValueHandle const& value) {
	BaseObject::onAfterValueChanged(context, value);

	if (value.isRefToProp(&LuaInterface::objectName_)) {
		context.updateBrokenLinkErrorsAttachedTo(shared_from_this());
	}

	ValueHandle stdModulesHandle(shared_from_this(), &LuaInterface::stdModules_);
	if (stdModulesHandle.contains(value)) {
		syncLuaScript(context, true);
	}

	ValueHandle modulesHandle(shared_from_this(), &LuaInterface::luaModules_);
	if (modulesHandle.contains(value)) {
		// -> only script parsing
		syncLuaScript(context, true);
	}
}

void LuaInterface::updateFromExternalFile(BaseContext& context) {
	syncLuaScript(context, true);
}

void LuaInterface::syncLuaScript(BaseContext& context, bool syncModules) {
	// TODO(error) try to avoid unnecessary error item updates
	context.errors().removeAll({shared_from_this()});

	PropertyInterfaceList inputs{};

	if (validateURI(context, {shared_from_this(), &LuaInterface::uri_})) {
		std::string luaInterface = utils::file::read(PathQueries::resolveUriPropertyToAbsolutePath(*context.project(), {shared_from_this(), &LuaInterface::uri_}));

		std::string error{};
		bool success = true;

		if (syncModules) {
			success = syncLuaModules(context, ValueHandle{shared_from_this(), &LuaInterface::luaModules_}, luaInterface, cachedModuleRefs_, error);
		}

		if (success) {
			success = checkLuaModules(ValueHandle{shared_from_this(), &LuaInterface::luaModules_}, context.errors());
		}

		if (success) {
			success = context.engineInterface().parseLuaInterface(luaInterface, stdModules_->activeModules(), *luaModules_, inputs, error);
		}

		if (!success) {
			if (!error.empty()) {
				context.errors().addError(ErrorCategory::PARSING, ErrorLevel::ERROR, shared_from_this(), error);
			}
		}
	}

	syncTableWithEngineInterface(context, inputs, ValueHandle(shared_from_this(), &LuaInterface::inputs_), cachedLuaInputValues_, true, true);
	context.updateBrokenLinkErrorsAttachedTo(shared_from_this());
	context.changeMultiplexer().recordPreviewDirty(shared_from_this());
}

}  // namespace raco::user_types
