/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "data_storage/Value.h"

#include "data_storage/BasicTypes.h"
#include "data_storage/Table.h"

#include <string>
#include <map>

namespace raco::data_storage {

static std::map<PrimitiveType, std::string>& primitiveTypeName()  {
	static std::map<PrimitiveType, std::string> primitiveTypeNameMap {
		{PrimitiveType::Bool, "Bool"},
		{PrimitiveType::Int, "Int"},
		{PrimitiveType::Double, "Double"},
		{PrimitiveType::String, "String"},

		{PrimitiveType::Ref, "Ref"},
		{PrimitiveType::Table, "Table"},

		{PrimitiveType::Vec2f, "Vec2f"},
		{PrimitiveType::Vec3f, "Vec3f"},
		{PrimitiveType::Vec4f, "Vec4f"},
		{PrimitiveType::Vec2i, "Vec2i"},
		{PrimitiveType::Vec3i, "Vec3i"},
		{PrimitiveType::Vec4i, "Vec4i"}
	};
	return primitiveTypeNameMap;
};

std::string getTypeName(PrimitiveType type) {
	return primitiveTypeName()[type];
}

bool isPrimitiveTypeName(const std::string& type) {
	for (const auto& [ key, value] :  primitiveTypeName()) {
		if (type == value) return true;
	}
	return false;
}

PrimitiveType toPrimitiveType(const std::string& type) {
	for (const auto& [ key, value] :  primitiveTypeName()) {
		if (type == value) return key;
	}
	return PrimitiveType::Bool;
}

std::unique_ptr<ValueBase> ValueBase::create(PrimitiveType type)
{
	// The code below forces instantiation of the Value classes with the allowed templated arguments;
	// Other types as template arguments will lead to linker errors with missing functions.

	switch (type) {
	case PrimitiveType::Bool:
		return std::unique_ptr<ValueBase>(new Value<bool>());
		break;
	case PrimitiveType::Int:
		return std::unique_ptr<ValueBase>(new Value<int>());
		break;
	case PrimitiveType::Double:
		return std::unique_ptr<ValueBase>(new Value<double>());
		break;
	case PrimitiveType::String:
		return std::unique_ptr<ValueBase>(new Value<std::string>());
		break;

	case PrimitiveType::Ref:
		return std::unique_ptr<ValueBase>(new Value<SEditorObject>());
		break;

	case PrimitiveType::Table:
		return std::unique_ptr<ValueBase>(new Value<Table>());
		break;

	case PrimitiveType::Vec2f:
		return std::unique_ptr<ValueBase>(new Value<Vec2f>());
		break;
	case PrimitiveType::Vec3f:
		return std::unique_ptr<ValueBase>(new Value<Vec3f>());
		break;
	case PrimitiveType::Vec4f:
		return std::unique_ptr<ValueBase>(new Value<Vec4f>());
		break;

	case PrimitiveType::Vec2i:
		return std::unique_ptr<ValueBase>(new Value<Vec2i>());
		break;
	case PrimitiveType::Vec3i:
		return std::unique_ptr<ValueBase>(new Value<Vec3i>());
		break;
	case PrimitiveType::Vec4i:
		return std::unique_ptr<ValueBase>(new Value<Vec4i>());
		break;
	}
	return std::unique_ptr<ValueBase>();
}


template<>
bool& ValueBase::as<bool>() {
	return asBool();
}

template<>
int& ValueBase::as<int>() {
	return asInt();
}

template<>
double& ValueBase::as<double>() {
	return asDouble();
}

template<>
std::string& ValueBase::as<std::string>() {
	return asString();
}

template<>
Table& ValueBase::as<Table>() {
	return asTable();
}

template<>
Vec2f& ValueBase::as<Vec2f>() {
	return asVec2f();
}
template<>
Vec3f& ValueBase::as<Vec3f>() {
	return asVec3f();
}
template<>
Vec4f& ValueBase::as<Vec4f>() {
	return asVec4f();
}

template<>
Vec2i& ValueBase::as<Vec2i>() {
	return asVec2i();
}
template<>
Vec3i& ValueBase::as<Vec3i>() {
	return asVec3i();
}
template<>
Vec4i& ValueBase::as<Vec4i>() {
	return asVec4i();
}

template <>
const bool& ValueBase::as<bool>() const {
	return asBool();
}

template <>
const int& ValueBase::as<int>() const {
	return asInt();
}

template <>
const double& ValueBase::as<double>() const {
	return asDouble();
}

template <>
const std::string& ValueBase::as<std::string>() const {
	return asString();
}

template <>
const Table& ValueBase::as<Table>() const {
	return asTable();
}

template <>
const Vec2f& ValueBase::as<Vec2f>() const {
	return asVec2f();
}
template <>
const Vec3f& ValueBase::as<Vec3f>() const {
	return asVec3f();
}
template <>
const Vec4f& ValueBase::as<Vec4f>() const {
	return asVec4f();
}

template <>
const Vec2i& ValueBase::as<Vec2i>() const {
	return asVec2i();
}
template <>
const Vec3i& ValueBase::as<Vec3i>() const {
	return asVec3i();
}
template <>
const Vec4i& ValueBase::as<Vec4i>() const {
	return asVec4i();
}

bool ValueBase::classesEqual(const ValueBase& left, const ValueBase& right) {
	return typeid(left) == typeid(right);
}

ValueBase& ValueBase::operator=(bool value) {
	asBool() = value;
	return *this;
}

ValueBase& ValueBase::operator=(int value) {
	asInt() = value;
	return *this;
}

ValueBase& ValueBase::operator=(double value) {
	asDouble() = value;
	return *this;
}

ValueBase& ValueBase::operator=(std::string const & value) {
	asString() = value;
	return *this;
}

ValueBase& ValueBase::operator=(SEditorObject value) {
	set(value);
	return *this;
}

template<typename T>
ValueBase& ValueBase::set(T const& value) {
	as<T>() = value;
	return *this;
}

template ValueBase& ValueBase::set<bool>(bool const& value);
template ValueBase& ValueBase::set<int>(int const& value);
template ValueBase& ValueBase::set<double>(double const& value);
template ValueBase& ValueBase::set<std::string>(std::string const& value);

template<> ValueBase& ValueBase::set<std::vector<std::string>>(std::vector<std::string> const& value) {
	asTable().set(value);
	return *this;
}


std::unique_ptr<ValueBase> ValueBase::from(bool value) {
	return std::make_unique<Value<bool>>(value);
}

std::unique_ptr<ValueBase> ValueBase::from(double value) {
	return std::make_unique<Value<double>>(value);
}

template<typename T>
void primitiveCopyAnnotationData(T& dest, const T& src) {
}

template void primitiveCopyAnnotationData<bool>(bool& dest, const bool& src);
template void primitiveCopyAnnotationData<int>(int& dest, const int& src);
template void primitiveCopyAnnotationData<double>(double& dest, const double& src);
template void primitiveCopyAnnotationData<std::string>(std::string& dest, const std::string& src);
template void primitiveCopyAnnotationData<Table>(Table& dest, const Table& src);

template<> void primitiveCopyAnnotationData<Vec2f>(Vec2f& dest, const Vec2f& src) {
	dest.copyAnnotationData(src);
}

template <>
void primitiveCopyAnnotationData<Vec3f>(Vec3f& dest, const Vec3f& src) {
	dest.copyAnnotationData(src);
}
template <>
void primitiveCopyAnnotationData<Vec4f>(Vec4f& dest, const Vec4f& src) {
	dest.copyAnnotationData(src);
}

template <>
void primitiveCopyAnnotationData<Vec2i>(Vec2i& dest, const Vec2i& src) {
	dest.copyAnnotationData(src);
}

template <>
void primitiveCopyAnnotationData<Vec3i>(Vec3i& dest, const Vec3i& src) {
	dest.copyAnnotationData(src);
}
template <>
void primitiveCopyAnnotationData<Vec4i>(Vec4i& dest, const Vec4i& src) {
	dest.copyAnnotationData(src);
}

template <>
Value<Table>::Value(const Value& other, std::function<SEditorObject(SEditorObject)>* translateRef) : ValueBase(), value_(*other, translateRef) {
}

template <>
Value<int>::Value(const Value& other, std::function<SEditorObject(SEditorObject)>* translateRef) : ValueBase(), value_(*other) {
}

template <>
Value<Vec3f>::Value(const Value& other, std::function<SEditorObject(SEditorObject)>* translateRef) : ValueBase(), value_(*other) {
}

template <>
Value<double>::Value(const Value& other, std::function<SEditorObject(SEditorObject)>* translateRef) : ValueBase(), value_(*other) {
}

template <>
Value<std::string>::Value(const Value& other, std::function<SEditorObject(SEditorObject)>* translateRef) : ValueBase(), value_(*other) {
}

template <>
Value<bool>::Value(const Value& other, std::function<SEditorObject(SEditorObject)>* translateRef) : ValueBase(), value_(*other) {
}

template <>
Value<Vec3i>::Value(const Value& other, std::function<SEditorObject(SEditorObject)>* translateRef) : ValueBase(), value_(*other) {
}

template <>
Value<Vec2f>::Value(const Value& other, std::function<SEditorObject(SEditorObject)>* translateRef) : ValueBase(), value_(*other) {
}

template <>
Value<Vec4i>::Value(const Value& other, std::function<SEditorObject(SEditorObject)>* translateRef) : ValueBase(), value_(*other) {
}

template <>
Value<Vec4f>::Value(const Value& other, std::function<SEditorObject(SEditorObject)>* translateRef) : ValueBase(), value_(*other) {
}

template <>
Value<Vec2i>::Value(const Value& other, std::function<SEditorObject(SEditorObject)>* translateRef) : ValueBase(), value_(*other) {
}
}