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

#include <charconv>
#include <memory>
#include <string>
#include <vector>

namespace raco::data_storage {

// Base class for all Array types
class ArrayBase : public ReflectionInterface {
public:
	static inline const TypeDescriptor typeDescription = {"Array", false};
	TypeDescriptor const& getTypeDescription() const override {
		return typeDescription;
	}
	bool serializationRequired() const override {
		return true;
	}

	virtual std::string typeName() const = 0;

	virtual PrimitiveType elementType() const = 0;

	virtual std::string elementTypeName() const = 0;

	// Create and insert property of statically known element type into array before the insertion_index
	// or at the end if insertion_index = -1
	virtual ValueBase* addProperty(int index_before = -1) = 0;

	// Insert user-created property before the insertion_index or at the end if insertion_index = -1.
	// Will check if the type of the property is identical to the array element type and throw exception if not.
	virtual ValueBase* addProperty(ValueBase* property, int index_before = -1) = 0;

	// Insert user-created property before the insertion_index or at the end if insertion_index = -1.
	// Will check if the type of the property is identical to the array element type and throw exception if not.
	virtual ValueBase* addProperty(std::unique_ptr<ValueBase>&& property, int index_before = -1) = 0;

	virtual void resize(size_t newSize) = 0;

	virtual void removeProperty(size_t index) = 0;

protected:
	void growPropertyNames(size_t requestedSize) const {
		if (requestedSize > propNames_.size()) {
			auto oldSize = propNames_.size();
			size_t newSize = std::max(2 * oldSize, requestedSize);
			propNames_.resize(newSize);
			for (size_t index = oldSize; index < propNames_.size(); index++) {
				propNames_[index] = std::to_string(index + 1);
			}
		}
	}

	static std::vector<std::string> propNames_;
};

// Concrete Array type with statically known element type
// - elements are of type Value<T>
template <typename T>
class Array : public ArrayBase {
public:
	Array() = default;

	Array(const Array& other, std::function<SEditorObject(SEditorObject)>* translateRef = nullptr) {
		for (auto const& item : other.elements_) {
			addProperty(item->staticClone(translateRef));
		}
	}

	Array& operator=(const Array& value) {
		elements_.clear();
		for (auto const& item : value.elements_) {
			addProperty(item->staticClone(nullptr));
		}
		return *this;
	}

	static std::string staticTypeName() {
		return "Array[" + Value<T>::staticTypeName() + "]";
	}

	///
	/// ReflectionInterface interface implementation:
	///

	Value<T>* get(std::string_view propertyName) override {
		int index;
		auto [ptr, error] = std::from_chars(propertyName.data(), propertyName.data() + propertyName.size(), index);
		if (error == std::errc() && ptr == propertyName.data() + propertyName.size() && index > 0) {
			return get(index - 1);
		}
		throw std::out_of_range("Array::get: property doesn't exist.");
	}

	// Note: the return type is more specific than the overriden ReflectionInterface function
	Value<T>* get(size_t index) override {
		if (index < elements_.size()) {
			return elements_[index].get();
		}
		throw std::out_of_range("Array::name: index out of range");
	}

	const Value<T>* get(std::string_view propertyName) const override {
		int index;
		auto [ptr, error] = std::from_chars(propertyName.data(), propertyName.data() + propertyName.size(), index);
		if (error == std::errc() && ptr == propertyName.data() + propertyName.size() && index > 0) {
			return get(index - 1);
		}
		throw std::out_of_range("Array::get: property doesn't exist.");
	}

	// Note: the return type is more specific than the overriden ReflectionInterface function
	const Value<T>* get(size_t index) const override {
		if (index < elements_.size()) {
			return elements_[index].get();
		}
		throw std::out_of_range("Array::name: index out of range");
	}

	size_t size() const override {
		return elements_.size();
	}

	int index(std::string_view propertyName) const override {
		int index;
		auto [ptr, error] = std::from_chars(propertyName.data(), propertyName.data() + propertyName.size(), index);
		if (error == std::errc() && ptr == propertyName.data() + propertyName.size() && index > 0) {
			return index - 1;
		}
		return -1;
	}

	const std::string& name(size_t index) const override {
		if (index >= elements_.size()) {
			throw std::out_of_range("Array<T>::name: index out of range");
		}
		growPropertyNames(index + 1);
		return propNames_[index];
	}

	///
	/// ArrayBase interface implementation:
	///

	std::string typeName() const override {
		return staticTypeName();
	}

	PrimitiveType elementType() const override {
		return Value<T>::staticType();
	}

	std::string elementTypeName() const override {
		return Value<T>::staticTypeName();
	}

	Value<T>* addProperty(int index_before = -1) override {
		if (index_before < -1 || index_before > static_cast<int>(elements_.size())) {
			throw std::out_of_range("Array<T>::addProperty: index out of range");
		}

		if (index_before == -1) {
			return elements_.emplace_back(std::unique_ptr<Value<T>>(new Value<T>())).get();
		}
		return elements_.insert(elements_.begin() + index_before, std::unique_ptr<Value<T>>(new Value<T>()))->get();
	}

	Value<T>* addProperty(ValueBase* property, int index_before = -1) override {
		auto* vp = dynamic_cast<Value<T>*>(property);
		if (!vp) {
			throw std::runtime_error("Array<T>::addProperty: type mismatch");
		}

		if (index_before < -1 || index_before > static_cast<int>(elements_.size())) {
			throw std::out_of_range("Array<T>::addProperty: index out of range");
		}

		if (index_before == -1) {
			return elements_.emplace_back(std::unique_ptr<Value<T>>(vp)).get();
		}
		return elements_.insert(elements_.begin() + index_before, std::unique_ptr<Value<T>>(vp))->get();
	}

	Value<T>* addProperty(std::unique_ptr<Value<T>>&& property, int index_before = -1) {
		if (index_before < -1 || index_before > static_cast<int>(elements_.size())) {
			throw std::out_of_range("Array<T>::addProperty: index out of range");
		}

		if (index_before == -1) {
			return elements_.emplace_back(std::move(property)).get();
		}
		return elements_.insert(elements_.begin() + index_before, std::move(property))->get();
	}

	ValueBase* addProperty(std::unique_ptr<ValueBase>&& property, int index_before = -1) override {
		auto* vp = dynamic_cast<Value<T>*>(property.release());
		if (!vp) {
			throw std::runtime_error("Array<T>::addProperty: type mismatch");
		}

		if (index_before < -1 || index_before > static_cast<int>(elements_.size())) {
			throw std::out_of_range("Array<T>::addProperty: index out of range");
		}

		if (index_before == -1) {
			return elements_.emplace_back(std::unique_ptr<Value<T>>(vp)).get();
		}
		return elements_.insert(elements_.begin() + index_before, std::unique_ptr<Value<T>>(vp))->get();
	}

	void resize(size_t newSize) override {
		if (newSize < 0) {
			throw std::out_of_range("Array<T>::resize: negative size not allowed");
		}
		auto oldSize = elements_.size();
		elements_.resize(newSize);
		for (size_t index = oldSize; index < elements_.size(); index++) {
			elements_[index] = std::make_unique<Value<T>>();
		}
	}

	void removeProperty(size_t index) override {
		if (index >= static_cast<int>(elements_.size())) {
			throw std::out_of_range("Array<T>::removeProperty: index out of range");
		}
		elements_.erase(elements_.begin() + index);
	}

	void copyAnnotationData(const Array& other) {
		if (elements_.size() != other.elements_.size()) {
			throw std::runtime_error("Array<T>::copyAnnotationData: array size mismatch");
		}
		for (size_t index = 0; index < elements_.size(); index++) {
			elements_[index]->copyAnnotationData(*other.elements_[index].get());
		}
	}

	template <typename U>
	typename std::enable_if<std::is_convertible<T, U>::value, std::vector<U>>::type
	asVector() const {
		std::vector<U> result;
		for (size_t index = 0; index < elements_.size(); index++) {
			result.emplace_back(**elements_[index]);
		}
		return result;
	}

	void set(const std::vector<T>& data) {
		resize(data.size());
		for (size_t index = 0; index < data.size(); index++) {
			*elements_[index] = data[index];
		}
	}

	bool compare(std::vector<T> const& array) const {
		if (elements_.size() != array.size()) {
			return false;
		}
		for (size_t index = 0; index < elements_.size(); index++) {
			if (**elements_[index] != array[index]) {
				return false;
			}
		}
		return true;
	}

	bool compare(const Array& other) const {
		if (elements_.size() != other.elements_.size()) {
			return false;
		}
		for (size_t index = 0; index < elements_.size(); index++) {
			if (**elements_[index] != **other.elements_[index]) {
				return false;
			}
		}
		return true;
	}

private:
	std::vector<std::unique_ptr<Value<T>>> elements_;
};

}  // namespace raco::data_storage