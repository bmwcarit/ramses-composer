/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "core/ErrorItem.h"

#include <cassert>

namespace raco::core {

ErrorItem::ErrorItem()
	: category_(ErrorCategory::GENERAL),
	  level_(ErrorLevel::NONE),
	  message_(),
	  handle_() {
}

ErrorItem::ErrorItem(ErrorCategory category, ErrorLevel level, const ValueHandle& handle, const std::string& message)
	: category_{category}, level_{level}, handle_{handle}, message_{message} {
	// ErrorLevel::NONE is just for convenience at should never be used to initialize an ErrorItem
	assert(level != ErrorLevel::NONE);
}

ValueHandle ErrorItem::valueHandle() const noexcept {
	return handle_;
}

const std::string& ErrorItem::message() const noexcept {
	return message_;
}

ErrorCategory ErrorItem::category() const noexcept {
	return category_;
}

ErrorLevel ErrorItem::level() const noexcept {
	return level_;
}

bool ErrorItem::operator==(const ErrorItem& other) const {
	return other.category_ == category_ && other.level_ == level_ && other.handle_ == handle_ && other.message_ == message_;
}

};	// namespace raco::core
