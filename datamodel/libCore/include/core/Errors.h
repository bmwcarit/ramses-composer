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

#include "core/EditorObject.h"
#include "core/ErrorItem.h"
#include "core/ChangeRecorder.h"
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
     * Removes the associated error form the given [ValueHandle] if it exists.
     * @returns if an error was added.
     */
	bool addError(ErrorCategory category, ErrorLevel level, const ValueHandle& handle, const std::string& message);
	/**
     * Removes the associated error form the given [ValueHandle] if it exists.
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

private:
	std::map<ValueHandle, ErrorItem> errors_;
     DataChangeRecorder* recorder_;
};

}  // namespace raco::core
