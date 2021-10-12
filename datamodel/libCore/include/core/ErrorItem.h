/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "Handles.h"
#include <string>

namespace raco::core {

enum class ErrorCategory {
	GENERAL = 0,
	PARSE_ERROR,
	FILESYSTEM_ERROR,
	RAMSES_LOGIC_RUNTIME_ERROR,
	EXTERNAL_REFERENCE_ERROR,
	MIGRATION_ERROR
};

enum class ErrorLevel {
	NONE = 0,
	INFORMATION = 1,
	WARNING = 2,
	ERROR = 3
};

/**
 * General error item for all [core::EditorObject]'s and [core::ValueHandle]'s within a project / context.
 * An ErrorItem can also contain an empty [core::ValueHandle] which means it is project-global instead of chained to an object or property.
 */
class ErrorItem {
public:
	explicit ErrorItem(ErrorCategory category, ErrorLevel level, const ValueHandle& handle, const std::string& message);

	ValueHandle valueHandle() const noexcept;
	const std::string& message() const noexcept;
	ErrorCategory category() const noexcept;
	ErrorLevel level() const noexcept;
	bool operator==(const ErrorItem& other) const;

private:
	ErrorCategory category_;
	ErrorLevel level_;
	std::string message_;
	ValueHandle handle_;
};

}  // namespace raco::core
