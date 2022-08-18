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

#include "data_storage/Value.h"

#include "core/EditorObject.h"

#include <cassert>
#include <memory>

namespace raco::serialization::proxy {

using raco::data_storage::ValueBase;

class DynamicPropertyInterface {
public:
	virtual ValueBase* addProperty(std::string const& name, raco::data_storage::PrimitiveType type) = 0;
	virtual ValueBase* addProperty(std::string const& name, ValueBase* property, int index_before = -1) = 0;
	virtual ValueBase* addProperty(std::string const& name, std::unique_ptr<ValueBase>&& property, int index_before) = 0;

	virtual void removeProperty(std::string const& propertyName) = 0;
	virtual void removeAllProperties() = 0;
	virtual std::unique_ptr<ValueBase> extractProperty(std::string const& propertyName) = 0;
};

template <class T>
class DynamicPropertyMixin : public DynamicPropertyInterface {
public:
	raco::data_storage::ValueBase* addProperty(std::string const& name, raco::data_storage::PrimitiveType type) override {
		T& asT = static_cast<T&>(*this);
		assert(asT.get(name) == nullptr);
		auto prop = dynamicProperties_.addProperty(name, type);
		asT.properties_.emplace_back(name, prop);
		return prop;
	}

	ValueBase* addProperty(std::string const& name, ValueBase* property, int index_before) override {
		T& asT = static_cast<T&>(*this);
		assert(asT.get(name) == nullptr);
		auto prop = dynamicProperties_.addProperty(name, property, index_before);
		asT.properties_.emplace_back(name, prop);
		return prop;
	}

	ValueBase* addProperty(std::string const& name, std::unique_ptr<ValueBase>&& property, int index_before) override {
		T& asT = static_cast<T&>(*this);
		assert(asT.get(name) == nullptr);
		auto prop = dynamicProperties_.addProperty(name, std::move(property), index_before);
		asT.properties_.emplace_back(name, prop);
		return prop;
	}

	void removeProperty(std::string const& propertyName) override {
		T& asT = static_cast<T&>(*this);
		assert(asT.get(propertyName) != nullptr);
		dynamicProperties_.removeProperty(propertyName);
		auto it = std::find_if(asT.properties_.begin(), asT.properties_.end(),
			[&propertyName](auto const& item) {
				return item.first == propertyName;
			});
		if (it != asT.properties_.end()) {
			asT.properties_.erase(it);
		}
	}

	void removeAllProperties() override {
		T& asT = static_cast<T&>(*this);
		dynamicProperties_.clear();
		asT.properties_.clear();
	}

	std::unique_ptr<ValueBase> extractProperty(std::string const& propertyName) override {
		T& asT = static_cast<T&>(*this);
		auto clonedProp = asT.get(propertyName)->clone({});
		removeProperty(propertyName);
		return clonedProp;
	}

protected:
	raco::data_storage::Table dynamicProperties_;
};

class DynamicEditorObject : public raco::core::EditorObject, public DynamicPropertyMixin<DynamicEditorObject> {
public:
	DynamicEditorObject(std::string name = std::string(), std::string id = std::string())
		: EditorObject(name, id) {
	}

	friend class DynamicPropertyMixin<DynamicEditorObject>;
};

using SDynamicEditorObject = std::shared_ptr<DynamicEditorObject>;

class DynamicGenericStruct : public raco::data_storage::StructBase, public DynamicPropertyMixin<DynamicGenericStruct> {
public:
	DynamicGenericStruct() : StructBase() {}

	friend class DynamicPropertyMixin<DynamicGenericStruct>;
};

}  // namespace raco::serialization::proxy
