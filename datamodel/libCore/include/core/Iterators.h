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

#include "data_storage/ReflectionInterface.h"
#include "data_storage/Value.h"

#include "core/Handles.h"

#include <stack>

namespace raco::core {

using namespace raco::data_storage;

class ValueTreeIterator {
public:
	ValueTreeIterator(ValueHandle root, ValueHandle current);

	ValueHandle operator*() const;
	const ValueHandle* operator->() const;
	
	bool operator!=(const ValueTreeIterator& other) const;
	ValueTreeIterator& operator++();
	
private:
	friend class BaseContext;

	static ValueTreeIterator normalized(const ValueTreeIterator& it);
		
	operator bool() const;

	ValueHandle root_;
	ValueHandle current_;
};

class ValueTreeIteratorAdaptor {
public:
	ValueTreeIteratorAdaptor(const ValueHandle& value) : root_(value) {}

	ValueTreeIterator begin() {
		return ValueTreeIterator(root_, root_.size() > 0 ? root_[0] : root_);
	}
	ValueTreeIterator end() {
		return ValueTreeIterator(root_, root_);
	}

private:
	ValueHandle root_;
};

}  // namespace raco::core

template <>
struct std::iterator_traits<raco::core::ValueTreeIterator> {
	using value_type = raco::core::ValueHandle;
	using reference = raco::core::ValueHandle&;
	using iterator_category = std::forward_iterator_tag;
};
