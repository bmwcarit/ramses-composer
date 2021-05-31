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

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace raco::data_storage {

class ValueBase;
class AnnotationBase;

// Generic reflection interface
// - property access interface
// - no dynamnic property support here -> use Table
// - provides object annotations
// - used by serialization and property browser
//
// property interface
// - ordered list of string, Value* pairs
// - ordered to support array behaviour of Tables
// - strings can be empty to support arrays; function as property names
// - bare pointers here: need to be able to return Values which are C++ class members
class ReflectionInterface {
public:

	struct TypeDescriptor {
		std::string typeName;
		bool isResource;
	};
	virtual TypeDescriptor const& getTypeDescription() const = 0;

	// Serialization
	virtual bool serializationRequired() const = 0;
	virtual const std::string& serializationTypeName() const {
		return getTypeDescription().typeName;
	}

	// Find property by name; returns nullptr if not found
	virtual ValueBase* get(std::string const& propertyName) = 0;

	// Find property by index; return nullptr if index out of bounds
	virtual ValueBase* get(size_t index) = 0;

	virtual const ValueBase* get(std::string const& propertyName) const = 0;
	virtual const ValueBase* get(size_t index) const = 0;

	
	ValueBase* operator[](std::string const& propertyName);
	ValueBase* operator[](size_t index);

	virtual size_t size() const = 0;

	// Find index from property name; return -1 if not found
	virtual int index(std::string const& propertyName) const = 0;

	// Find name from index; asserts when index out of bounds
	virtual std::string name(size_t index) const = 0;

	bool hasProperty(std::string const& propertyName) const;
	
	// Compare the value but not the annotation data
	bool operator==(const ReflectionInterface& other) const;
};

class ClassWithReflectedMembers : public ReflectionInterface {
public:
	ClassWithReflectedMembers(const ClassWithReflectedMembers &) = delete;
	ClassWithReflectedMembers(ClassWithReflectedMembers &&) = delete;

	ClassWithReflectedMembers& operator=(const ClassWithReflectedMembers&) = delete;
	ClassWithReflectedMembers& operator=(ClassWithReflectedMembers&&) = delete;
	
	ClassWithReflectedMembers(std::vector<std::pair<std::string, ValueBase*>>&& properties = {}) : properties_(std::move(properties)) {}

	virtual ValueBase* get(std::string const& propertyName) override;
	virtual ValueBase* get(size_t index) override;

	virtual const ValueBase* get(std::string const& propertyName) const override;
	virtual const ValueBase* get(size_t index) const override;
		
	virtual size_t size() const override;

	virtual int index(std::string const& propertyName) const override;
	virtual std::string name(size_t index) const override;

	
	template <class Anno>
	std::shared_ptr<Anno> query() const {
		for (auto anno : annotations_) {
			if (auto typedAnno = std::dynamic_pointer_cast<Anno>(anno)) {
				return typedAnno;
			}
		}
		return nullptr;
	}

	std::shared_ptr<AnnotationBase> query(const std::string& typeName);
	std::shared_ptr<const AnnotationBase> query(const std::string& typeName) const;

	void addAnnotation(std::shared_ptr<AnnotationBase> annotation);

	template <class Anno>
	void removeAnnotation() {
		removeAnnotation(query<Anno>());
	}

	void removeAnnotation(std::shared_ptr<AnnotationBase> annotation);

	const std::vector<std::shared_ptr<AnnotationBase>>& annotations() const;

protected:
	std::vector<std::pair<std::string, ValueBase*>> properties_;

	std::vector<std::shared_ptr<AnnotationBase>> annotations_;
};

}