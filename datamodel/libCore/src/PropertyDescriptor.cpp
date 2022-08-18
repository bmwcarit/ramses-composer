/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "core/PropertyDescriptor.h"
#include "core/EditorObject.h"
#include "core/Queries.h"

namespace raco::core {

std::string PropertyDescriptor::getPropertyPath(bool useObjectID) const {
	std::string propPath;
	if (useObjectID) {
		propPath = object_->objectID();
	} else {
		propPath = object_->objectName();
	}
	for (const std::string& name : propNames_) {
		propPath += "." + name;
	}
	return propPath;
}

std::string PropertyDescriptor::getFullPropertyPath() const {
	auto hierarchyPath = raco::core::Queries::getFullObjectHierarchyPath(object_->getParent());
	if (!hierarchyPath.empty()) {
		hierarchyPath.append("/");
	}
	return hierarchyPath + getPropertyPath(false);
}

bool PropertyDescriptor::operator==(const PropertyDescriptor& rhs) const {
	return object_ == rhs.object_ && propNames_ == rhs.propNames_;
}

bool PropertyDescriptor::contains(const PropertyDescriptor& other) const {
	return object_ == other.object_ && propNames_.size() < other.propNames_.size() &&
		   std::equal(propNames_.begin(), propNames_.end(), other.propNames_.begin(), other.propNames_.begin() + propNames_.size());
}

PropertyDescriptor PropertyDescriptor::child(const std::string& name) const {
	std::vector<std::string> names{propNames_};
	names.emplace_back(name);
	return PropertyDescriptor(object(), names);
}

}  // namespace raco::core
