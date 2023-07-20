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
#include "core/BasicTypes.h"
#include "core/PropertyDescriptor.h"

#include <memory>
#include <string>
#include <vector>

namespace raco::core {

using namespace raco::data_storage;

template <typename AnnoType>
class AnnotationHandle;

template <typename AnnoType>
class AnnotationValueHandle;

class BaseContext;
class CodeControlledPropertyModifier;
class ValueHandle;
class EditorObject;
class ValueTreeIterator;

// A ValueHandle is like a pointer to a property inside an EditorObject
// - properties are Value<T> or Property<T> types and are usually accessed through the ValueBase interface
// - they are necessary since ValueBase objects don't contain information on the parent EditorObject or Table
//   that contains the ValueBase object
// - they can refer to any (possible nested) property inside an EditorObject
// - for convenience it is possible to construct ValueHandles refering to objects instead of properties;
//   - only the substructure-related functions should be used in this case
//   - useful for generic iteration over all properties inside an EditorObject
// - invalidation: 
//   ValueHandles are pointer-like objects subject to pointer invalidation: the property referred to may disappear,
//   invalidating the ValueHandle. The validity can be checked using the "operator bool()".
//   Deletion of an object from the Project can not be detected using the ValueHandle alone since it is possible
//   that the shared pointer of the root object is kept alive elsewhere.
class ValueHandle {
public:
	ValueHandle(std::shared_ptr<EditorObject> object = nullptr, std::initializer_list<std::string> names = std::initializer_list<std::string>());
	ValueHandle(const std::shared_ptr<EditorObject>& object, const std::vector<std::string>& names);

	ValueHandle(std::shared_ptr<EditorObject> object, std::initializer_list<size_t> indices);
	ValueHandle(const std::shared_ptr<EditorObject>& object, const std::vector<size_t>& indices) : object_(object), indices_(indices) {
	}

	// Construct the ValueHandle using property fields and avoid using the property name. Example:
	// SMeshNode meshNode = ...;
	// ValueHandle vh { meshNode, &user_types::MeshNode::mesh_ };
	template <class ReflectedClass, class BaseClassWithProperty, class PropertyClassType1>
	ValueHandle(const std::shared_ptr<ReflectedClass>& object, PropertyClassType1 BaseClassWithProperty::*propertyPtr1) : object_(object) {
		static_assert(std::is_member_object_pointer<decltype(propertyPtr1)>::value);
		const int index = object->index(propertyPtr1);
		if (index == -1) {
 			object_ = nullptr;
			return;		
		}
		indices_ = {static_cast<std::size_t>(index)};
	}
	// Construct the ValueHandle using property fields and avoid using the property name. Example:
	// SNode node = ...;
	// ValueHandle vh { node, &user_types::Node::translation_, &Vec3f::y_ };
	template <class ReflectedClass, class BaseClassWithProperty1, class PropertyClassType1,
		class ClassWithProperty2, class PropertyClassType2>
	ValueHandle(const std::shared_ptr<ReflectedClass>& object, PropertyClassType1 BaseClassWithProperty1::*propertyPtr1, PropertyClassType2 ClassWithProperty2::*propertyPtr2) : object_(object) {
		static_assert(std::is_member_object_pointer<decltype(propertyPtr1)>::value);
		static_assert(std::is_member_object_pointer<decltype(propertyPtr2)>::value);
		const int index1 = object->index(propertyPtr1);
		if (index1 == -1) {
			object_ = nullptr;
			return;
		}
		auto subObject = *(dynamic_cast<BaseClassWithProperty1*>(object.get())->*propertyPtr1);
		const int index2 = subObject.index(propertyPtr2);
		if (index2 == -1) {
			object_ = nullptr;
			return;
		}
		indices_ = {static_cast<std::size_t>(index1), static_cast<std::size_t>(index2)};
	}

	// Make constructor from PropertyDescriptor explicit to avoid implicit conversions resulting in 
	// invalid ValueHandles if the property doesn't exist.
	explicit ValueHandle(const PropertyDescriptor& property) : ValueHandle(property.object(), property.propertyNames()) {
	}

	static ValueHandle translatedHandle(const ValueHandle& handle, SEditorObject newObject);
	static ValueHandle translatedHandle(const ValueHandle& handle, std::function<SEditorObject(SEditorObject)>  translateRef);

	PropertyDescriptor getDescriptor() const;

	// Check validity of handle
	operator bool() const;

	// Check if this is a top-level object.
	bool isObject() const;

	// Check if this is a property = not a top-level object
	bool isProperty() const;

	// Check if this is a class type, i.e. either a top-level object or a property with PrimitiveType
	// for which hasTypeSubstructure returns true.
	bool hasSubstructure() const;

	// Check if 'other' is nested inside this ValueHandle.
	bool contains(const ValueHandle &other) const;
	
	SEditorObject rootObject() const;

	// Extract scalar values directly

	// Templated accessor; only works for scalar types: bool, int, int64_t, double, std::string
	template<typename T> T as() const;

	bool asBool() const;
	int asInt() const;
	int64_t asInt64() const;
	double asDouble() const;
	std::string asString() const;
	SEditorObject asRef() const;
	const Vec2f& asVec2f() const;
	const Vec3f& asVec3f() const;
	const Vec4f& asVec4f() const;
	const Vec2i& asVec2i() const;
	const Vec3i& asVec3i() const;
	const Vec4i& asVec4i() const;
 
	bool isVec2f() const;
	bool isVec3f() const;
	bool isVec4f() const;
	bool isVec2i() const;
	bool isVec3i() const;
	bool isVec4i() const;

	template<class C>
	std::shared_ptr<C> asTypedRef() const {
		if (indices_.empty()) {
			return std::dynamic_pointer_cast<C>(object_);
		}
		ValueBase* v = valueRef();
		if (v) {
			return std::dynamic_pointer_cast<C>(v->asRef());
		}
		return nullptr;
	}
	
	PrimitiveType type() const;

	// Number of nested Values for class types including top-level EditorObject ValueHandles.
	// Zero for scalar types.
	size_t size() const;

	// Access Table by index as array
	ValueHandle operator[](size_t) const;

	bool hasProperty(std::string name) const;

	// For class type and Table properties access their members directly
	ValueHandle get(std::string propertyName) const;

	template <class Anno>
	AnnotationHandle<Anno> query() const
	{
		ValueBase* v = valueRef();
		Anno* anno = v->query<Anno>();
		return AnnotationHandle<Anno>(*this, anno);
	}

	// return last element of property names vector
	std::string getPropName() const;

	std::vector<std::string> getPropertyNamesVector() const;

	// Return '/'-separated property path starting at the root object
	std::string getPropertyPath(bool useObjectID = false) const;

	ValueHandle parent() const;

	// Nesting level of property.
	size_t depth() const;

	bool operator==(const ValueHandle& right) const;
	bool operator<(const ValueHandle& right) const;

	ValueHandle& nextSibling();

	const ValueBase* constValueRef() const;

	// Check if a ValueHandle is referring to a specific property if the C++ property member is known. Example:
	// ValueHandle vh = ....;
	// if(vh.isReferencing(&user_types::Node::translation_)) { ... }
	template <typename ReflectedClass, class PropertyClassType1>
	bool isRefToProp(PropertyClassType1 ReflectedClass::*propertyPtr) const {
		const SEditorObject r = rootObject();
		static_assert(std::is_base_of<EditorObject, ReflectedClass>::value);
		const ReflectedClass* userClassPtr = dynamic_cast<ReflectedClass*>(r.get());
		if (userClassPtr == nullptr) {
			return false;
		}
		if (indices_.size() != 1) {
			return false;
		}
		return userClassPtr->get(indices_.front()) == &(userClassPtr->*propertyPtr);
	}


private:
	friend class BaseContext;
	friend class CodeControlledPropertyModifier;
	friend class PrefabOperations;

	ValueBase* valueRef() const;
	ReflectionInterface* object() const;

	template <class T>
	bool isStruct() const {
		ValueBase* v = valueRef();
		if (v->type() == PrimitiveType::Struct) {
			return &v->asStruct().getTypeDescription() == &T::typeDescription;
		}
		return false;
	}

	std::shared_ptr<EditorObject> object_;
	std::vector<size_t> indices_;
};


using ValueHandles = std::vector<ValueHandle>;
/**
 * Creates a vector of [ValueHandle]'s for the given [EditorObject] using the given names.
 */
inline ValueHandles shallowHandles(std::shared_ptr<EditorObject> object = nullptr, std::initializer_list<std::string> names = std::initializer_list<std::string>()) {
	ValueHandles handles {};
	handles.reserve(names.size());
	for (auto& name: names) {
		handles.push_back({ object, { name } });
	}
	return handles;
}

// should behave like smart pointer to annotation itself
template <typename AnnoType>
class AnnotationHandle {
public:
	AnnotationHandle(ValueHandle value, AnnoType* anno)
		: value_(value)
		, annotation_(anno)
	{
	}

	// Check validity of handle
	operator bool() const {
		return annotation_ != nullptr && static_cast<bool>(value_);
	}

	AnnoType const* operator->()
	{
		return annotation_;
	}

	AnnotationValueHandle<AnnoType> get(std::string const& name)
	{
		return { *this, name };
	}

private:
	friend class AnnotationValueHandle<AnnoType>;

	ValueHandle value_;
	AnnoType* annotation_;
};

// Handle for value inside annotation
// - same value access as in ValueHandle
// - no annotation querying (different from ValueHandle)
//   values in annotations don't have annotations
template <typename AnnoType>
class AnnotationValueHandle {
public:
	AnnotationValueHandle(const AnnotationHandle<AnnoType> &annotation, 
	std::string valueName)
		: annotation_(annotation)
		, valueName_(valueName)
	{
	}

	// Check validity of handle
	operator bool() const {
		return static_cast<bool>(annotation_) && annotation_.annotation_->index(valueName_) != -1;
	}

	double asDouble() const
	{
		ValueBase* v = annotation_.annotation_->get(valueName_);
		return v->asDouble();
	}

private:
	AnnotationHandle<AnnoType> annotation_;
	std::string valueName_;
};

}