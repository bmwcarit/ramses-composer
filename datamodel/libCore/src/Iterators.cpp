/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "core/Iterators.h"

namespace raco::core {

ValueTreeIterator::ValueTreeIterator(ValueHandle root, ValueHandle current) : root_(root), current_(current) {
}

ValueTreeIterator::operator bool() const {
	return current_.depth() > root_.depth();
}

bool ValueTreeIterator::operator!=(const ValueTreeIterator& other) const {
	return !(root_ == other.root_ && current_ == other.current_);
}

ValueHandle ValueTreeIterator::operator*() const {
	if (*this) {
		return current_;
	}
	return ValueHandle(nullptr);
}

const ValueHandle* ValueTreeIterator::operator->() const {
	if (*this) {
		return &current_;
	}
	return nullptr;
}

ValueTreeIterator ValueTreeIterator::normalized(const ValueTreeIterator &it) {
	ValueTreeIterator normalized(it);
	while (normalized.current_.depth() > normalized.root_.depth() && !static_cast<bool>(normalized.current_)) {
		normalized.current_ = normalized.current_.parent();
	}
	return normalized;
}

ValueTreeIterator& ValueTreeIterator::operator++() {
	if (*this) {
		if (current_.size() > 0) {
			current_ = current_[0];
		} else {
			while (current_.depth() > root_.depth() && !static_cast<bool>(current_.nextSibling())) {
				current_ = current_.parent();
			}
		}
	}
	return *this;
}

}  // namespace raco::core