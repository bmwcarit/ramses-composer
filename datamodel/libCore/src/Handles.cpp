/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "core/Handles.h"
#include "core/EditorObject.h"
#include "core/Iterators.h"
#include "core/PropertyDescriptor.h"

namespace raco::core {

ValueHandle::ValueHandle(std::shared_ptr<EditorObject> object, std::initializer_list<std::string> names) : ValueHandle(object, std::vector<std::string>(names)) {}

ValueHandle::ValueHandle(const std::shared_ptr<EditorObject>& object, const std::vector<std::string>& names)
	: object_(object), indices_(names.size()) {
	indices_.resize(0);
	const ReflectionInterface* o = object_.get();
	for (const auto& name : names) {
		int index = o->index(name);
		if (index == -1) {
			object_ = nullptr;
			indices_.clear();
			break;
		}
		indices_.push_back(index);

		const ValueBase* val = o->get(index);
		if (hasTypeSubstructure(val->type())) {
			o = &val->getSubstructure();
		}
	}
}

ValueHandle::ValueHandle(std::shared_ptr<EditorObject> object, std::initializer_list<size_t> indices)
	: object_(object), indices_(indices) {
}

ValueHandle ValueHandle::translatedHandle(const ValueHandle& handle, SEditorObject newObject) {
	return ValueHandle(handle.object_ ? newObject : nullptr, handle.indices_);
}

ValueHandle ValueHandle::translatedHandle(const ValueHandle& handle, std::function<SEditorObject(SEditorObject)> translateRef) {
	return ValueHandle::translatedHandle(handle, translateRef(handle.rootObject()));
}

PropertyDescriptor ValueHandle::getDescriptor() const {
	return PropertyDescriptor(object_, getPropertyNamesVector());
}


template <>
bool ValueHandle::as<bool>() const {
	return asBool();
}

template <>
int ValueHandle::as<int>() const {
	return asInt();
}

template <>
int64_t ValueHandle::as<int64_t>() const {
	return asInt64();
}

template<>
float ValueHandle::as<float>() const {
	return static_cast<float>(asDouble());
}

template<>
double ValueHandle::as<double>() const {
	return asDouble();
}

template <>
std::string ValueHandle::as<std::string>() const {
	return asString();
}

bool ValueHandle::asBool() const {
	ValueBase* v = valueRef();
	return v->asBool();
}

int ValueHandle::asInt() const {
	ValueBase* v = valueRef();
	return v->asInt();
}

int64_t ValueHandle::asInt64() const {
	ValueBase* v = valueRef();
	return v->asInt64();
}

double ValueHandle::asDouble() const {
	ValueBase* v = valueRef();
	return v->asDouble();
}

std::string ValueHandle::asString() const {
	ValueBase* v = valueRef();
	return v->asString();
}

SEditorObject ValueHandle::asRef() const {
	ValueBase* v = valueRef();
	return v->asRef();
}

const Vec2f& ValueHandle::asVec2f() const {
	ValueBase* v = valueRef();
	return dynamic_cast<const Vec2f&>(v->asStruct());
}

const Vec3f& ValueHandle::asVec3f() const {
	ValueBase* v = valueRef();
	return dynamic_cast<const Vec3f&>(v->asStruct());
}

const Vec4f& ValueHandle::asVec4f() const {
	ValueBase* v = valueRef();
	return dynamic_cast<const Vec4f&>(v->asStruct());
}

const Vec2i& ValueHandle::asVec2i() const {
	ValueBase* v = valueRef();
	return dynamic_cast<const Vec2i&>(v->asStruct());
}

const Vec3i& ValueHandle::asVec3i() const {
	ValueBase* v = valueRef();
	return dynamic_cast<const Vec3i&>(v->asStruct());
}

const Vec4i& ValueHandle::asVec4i() const {
	ValueBase* v = valueRef();
	return dynamic_cast<const Vec4i&>(v->asStruct());
}

bool ValueHandle::isVec2f() const {
	return isStruct<Vec2f>();
}

bool ValueHandle::isVec3f() const {
	return isStruct<Vec3f>();
}

bool ValueHandle::isVec4f() const {
	return isStruct<Vec4f>();
}

bool ValueHandle::isVec2i() const {
	return isStruct<Vec2i>();
}

bool ValueHandle::isVec3i() const {
	return isStruct<Vec3i>();
}

bool ValueHandle::isVec4i() const {
	return isStruct<Vec4i>();
}

size_t ValueHandle::size() const {
	if (indices_.empty()) {
		return object_->size();
	}
	auto v = valueRef();
	if (hasTypeSubstructure(v->type())) {
		return v->getSubstructure().size();
	}
	return 0;
}

PrimitiveType ValueHandle::type() const {
	return valueRef()->type();
}

ValueHandle ValueHandle::operator[](size_t index) const {
	ValueHandle v(object_, indices_);
	v.indices_.push_back(index);
	return v;
}

bool ValueHandle::hasProperty(std::string name) const {
	if (indices_.empty()) {
		return object_->hasProperty(name);
	}
	auto v = valueRef();
	if (hasTypeSubstructure(v->type())) {
		return v->getSubstructure().hasProperty(name);
	}
	return false;
}

ValueHandle ValueHandle::get(std::string propertyName) const {
	ValueHandle v(object_, indices_);
	size_t index = object()->index(propertyName);
	v.indices_.emplace_back(index);
	return v;
}

std::string ValueHandle::getPropName() const {
	if (!indices_.empty()) {
		ReflectionInterface* o = object_.get();

		for (int i = 0; i < indices_.size() - 1; i++) {
			auto v = (*o)[indices_[i]];
			o = &v->getSubstructure();
		}

        return o->name(indices_.back());
	}
    throw std::runtime_error("invalid property");
}

std::vector<std::string> ValueHandle::getPropertyNamesVector() const {
	if (!indices_.empty()) {
		std::vector<std::string> result; 
		ReflectionInterface* o = object_.get();
		for (int i = 0; i < indices_.size() - 1; i++) {
			result.emplace_back(o->name(indices_[i]));
			auto v = (*o)[indices_[i]];
			o = &v->getSubstructure();
		}
		result.emplace_back(o->name(indices_.back()));
		return result;
	}
	throw std::runtime_error("invalid property");
}

std::string ValueHandle::getPropertyPath(bool useObjectID) const {
	if (!indices_.empty()) {
		ReflectionInterface* o = object_.get();
		std::string propPath;
		if (useObjectID) {
			propPath = object_->objectID();
		} else {
			propPath = object_->objectName();
		}
		for (int i = 0; i < indices_.size() - 1; i++) {
			propPath += "." + o->name(indices_[i]);
			auto v = (*o)[indices_[i]];
			o = &v->getSubstructure();
		}
		propPath += "." + o->name(indices_.back());
		return propPath;
	}
	throw std::runtime_error("invalid property");
}


ValueHandle ValueHandle::parent() const {
	if (indices_.empty()) {
		return ValueHandle(nullptr);
	}
	ValueHandle v(object_, std::vector<size_t>(indices_.begin(), indices_.begin() + indices_.size() - 1));
	return v;
}

ValueHandle::operator bool() const {
	if (indices_.empty()) {
		return object_ != nullptr;
	}

	return valueRef() != nullptr;
}

bool ValueHandle::isObject() const {
	return indices_.empty();
}

bool ValueHandle::isProperty() const {
	return !indices_.empty();
}

bool ValueHandle::hasSubstructure() const {
	return isObject() || hasTypeSubstructure(valueRef()->type());
}

bool ValueHandle::contains(const ValueHandle& other) const {
	return object_ == other.object_ && indices_.size() < other.indices_.size() &&
		   std::equal(indices_.begin(), indices_.end(), other.indices_.begin(), other.indices_.begin() + indices_.size());
}

SEditorObject ValueHandle::rootObject() const {
	return object_;
}

size_t ValueHandle::depth() const {
	return indices_.size();
}

const ValueBase* ValueHandle::constValueRef() const {
	return valueRef();
}

ValueBase* ValueHandle::valueRef() const {
	if (!indices_.empty()) {
		ReflectionInterface* o = object_.get();
		ValueBase* v = nullptr;

		for (auto index : indices_) {
			if (v) {
				if (!hasTypeSubstructure(v->type())) {
					return nullptr;
				}
				o = &v->getSubstructure();
			}
			v = (*o)[index];
			if (!v) {
				return nullptr;
			}
		}
		return v;
	}
	return nullptr;
}

ReflectionInterface* ValueHandle::object() const {
	if (indices_.empty()) {
		return object_.get();
	} else {
		auto v = valueRef();
		return &v->getSubstructure();
	}

	return nullptr;
}

bool ValueHandle::operator==(const ValueHandle& right) const {
	return object_ == right.object_ && indices_ == right.indices_;
}

bool ValueHandle::operator<(const ValueHandle& right) const {
	return object_.get() < right.object_.get() || (object_.get() == right.object_.get() && indices_ < right.indices_);
}

ValueHandle& ValueHandle::nextSibling() {
	++indices_.back();
	return *this;
}

}  // namespace raco::core
