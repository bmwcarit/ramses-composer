/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "data_storage/ReflectionInterface.h"
#include "data_storage/Value.h"
#include "data_storage/AnnotationBase.h"

#include <algorithm>
#include <stdexcept>
#include <cassert>

namespace raco::data_storage {

ValueBase* ReflectionInterface::operator[](std::string const& propertyName)
{
	return get(propertyName);
}

ValueBase* ReflectionInterface::operator[](size_t index)
{
	return get(index);
}



ValueBase* ClassWithReflectedMembers::get(std::string const& propertyName) {
	auto it = std::find_if(properties_.begin(), properties_.end(),
		[&propertyName](auto const& item) {
			return item.first == propertyName;
		});
	if (it != properties_.end()) {
		return it->second;
	}
	return nullptr;
}

ValueBase* ClassWithReflectedMembers::get(size_t index) {
	if (index < properties_.size()) {
		return properties_[index].second;
	}
	return nullptr;
}

const ValueBase* ClassWithReflectedMembers::get(std::string const& propertyName) const {
	auto it = std::find_if(properties_.begin(), properties_.end(),
		[&propertyName](auto const& item) {
			return item.first == propertyName;
		});
	if (it != properties_.end()) {
		return it->second;
	}
	return nullptr;
}

const ValueBase* ClassWithReflectedMembers::get(size_t index) const {
	if (index < properties_.size()) {
		return properties_[index].second;
	}
	return nullptr;
}

size_t ClassWithReflectedMembers::size() const {
	return properties_.size();
}

int ClassWithReflectedMembers::index(std::string const& propertyName) const {
	auto it = std::find_if(properties_.begin(), properties_.end(),
		[&propertyName](auto const& item) {
			return item.first == propertyName;
		});
	if (it != properties_.end()) {
		return static_cast<int>(it - properties_.begin());
	}
	return -1;
}

std::string ClassWithReflectedMembers::name(size_t index) const {
	assert(index < properties_.size());
	return properties_[index].first;
}

bool ReflectionInterface::hasProperty(std::string const& propertyName) const {
	return index(propertyName) != -1;
}

bool ReflectionInterface::operator==(const ReflectionInterface& other) const {
	if (size() != other.size()) {
		return false;
	}
	for (size_t index{0}; index < size(); index++) {
		if (name(index) != other.name(index)) {
			return false;
		}
		if (!(*get(index) == *other.get(index))) {
			return false;
		}
	}
	return true;
}

std::shared_ptr<AnnotationBase> ClassWithReflectedMembers::query(const std::string& typeName) {
	for (auto anno : annotations_ ) {
		if (anno->serializationTypeName() == typeName) {
			return anno;
		}
	}
	return nullptr;
}

std::shared_ptr<const AnnotationBase> ClassWithReflectedMembers::query(const std::string& typeName) const {
	for (auto anno : annotations_) {
		if (anno->serializationTypeName() == typeName) {
			return anno;
		}
	}
	return nullptr;
}

void ClassWithReflectedMembers::addAnnotation(std::shared_ptr<AnnotationBase> annotation) {
	annotations_.emplace_back(annotation);
}

void ClassWithReflectedMembers::removeAnnotation(std::shared_ptr<AnnotationBase> annotation) {
	auto it = std::find(annotations_.begin(), annotations_.end(), annotation);
	if (it != annotations_.end()) {
		annotations_.erase(it);
	}
}

const std::vector<std::shared_ptr<AnnotationBase>>& ClassWithReflectedMembers::annotations() const {
	return annotations_;
}

}
