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

void Errors::addError(ErrorCategory category, ErrorLevel level, const ValueHandle& handle, const std::string& message) {
	ErrorItem newError{category, level, handle, message};
	if (!hasError(handle) || !(newError == getError(handle))) {
		errors_[handle.rootObject()][handle] = newError;
		recorder_->recordErrorChanged(handle);
	}
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
	for (const auto& [objectID, objErrors] : errors_) {
		for (const auto& [handle, error] : objErrors) {
			logError(error);
		}
	}
}

bool Errors::removeError(const ValueHandle& handle) {
	auto const objIt = errors_.find(handle.rootObject());
	if (objIt != errors_.end()) {
		auto& cont = objIt->second;
		auto const it = cont.find(handle);
		if (it != cont.end()) {
			cont.erase(it);
			if (cont.empty()) {
				errors_.erase(objIt);
			}
			recorder_->recordErrorChanged(handle);
			return true;
		}
	}
	return false;
}

bool Errors::hasError(const ValueHandle& handle) const noexcept {
	auto const objIt = errors_.find(handle.rootObject());
	if (objIt != errors_.end()) {
		auto& cont = objIt->second;
		return cont.find(handle) != cont.end();
	}
	return false;
}

bool Errors::hasError(ErrorLevel minLevel) const {
	for (const auto& [objectID, objErrors] : errors_) {
		for (const auto& [handle, error] : objErrors) {
			if (error.level() >= minLevel) {
				return true;
			}
		}
	}
	return false;
}

ErrorLevel Errors::maxErrorLevel() const {
	ErrorLevel maxLevel = ErrorLevel::NONE;
	for (const auto& [objectID, objErrors] : errors_) {
		for (const auto& [handle, error] : objErrors) {
			maxLevel = std::max(maxLevel, error.level());
		}
	}
	return maxLevel;
}

const ErrorItem& Errors::getError(const ValueHandle& handle) const noexcept {
	return errors_.find(handle.rootObject())->second.find(handle)->second;
}

bool Errors::removeAll(const SCEditorObject& object) {
	auto const objIt = errors_.find(object);
	if (objIt != errors_.end()) {
		auto& cont = objIt->second;
		for (const auto& [handle, item] : cont) {
			recorder_->recordErrorChanged(handle);
		}
		errors_.erase(objIt);
		return true;
	}
	return false;
}

// TODO this is inefficient, don't use
bool Errors::removeIf(const std::function<bool(ErrorItem const&)>& predicate) {
	bool changed = true;
	auto objIt = errors_.begin();
	while (objIt != errors_.end()) {
		auto& cont = objIt->second;
		auto it = cont.begin();
		while (it != cont.end()) {
			if (predicate(it->second)) {
				recorder_->recordErrorChanged(it->first);
				it = cont.erase(it);
				changed = true;
			} else {
				++it;
			}
		}
		if (cont.empty()) {
			objIt = errors_.erase(objIt);
		} else {
			++objIt;
		}
	}
	return changed;
}

const std::map<SCEditorObject, std::map<ValueHandle, ErrorItem>>& Errors::getAllErrors() const {
	return errors_;
}

}  // namespace raco::core
