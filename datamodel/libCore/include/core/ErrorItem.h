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

#include "Handles.h"
#include <string>

namespace raco::core {

enum class ErrorCategory {
	GENERAL = 0,
	PARSING, // PARSING
	FILESYSTEM,
	RAMSES_LOGIC_RUNTIME,
	EXTERNAL_REFERENCE,
	MIGRATION
};

enum class ErrorLevel {
	NONE = 0,
	INFORMATION = 1,
	WARNING = 2,
	ERROR = 3
};

/**
 * General error item for all errors within a project.
 * 
 * There are 3 different classes of errors
 * - Project global errors: the ErrorItem contains an empty [core::ValueHandle]
 * - Object errors: the ValueHandle describes an object, i.e. ValueHandle::isObject() == true
 * - Property errors: the ValueHandle describes a property, i.e. ValueHandle::isProperty() == true
 */
class ErrorItem {
public:
	explicit ErrorItem();
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
