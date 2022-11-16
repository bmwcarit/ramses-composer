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

#include "core/ChangeRecorder.h"
#include "core/EditorObject.h"
#include "core/ErrorItem.h"
#include "core/Handles.h"
#include "log_system/log.h"

#include <map>

namespace raco::core {

/**
 * Basic Error storage.
 * For now we only allow one error per [ValueHandle].
 */
class Errors {
public:
	explicit Errors(DataChangeRecorder* recorder) noexcept;

	/**
	 * Adds an associated error to the given [ValueHandle]. If the ValueHandle already has an error it is replaced.
	 * 
	 * There are 3 different classes of errors depending on the [ValueHandle]
	 * - Project global errors: the ValueHandle is empty
	 * - Object errors: the ValueHandle describes an object, i.e. ValueHandle::isObject() == true
	 * - Property errors: the ValueHandle describes a property, i.e. ValueHandle::isProperty() == true
	 * 
	 * @returns Returns false if an identical error already existed, true otherwise.
	 */
	bool addError(ErrorCategory category, ErrorLevel level, const ValueHandle& handle, const std::string& message);

	static std::string formatError(const ErrorItem& error);

	static void logError(const ErrorItem& error);
	void logAllErrors() const;

	/**
	 * Removes the associated error from the given [ValueHandle] if it exists.
	 * @returns if an error was removed.
	 */
	bool removeError(const ValueHandle& handle);
	/**
	 * @returns if an error exists for the given [ValueHandle].
	 */
	bool hasError(const ValueHandle& handle) const noexcept;
	/**
	 * @returns reference to the [ErrorItem] for the given [ValueHandle]. Call #hasError before to check if the [ErrorItem] exists.
	 */
	const ErrorItem& getError(const ValueHandle& handle) const noexcept;
	/**
	 * Removes all error items associated with the given [SEditorObject].
	 * @returns true if any error item has been removed.
	 */
	bool removeAll(const SCEditorObject& object);
	/**
	 * Remove all error items matching the given filter.
	 * @returns true if any error item has been removed.
	 */
	bool removeIf(const std::function<bool(const ErrorItem&)>& predicate);

	/**
	 * @returns read-only reference to all saved errors.
	 */
	const std::map<ValueHandle, ErrorItem>& getAllErrors() const;

	bool hasError(ErrorLevel minLevel) const;

private:
	std::map<ValueHandle, ErrorItem> errors_;
	DataChangeRecorder* recorder_;
};

}  // namespace raco::core
