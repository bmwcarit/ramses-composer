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

#include "ReflectionInterface.h"
#include "Value.h"

#include <vector>
#include <set>
#include <string>
#include <memory>

namespace raco::data_storage {

// Dictionary with annotations
class Table : public ReflectionInterface {
public:
	static inline const TypeDescriptor typeDescription = { "Table", false };
	TypeDescriptor const& getTypeDescription() const override {
		return typeDescription;
	}
	bool serializationRequired() const override {
		return true;
	}
	Table() = default;

	// Performs deep copy of all property values of the argument
	Table(const Table&, std::function<SEditorObject(SEditorObject)>* translateRef = nullptr);

	virtual ValueBase* get(std::string_view propertyName) override;
	virtual ValueBase* get(size_t index) override;

	virtual const ValueBase* get(std::string_view propertyName) const override;
	virtual const ValueBase* get(size_t index) const override;

	virtual size_t size() const override;

	virtual int index(std::string_view propertyName) const override;
	virtual const std::string& name(size_t index) const override;


	std::vector<std::string> propertyNames() const;
	std::set<std::string> propertyNameSet() const;

	// array and dictionary interface
	// can add and remove array entries / named properties

	// Add named property at the end of the property list.
	ValueBase *addProperty(std::string_view name, PrimitiveType type);
	
	// Add user-created property; Table will take over ownership of object.
	ValueBase* addProperty(std::string_view name, ValueBase* property, int index_before = -1);

	// Add user-created property; Table will take over ownership of object.
	ValueBase* addProperty(std::string_view name, std::unique_ptr<ValueBase>&& property, int index_before = -1);

	// Insert new property at specified position into the property list.
	// If index_before = -1 the property is appended, otherwise it will be insert before the
	// current element at the position.
	ValueBase* addProperty(PrimitiveType type, int index_before = -1);
	ValueBase* addProperty(ValueBase* property, int index_before = -1);
	ValueBase* addProperty(std::unique_ptr<ValueBase>&& property, int index_before = -1);

	void removeProperty(size_t index);
	void removeProperty(std::string_view propertyName);

	void renameProperty(std::string_view oldName, std::string_view newName);

	void replaceProperty(size_t index, ValueBase* property);
	void replaceProperty(std::string_view name, ValueBase* property);

	void swapProperties(size_t index_1, size_t index_2);

	// Remove all properties.
	void clear();
	
	// Set the entire Table contents to 'array'
	// Clears the Table, adds unnamed properties compatible with type T for all array elements
	// Currently works only for T=std::string
	template<typename T>
	void set(std::vector<T> const& array);

	// Construct and return Table contents as vector.
	// Assumes that all Table properties are of type T.
	template<typename T>
	std::vector<T> asVector() const;

	Table& operator=(const Table& value);

	// Compare all Table property value with the input vector.
	// Assumes that all Table properties are of type T.
	// @return true if equal
	template <typename T>
	bool compare(std::vector<T> const& array) const;

private:
	std::vector<std::pair<std::string, std::unique_ptr<ValueBase>>> properties_;
};

}