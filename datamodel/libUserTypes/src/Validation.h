/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#pragma once

#include "core/Context.h"
#include "core/ErrorItem.h"
#include "core/Handles.h"
#include "utils/FileUtils.h"
#include "utils/u8path.h"
#include "core/PathManager.h"
#include "core/PathQueries.h"
#include "core/Project.h"
#include "core/Errors.h"

namespace raco::user_types {

inline bool validateURI(core::BaseContext& context, const core::ValueHandle& handle) {
	using core::ErrorCategory;
	using core::ErrorLevel;

	auto uriPath = core::PathQueries::resolveUriPropertyToAbsolutePath(*context.project(), handle);

	if (handle.asString().empty()) {
		context.errors().addError(ErrorCategory::FILESYSTEM, ErrorLevel::WARNING, handle, "Empty URI.");
		return false;
	} else if (!utils::u8path(uriPath).exists(context.isUriValidationCaseSensitive())) {
		context.errors().addError(ErrorCategory::FILESYSTEM, ErrorLevel::ERROR, handle, "File not found.");
		return false;
	} else {
		context.errors().removeError(handle);
		return true;
	}
}

template <typename... Args>
inline bool validateURIs(core::BaseContext& context, Args... args) {
	return (validateURI(context, args) & ...) > 0;
}

}  // namespace raco
