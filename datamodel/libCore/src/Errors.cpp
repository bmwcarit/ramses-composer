/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "core/Errors.h"

namespace raco::core {

Errors::Errors(DataChangeRecorder* recorder) noexcept :	recorder_{ recorder } {}

bool Errors::addError(ErrorCategory category, ErrorLevel level, const ValueHandle& handle, const std::string& message) {
	auto const it = errors_.find(handle);
	if (it != errors_.end()) {
		if (it->second == ErrorItem{category, level, handle, message}) {
			return false;
		}
		errors_.erase(it);
	}
	errors_.emplace(handle, ErrorItem(category, level, handle, message));

	recorder_->recordErrorChanged(handle);

	return true;
}

std::string Errors::formatError(const ErrorItem& error) {
	auto handle = error.valueHandle();
	const auto& message = error.message();
	if (!handle) {
		return fmt::format("Project-global: {}", message);
	} else if (handle.isObject()) {
		return fmt::format("{}[{}]: {}", handle.rootObject()->objectName(), handle.rootObject()->objectID(), message);
	} else {
		return fmt::format("{}[{}]#{}: {}", handle.rootObject()->objectName(), handle.rootObject()->objectID(), handle.getPropName(), message);
	}
}

void Errors::logError(const ErrorItem& error) {
	auto handle = error.valueHandle();
	const auto& message = error.message();
	switch (error.level()) {
		case ErrorLevel::ERROR:
			LOG_ERROR(log_system::CONTEXT, formatError(error));
			break;
		case ErrorLevel::WARNING:
			LOG_WARNING(log_system::CONTEXT, formatError(error));
			break;
		default:
			break;
	}
}

void Errors::logAllErrors() const {
	for (const auto& [handle, error]: errors_) {
		logError(error);
	}
}

bool Errors::removeError(const ValueHandle& handle) {
	auto const it = errors_.find(handle);
	if (it != errors_.end()) {
		errors_.erase(it);

		recorder_->recordErrorChanged(handle);
		return true;
	} else {
		return false;
	}
}

bool Errors::hasError(const ValueHandle& handle) const noexcept {
	return errors_.find(handle) != errors_.end();
}

bool Errors::hasError(ErrorLevel minLevel) const {
	for (const auto& [handle, error] : errors_) {
		if (error.level() >= minLevel) {
			return true;
		}
	}
	return false;
}

const ErrorItem& Errors::getError(const ValueHandle& handle) const noexcept {
	return errors_.find(handle)->second;
}

bool Errors::removeAll(const SCEditorObject& object) {
	auto filter{[object](const ErrorItem& err) { return err.valueHandle().rootObject() == object; }};
	return removeIf(filter);
}

bool Errors::removeIf(const std::function<bool(ErrorItem const&)>& predicate) {
	bool hasChanged{ false };
	auto wrapper = [&predicate](const std::pair<ValueHandle, ErrorItem>& p) { return predicate(p.second); };
	auto it = std::find_if(errors_.begin(), errors_.end(), wrapper);
	while (it != errors_.end()) {
		recorder_->recordErrorChanged(it->first);
		it = errors_.erase(it);
		hasChanged = true;
		it = std::find_if(it, errors_.end(), wrapper);
	}
	return hasChanged;
}

const std::map<ValueHandle, ErrorItem>& Errors::getAllErrors() const {
	return errors_;
}

}  // namespace raco::core
