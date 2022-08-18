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

class URIAnnotation : public raco::data_storage::AnnotationBase {
public:
	static inline const TypeDescriptor typeDescription = { "URIAnnotation", false };

	// Filter for relative directory URIs that stay the same relative to the project (and do not get re-rooted), when the project file path changes.
	static inline const std::string projectSubdirectoryFilter {"projectSubDir"};

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

	bool isProjectSubdirectoryURI() const {
		return filter_.asString() == projectSubdirectoryFilter;
	}

    raco::data_storage::Value<std::string> filter_;
};

class HiddenProperty : public raco::data_storage::AnnotationBase {
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

// Tag a Table with array semantic. This implies
// - the properties in the Table have empty property names
// - PrimitiveType::Ref properties in the Table must not contain nullptrs.
//   Instead of setting a reference property to nullptr the property needs to be removed.
class ArraySemanticAnnotation : public raco::data_storage::AnnotationBase {
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

class TagContainerAnnotation : public raco::data_storage::AnnotationBase {
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

class RenderableTagContainerAnnotation : public raco::data_storage::AnnotationBase {
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

class EnumerationAnnotation : public raco::data_storage::AnnotationBase {
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

	// This is really a raco::core::EngineEnumeration
	raco::data_storage::Value<int> type_;
};

class ExpectEmptyReference : public raco::data_storage::AnnotationBase {
public:
	static inline const TypeDescriptor typeDescription = {"EmptyReferenceAllowable", false};
	TypeDescriptor const& getTypeDescription() const override {
		return typeDescription;
	}
	bool serializationRequired() const override {
		return false;
	}

	ExpectEmptyReference(ExpectEmptyReference const& other) : AnnotationBase({{"label", &emptyRefLabel_}}),
															  emptyRefLabel_(other.emptyRefLabel_) {}

	ExpectEmptyReference(std::string emptyRefLabel = std::string()) : AnnotationBase({{"label", &emptyRefLabel_}}),
															 emptyRefLabel_(emptyRefLabel) {
	}

	ExpectEmptyReference& operator=(const ExpectEmptyReference& other) {
		emptyRefLabel_ = other.emptyRefLabel_;
		return *this;
	}

	raco::data_storage::Value<std::string> emptyRefLabel_;
};

}  // namespace raco::data_storage