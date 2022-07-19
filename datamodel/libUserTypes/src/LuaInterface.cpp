/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
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

namespace raco::user_types {

void LuaInterface::onAfterValueChanged(BaseContext& context, ValueHandle const& value) {
	BaseObject::onAfterValueChanged(context, value);

	if (value.isRefToProp(&LuaInterface::objectName_)) {
		context.updateBrokenLinkErrorsAttachedTo(shared_from_this());
	}
}

void LuaInterface::updateFromExternalFile(BaseContext& context) {
	context.errors().removeAll({shared_from_this()});

	PropertyInterfaceList inputs{};

	if (validateURI(context, {shared_from_this(), &LuaInterface::uri_})) {
		std::string luaInterface = utils::file::read(PathQueries::resolveUriPropertyToAbsolutePath(*context.project(), {shared_from_this(), &LuaInterface::uri_}));

		std::string error{};
		bool success = context.engineInterface().parseLuaInterface(luaInterface, inputs, error);

		if (!success) {
			if (!error.empty()) {
				context.errors().addError(ErrorCategory::PARSE_ERROR, ErrorLevel::ERROR, shared_from_this(), error);
			}
		}
	}

	syncTableWithEngineInterface(context, inputs, ValueHandle(shared_from_this(), &LuaInterface::inputs_), cachedLuaInputValues_, true, true);
	context.updateBrokenLinkErrorsAttachedTo(shared_from_this());
	context.changeMultiplexer().recordPreviewDirty(shared_from_this());
}

}  // namespace raco::user_types
