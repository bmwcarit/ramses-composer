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

#include <memory>
#include <string>
#include <vector>

namespace raco::core {

class EditorObject;
using SEditorObject = std::shared_ptr<EditorObject>;

// PropertyDescriptors may be used even for properties that don't exist.
// Only the object itself is assumed to exist.
// This means that no access to the values is possible using them. If value access is required 
// one needs to construct a ValueHandle from the PropertyDescriptor.
class PropertyDescriptor {
public:
	PropertyDescriptor(SEditorObject object, const std::vector<std::string>& propertyNamesPath) : object_(object), propNames_(propertyNamesPath) {
	}

	PropertyDescriptor(SEditorObject object, std::vector<std::string>&& propertyNamesPath) : object_(object), propNames_(std::move(propertyNamesPath)) {
	}

	SEditorObject object() const {
		return object_;
	}

	const std::vector<std::string>& propertyNames() const {
		return propNames_;
	}

	// Return '.'-separated property path starting at the property root object
	std::string getPropertyPath(bool useObjectID = false) const;

	// Return '/'-separated property path starting at the root object in the scene hierarchy
	std::string getFullPropertyPath() const;
	
	bool operator==(const PropertyDescriptor& rhs) const;

	// Check if 'other' is nested strictly inside this property.
	// Returns false for equal properties.
	bool contains(const PropertyDescriptor& other) const;

private:
	SEditorObject object_;
	std::vector<std::string> propNames_;
};

}