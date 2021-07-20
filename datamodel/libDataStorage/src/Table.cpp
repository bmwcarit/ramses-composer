/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "data_storage/Table.h"
#include "data_storage/Value.h"

#include <algorithm>
#include <cassert>
#include <tuple>

namespace raco::data_storage {

Table::Table(const Table& other, std::function<SEditorObject(SEditorObject)>* translateRef) {
	for (auto const& item : other.properties_) {
		addProperty(item.first, item.second->clone(translateRef));
	}
}

ValueBase* Table::get(std::string const& propertyName) {
	auto it = std::find_if(properties_.begin(), properties_.end(),
		[&propertyName](auto const& item) {
			return item.first == propertyName;
		});
	if (it != properties_.end()) {
		return it->second.get();
	}
	return nullptr;
}

ValueBase* Table::get(size_t index) {
	if (index < properties_.size()) {
		return properties_[index].second.get();
	}
	return nullptr;
}

const ValueBase* Table::get(std::string const& propertyName) const {
	auto it = std::find_if(properties_.begin(), properties_.end(),
		[&propertyName](auto const& item) {
			return item.first == propertyName;
		});
	if (it != properties_.end()) {
		return it->second.get();
	}
	return nullptr;
}

const ValueBase* Table::get(size_t index) const {
	if (index < properties_.size()) {
		return properties_[index].second.get();
	}
	return nullptr;
}


size_t Table::size() const {
	return properties_.size();
}

std::string Table::name(size_t index) const {
	assert(index < properties_.size());
	return properties_[index].first;
}

int Table::index(std::string const& propertyName) const {
	auto it = std::find_if(properties_.begin(), properties_.end(),
		[&propertyName](auto const& item) {
			return item.first == propertyName;
		});
	if (it != properties_.end()) {
		return static_cast<int>(it - properties_.begin());
	}
	return -1;
}


ValueBase *Table::addProperty(std::string const &name, PrimitiveType type)
{
	properties_.emplace_back(std::make_pair(name, ValueBase::create(type)));
	return properties_.back().second.get();
}

ValueBase* Table::addProperty(std::string const& name, ValueBase* property, int index_before) {
	assert(index_before >= -1 && index_before <= static_cast<int>(properties_.size()));

	if (index_before == -1) {
		properties_.emplace_back(std::make_pair(name, std::unique_ptr<ValueBase>(property)));
		return properties_.back().second.get();
	}

	return properties_.insert(properties_.begin() + index_before, std::make_pair(name, std::unique_ptr<ValueBase>(property)))->second.get();
}

ValueBase* Table::addProperty(const std::string& name, std::unique_ptr<ValueBase>&& property) {
	properties_.emplace_back(std::make_pair(name, std::move(property)));
	return properties_.back().second.get();
}


ValueBase* Table::addProperty(PrimitiveType type, int index_before) {
	assert(index_before >= -1 && index_before <= static_cast<int>(properties_.size()));

	if (index_before == -1) {
		properties_.emplace_back(std::make_pair(std::string(), ValueBase::create(type)));
		return properties_.back().second.get();
	}

	return properties_.insert(properties_.begin() + index_before, std::make_pair(std::string(), ValueBase::create(type)))->second.get();
}

ValueBase* Table::addProperty(ValueBase* property, int index_before) {
	return addProperty(std::unique_ptr<ValueBase>(property), index_before);
}

ValueBase* Table::addProperty(std::unique_ptr<ValueBase>&& property, int index_before) {
	assert(index_before >= -1 && index_before <= static_cast<int>(properties_.size()));

	if (index_before == -1) {
		properties_.emplace_back(std::make_pair(std::string(), std::move(property)));
		return properties_.back().second.get();
	}

	return properties_.insert(properties_.begin() + index_before, std::make_pair(std::string(), std::move(property)))->second.get();
}

void Table::removeProperty(size_t index) {
	assert(index < properties_.size());
	properties_.erase(properties_.begin() + index);
}

void Table::removeProperty(std::string const &propertyName) {
	int ind = index(propertyName);
	if (ind != -1) {
		removeProperty(ind);
	}
}

void Table::renameProperty(const std::string& oldName, const std::string& newName) {
	auto it = std::find_if(properties_.begin(), properties_.end(),
		[&oldName](auto const& item) {
			return item.first == oldName;
		});
	if (it != properties_.end()) {
		it->first = newName;
	}
}

void Table::replaceProperty(size_t index, ValueBase* property) {
	if (index < properties_.size()) {
		properties_[index].second = std::unique_ptr<ValueBase>(property);
	}
}


void Table::clear() {
	properties_.clear();
}

template<typename T>
struct TypeMap;

template<> struct TypeMap<std::string> { static const PrimitiveType primType = PrimitiveType::String; };

template<typename T>
void Table::set(std::vector<T> const& array) {
	properties_.clear();

	for (auto item : array) {
		ValueBase* prop = addProperty(TypeMap<T>::primType);
		prop->set(item);
	}
}

template void Table::set<std::string>(std::vector<std::string> const& array);


template<typename T>
std::vector<T> Table::asVector() const {

	std::vector<T> result;
	for (auto const &prop : properties_) {
		result.push_back(prop.second->as<T>());
	}
	return result;
}

template<>
std::vector<SEditorObject> Table::asVector<SEditorObject>() const {

	std::vector<SEditorObject> result;
	for (auto const& prop : properties_) {
		result.push_back(prop.second->asRef());
	}
	return result;
}

template std::vector<std::string> Table::asVector<std::string>() const;


template <typename T>
bool Table::compare(std::vector<T> const& array) const {
	if (array.size() != properties_.size()) {
		return false;
	}
	for (size_t i = 0; i < properties_.size(); i++) {
		if (properties_[i].second->as<T>() != array[i]) {
			return false;
		}
	}
	return true;
}

template bool Table::compare<std::string>(std::vector<std::string> const& array) const;


Table& Table::operator=(const Table& value) {
	properties_.clear();
	for (auto const &item : value.properties_) {
		ValueBase* prop = addProperty(item.first, item.second->type());
		*prop = *item.second;
	}
	return *this;
}

std::vector<std::string> Table::propertyNames() const {
	std::vector<std::string> result;
	for (auto const& prop : properties_) {
		result.emplace_back(prop.first);
	}
	return result;
}

}
