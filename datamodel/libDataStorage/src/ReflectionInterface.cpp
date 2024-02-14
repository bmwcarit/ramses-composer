/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "data_storage/ReflectionInterface.h"
#include "data_storage/Value.h"
#include "data_storage/AnnotationBase.h"

#include <algorithm>
#include <stdexcept>

namespace raco::data_storage {

ValueBase* ReflectionInterface::operator[](std::string_view propertyName)
{
	return get(propertyName);
}

ValueBase* ReflectionInterface::operator[](size_t index)
{
	return get(index);
}

const ValueBase* ReflectionInterface::operator[](std::string_view propertyName) const {
	return get(propertyName);
}

const ValueBase* ReflectionInterface::operator[](size_t index) const {
	return get(index);
}

ValueBase* ClassWithReflectedMembers::get(std::string_view propertyName) {
	auto it = std::find_if(properties_.begin(), properties_.end(),
		[&propertyName](auto const& item) {
			return item.first == propertyName;
		});
	if (it != properties_.end()) {
		return it->second;
	}
	throw std::out_of_range("ClassWithReflectedMembers::get: property doesn't exist.");
}

ValueBase* ClassWithReflectedMembers::get(size_t index) {
	if (index < properties_.size()) {
		return properties_[index].second;
	}
	throw std::out_of_range("ClassWithReflectedMembers::get: index out of range.");
}

const ValueBase* ClassWithReflectedMembers::get(std::string_view propertyName) const {
	auto it = std::find_if(properties_.begin(), properties_.end(),
		[&propertyName](auto const& item) {
			return item.first == propertyName;
		});
	if (it != properties_.end()) {
		return it->second;
	}
	throw std::out_of_range("ClassWithReflectedMembers::get: property doesn't exist.");
}

const ValueBase* ClassWithReflectedMembers::get(size_t index) const {
	if (index < properties_.size()) {
		return properties_[index].second;
	}
	throw std::out_of_range("ClassWithReflectedMembers::get: index out of range.");
}

size_t ClassWithReflectedMembers::size() const {
	return properties_.size();
}

int ClassWithReflectedMembers::index(std::string_view propertyName) const {
	auto it = std::find_if(properties_.begin(), properties_.end(),
		[&propertyName](auto const& item) {
			return item.first == propertyName;
		});
	if (it != properties_.end()) {
		return static_cast<int>(it - properties_.begin());
	}
	return -1;
}

const std::string& ClassWithReflectedMembers::name(size_t index) const {
	if (index >= properties_.size()) {
		throw std::out_of_range("ClassWithReflectedMembers::name: index out of range");
	}
	return properties_[index].first;
}

bool ReflectionInterface::hasProperty(std::string_view propertyName) const {
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

bool ReflectionInterface::compare(const ReflectionInterface& left, const ReflectionInterface& right, std::function<SEditorObject(SEditorObject)> translateRefLeftToRight) {
	if (left.size() != right.size()) {
		return false;
	}
	for (size_t index{0}; index < left.size(); index++) {
		if (left.name(index) != right.name(index)) {
			return false;
		}

		if (!(left.get(index)->compare(*right.get(index), translateRefLeftToRight))) {
			return false;
		}
	}
	return true;
}


std::shared_ptr<AnnotationBase> ClassWithReflectedMembers::query(std::string_view typeName) {
	for (auto anno : annotations_ ) {
		if (anno->serializationTypeName() == typeName) {
			return anno;
		}
	}
	return nullptr;
}

std::shared_ptr<const AnnotationBase> ClassWithReflectedMembers::query(std::string_view typeName) const {
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
