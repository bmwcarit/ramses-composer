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

#include "data_storage/AnnotationBase.h"
#include "data_storage/Value.h"

namespace raco::core {

template <typename T>
class RangeAnnotation : public data_storage::AnnotationBase {
public:
	static const TypeDescriptor typeDescription;
	TypeDescriptor const& getTypeDescription() const override {
		return typeDescription;
	}
	bool serializationRequired() const override {
		return true;
	}
	RangeAnnotation(RangeAnnotation const& other) : AnnotationBase({{"min", &min_}, {"max", &max_}}),
													min_(other.min_),
													max_(other.max_) {}

	RangeAnnotation(T min = 0, T max = 1) : AnnotationBase({{"min", &min_}, {"max", &max_}}),
											min_(min),
											max_(max) {
	}

	RangeAnnotation& operator=(const RangeAnnotation& other) {
		min_ = other.min_;
		max_ = other.max_;
		return *this;
	}

	virtual T getMin() {
		return *min_;
	}
	virtual T getMax() {
		return *max_;
	}

	data_storage::Value<T> min_, max_;
};

class DisplayNameAnnotation : public data_storage::AnnotationBase {
public:
	static inline const TypeDescriptor typeDescription = { "DisplayNameAnnotation", false };
	TypeDescriptor const& getTypeDescription() const override {
		return typeDescription;
	}
	bool serializationRequired() const override {
		return false;
	}
	DisplayNameAnnotation(DisplayNameAnnotation const& other) : AnnotationBase({{"name", &name_}}),
																name_(other.name_) {}

	DisplayNameAnnotation(std::string name = std::string()) : AnnotationBase({{"name", &name_}}),
															  name_(name) {
	}

	DisplayNameAnnotation& operator=(const DisplayNameAnnotation& other) {
		name_ = other.name_;
		return *this;
	}

	data_storage::Value<std::string> name_;
};

template<> inline const data_storage::ReflectionInterface::TypeDescriptor core::RangeAnnotation<double>::typeDescription {
	"RangeAnnotationDouble", false
};
template<> inline const data_storage::ReflectionInterface::TypeDescriptor core::RangeAnnotation<int>::typeDescription {
	"RangeAnnotationInt", false
};

}  // namespace raco::data_storage