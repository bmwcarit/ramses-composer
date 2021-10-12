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

#include "AnnotationBase.h"
#include "Value.h"

namespace raco::data_storage {

class URIAnnotation : public AnnotationBase {
public:
	static inline const TypeDescriptor typeDescription = { "URIAnnotation", false };
	TypeDescriptor const& getTypeDescription() const override {
		return typeDescription;
	}
	bool serializationRequired() const override {
		return false;
	}
	URIAnnotation(const URIAnnotation& other) : AnnotationBase({{"filter", &filter_}}),
                                                filter_(other.filter_) {
    }

	URIAnnotation(const std::string& filter = "") : AnnotationBase({{"filter", &filter_}}),
                                          filter_(filter) {
    }

	URIAnnotation& operator=(const URIAnnotation& other) {
		filter_ = other.filter_;
		return *this;
	}

    virtual const std::string& getFilter() {
		return *filter_;
	}

    Value<std::string> filter_;
};

class HiddenProperty : public AnnotationBase {
public:
	static inline const TypeDescriptor typeDescription = { "HiddenProperty", false };
	TypeDescriptor const& getTypeDescription() const override {
		return typeDescription;
	}
	bool serializationRequired() const override {
		return false;
	}
	HiddenProperty(const HiddenProperty& other) : AnnotationBase({}) {}
	HiddenProperty() : AnnotationBase({}) {}

	HiddenProperty& operator=(const HiddenProperty& other) {
		return *this;
	}
};

template <typename T>
class RangeAnnotation : public AnnotationBase {
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

	Value<T> min_, max_;
};

class DisplayNameAnnotation : public AnnotationBase {
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

	Value<std::string> name_;
};

template<> inline const raco::data_storage::ReflectionInterface::TypeDescriptor raco::data_storage::RangeAnnotation<double>::typeDescription {
	"RangeAnnotationDouble", false
};
template<> inline const raco::data_storage::ReflectionInterface::TypeDescriptor raco::data_storage::RangeAnnotation<int>::typeDescription {
	"RangeAnnotationInt", false
};

// Tag a Table with array semantic. This implies
// - the properties in the Table have empty property names
// - PrimitiveType::Ref properties in the Table must not contain nullptrs.
//   Instead of setting a reference property to nullptr the property needs to be removed.
class ArraySemanticAnnotation : public AnnotationBase {
public:
	static inline const TypeDescriptor typeDescription = { "ArraySemanticAnnotation", false };
	TypeDescriptor const& getTypeDescription() const override {
		return typeDescription;
	}
	bool serializationRequired() const override {
		return false;
	}
	ArraySemanticAnnotation(const ArraySemanticAnnotation& other) : AnnotationBase({}) {}
	ArraySemanticAnnotation() : AnnotationBase({}) {}
	ArraySemanticAnnotation& operator=(const ArraySemanticAnnotation& other) {
		return *this;
	}
};

class TagContainerAnnotation : public AnnotationBase {
public:
	static inline const TypeDescriptor typeDescription = {"TagContainerAnnotation", false};
	TypeDescriptor const& getTypeDescription() const override {
		return typeDescription;
	}
	bool serializationRequired() const override {
		return false;
	}
	TagContainerAnnotation(const TagContainerAnnotation& other) : AnnotationBase({}) {}
	TagContainerAnnotation() : AnnotationBase({}) {}
	TagContainerAnnotation& operator=(const TagContainerAnnotation& other) {
		return *this;
	}
};

class RenderableTagContainerAnnotation : public AnnotationBase {
public:
	static inline const TypeDescriptor typeDescription = {"RenderableTagContainerAnnotation", false};
	TypeDescriptor const& getTypeDescription() const override {
		return typeDescription;
	}
	bool serializationRequired() const override {
		return false;
	}
	RenderableTagContainerAnnotation(const RenderableTagContainerAnnotation& other) : AnnotationBase({}) {}
	RenderableTagContainerAnnotation() : AnnotationBase({}) {}
	RenderableTagContainerAnnotation& operator=(const RenderableTagContainerAnnotation& other) {
		return *this;
	}
};

class EnumerationAnnotation : public AnnotationBase {
public:
	static inline const TypeDescriptor typeDescription = { "EnumerationAnnotation", false };
	TypeDescriptor const& getTypeDescription() const override {
		return typeDescription;
	}
	bool serializationRequired() const override {
		return false;
	}
	EnumerationAnnotation(EnumerationAnnotation const& other) : AnnotationBase({{"type", &type_}}),
																type_(other.type_) {}

	EnumerationAnnotation(const int type = 0) : AnnotationBase({{"type", &type_}}),
															  type_(type) {
	}

	EnumerationAnnotation& operator=(const EnumerationAnnotation& other) {
		type_ = other.type_;
		return *this;
	}

	Value<int> type_;
};

class ExpectEmptyReference : public AnnotationBase {
public:
	static inline const TypeDescriptor typeDescription = {"EmptyReferenceAllowable", false};
	TypeDescriptor const& getTypeDescription() const override {
		return typeDescription;
	}
	bool serializationRequired() const override {
		return false;
	}
	ExpectEmptyReference(ExpectEmptyReference const& other) : AnnotationBase({}) {}
	ExpectEmptyReference() : AnnotationBase({}) {}
	ExpectEmptyReference& operator=(const ExpectEmptyReference& other) {
		return *this;
	}
};

}  // namespace raco::data_storage