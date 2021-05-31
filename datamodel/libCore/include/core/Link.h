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

#include "data_storage/AnnotationBase.h"
#include "data_storage/ReflectionInterface.h"
#include "data_storage/Table.h"
#include "data_storage/Value.h"
#include "data_storage/BasicAnnotations.h"

#include "core/Handles.h"
#include "core/PropertyDescriptor.h"

namespace raco::core {

using namespace raco::data_storage;

class Link;
using SLink = std::shared_ptr<Link>;


// The LinkDescriptor is needed by the ChangeRecorder and engine interface and is currently
// needed to avoid accidental pointer comparisons of std::shared_ptr<Link> objects.
// The pointer comparisons do not establish a valid identity relation for links (see discussion 
// of Link value semantic in that class).
struct LinkDescriptor {
	PropertyDescriptor start;
	PropertyDescriptor end;
	bool isValid{true};
};

bool operator==(const LinkDescriptor& lhs, const LinkDescriptor& rhs);
bool operator<(const LinkDescriptor& lhs, const LinkDescriptor& rhs);


//
// Link objects represent links between EditorObject properties in the data model
// - Link objects do not possess an identity beyond their content, i.e. they have value semantic
// - only the start and end object and property names participate in comparison and establish an identity
//   relation between links. The isValid flag is ignored in comparisons
// - links must start and end on existing objects in the data model
// - links may start/end on properties which don't exist (but the objects must exist).
//   this can occur for links on lua in/out properties or uniforms
// - link validity can change when the set of dynamic properties of objects changes.
//   currently this concerns lua in/out properties and meshnode uniform properties.
//
class Link : public ClassWithReflectedMembers {
public:
	static inline const TypeDescriptor typeDescription = {"Link", false};
	TypeDescriptor const& getTypeDescription() const override {
		return typeDescription;
	}
	bool serializationRequired() const override {
		return true;
	}

	// Needs to be default constructible for deserialization
	Link();
	Link(const Link& other) : ClassWithReflectedMembers(getProperties()), startObject_(other.startObject_), startProp_(other.startProp_), endObject_(other.endObject_), endProp_(other.endProp_), isValid_(other.isValid_) {}

	Link(const PropertyDescriptor& start, const PropertyDescriptor& end, bool isValid = true);

	bool isValid() const;

	std::vector<std::pair<std::string, ValueBase*>> getProperties();

	LinkDescriptor descriptor() const;

	std::vector<std::string> startPropertyNamesVector() const;
	std::vector<std::string> endPropertyNamesVector() const;

	PropertyDescriptor startProp() const;
	PropertyDescriptor endProp() const;

	// @return true if property names are equal
	bool compareStartPropertyNames(const std::vector<std::string>& propertyNames);

	// @return true if property names are equal
	bool compareEndPropertyNames(const std::vector<std::string>& propertyNames);

	static SLink cloneLinkWithTranslation(const SLink& link, std::function<SEditorObject(SEditorObject)> translateRef);

	
	// startProp_ and endProp_ contain Table of unnamed string property denoting the property path

	Value<SEditorObject> startObject_;
	Property<Table, ArraySemanticAnnotation> startProp_ {{}, {}};

	Value<SEditorObject> endObject_;
	Property<Table, ArraySemanticAnnotation> endProp_ {{}, {}};

	Value<bool> isValid_{true};
};

// Compare links but use object id and not SEditorObject pointer itself to compare start/end objects.
bool compareLinksByObjectID(const Link& left, const Link& right);

// LinkStartAnnotation tags properties as valid starting points in the data model
class LinkStartAnnotation : public AnnotationBase {
public:
	static inline const TypeDescriptor typeDescription = {"LinkStartAnnotation", false};
	TypeDescriptor const& getTypeDescription() const override {
		return typeDescription;
	}
	bool serializationRequired() const override {
		return false;
	}

	LinkStartAnnotation(const LinkStartAnnotation& other) : AnnotationBase({}) {}
	LinkStartAnnotation() : AnnotationBase({}) {}

	LinkStartAnnotation& operator=(const LinkStartAnnotation& other) {
		return *this;
	}
};

// LinkEndAnnotation tags properties as valid end points in the data model
class LinkEndAnnotation : public AnnotationBase {
public:
	static inline const TypeDescriptor typeDescription = {"LinkEndAnnotation", false};
	TypeDescriptor const& getTypeDescription() const override {
		return typeDescription;
	}
	bool serializationRequired() const override {
		return false;
	}

	LinkEndAnnotation(const LinkEndAnnotation& other) : AnnotationBase({}) {}
	LinkEndAnnotation() : AnnotationBase({}) {}

	LinkEndAnnotation& operator=(const LinkEndAnnotation& other) {
		return *this;
	}
};

}  // namespace raco::core