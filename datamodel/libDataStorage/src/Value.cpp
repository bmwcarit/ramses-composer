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

#include "data_storage/Table.h"

#include <map>
#include <string>

namespace raco::data_storage {

static std::map<PrimitiveType, std::string>& primitiveTypeName() {
	static std::map<PrimitiveType, std::string> primitiveTypeNameMap{
		{PrimitiveType::Bool, "Bool"},
		{PrimitiveType::Int, "Int"},
		{PrimitiveType::Int64, "Int64"},
		{PrimitiveType::Double, "Double"},
		{PrimitiveType::String, "String"},

		{PrimitiveType::Ref, "Ref"},
		{PrimitiveType::Table, "Table"}};
	return primitiveTypeNameMap;
};

std::string getTypeName(PrimitiveType type) {
	return primitiveTypeName()[type];
}

bool isPrimitiveTypeName(const std::string& type) {
	for (const auto& [key, value] : primitiveTypeName()) {
		if (type == value) return true;
	}
	return false;
}

PrimitiveType toPrimitiveType(const std::string& type) {
	for (const auto& [key, value] : primitiveTypeName()) {
		if (type == value) return key;
	}
	return PrimitiveType::Bool;
}

std::unique_ptr<ValueBase> ValueBase::create(PrimitiveType type) {
	// The code below forces instantiation of the Value classes with the allowed templated arguments;
	// Other types as template arguments will lead to linker errors with missing functions.

	switch (type) {
		case PrimitiveType::Bool:
			return std::unique_ptr<ValueBase>(new Value<bool>());
			break;
		case PrimitiveType::Int:
			return std::unique_ptr<ValueBase>(new Value<int>());
			break;
		case PrimitiveType::Int64:
			return std::unique_ptr<ValueBase>(new Value<int64_t>());
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

		case PrimitiveType::Struct:
			throw std::runtime_error("ValueBase::create can't create generic Value<Struct>");
			break;
	}
	return std::unique_ptr<ValueBase>();
}

template <>
bool& ValueBase::as<bool>() {
	return asBool();
}

template <>
int& ValueBase::as<int>() {
	return asInt();
}

template <>
int64_t& ValueBase::as<int64_t>() {
	return asInt64();
}

template <>
double& ValueBase::as<double>() {
	return asDouble();
}

template <>
std::string& ValueBase::as<std::string>() {
	return asString();
}

template <>
Table& ValueBase::as<Table>() {
	return asTable();
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
const int64_t& ValueBase::as<int64_t>() const {
	return asInt64();
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

ValueBase& ValueBase::operator=(int64_t value) {
	asInt64() = value;
	return *this;
}

ValueBase& ValueBase::operator=(double value) {
	asDouble() = value;
	return *this;
}

ValueBase& ValueBase::operator=(std::string const& value) {
	asString() = value;
	return *this;
}

ValueBase& ValueBase::operator=(SEditorObject value) {
	set(value);
	return *this;
}

template <typename T>
ValueBase& ValueBase::set(T const& value) {
	as<T>() = value;
	return *this;
}

template ValueBase& ValueBase::set<bool>(bool const& value);
template ValueBase& ValueBase::set<int>(int const& value);
template ValueBase& ValueBase::set<int64_t>(int64_t const& value);
template ValueBase& ValueBase::set<double>(double const& value);
template ValueBase& ValueBase::set<std::string>(std::string const& value);

template <>
ValueBase& ValueBase::set<std::vector<std::string>>(std::vector<std::string> const& value) {
	asTable().set(value);
	return *this;
}

template <>
ValueBase& ValueBase::set<Table>(Table const& value) {
	asTable() = value;
	return *this;
}

std::unique_ptr<ValueBase> ValueBase::from(bool value) {
	return std::make_unique<Value<bool>>(value);
}

std::unique_ptr<ValueBase> ValueBase::from(double value) {
	return std::make_unique<Value<double>>(value);
}

template <typename T>
void primitiveCopyAnnotationData(T& dest, const T& src) {
}

template void primitiveCopyAnnotationData<bool>(bool& dest, const bool& src);
template void primitiveCopyAnnotationData<int>(int& dest, const int& src);
template void primitiveCopyAnnotationData<int64_t>(int64_t& dest, const int64_t& src);
template void primitiveCopyAnnotationData<double>(double& dest, const double& src);
template void primitiveCopyAnnotationData<std::string>(std::string& dest, const std::string& src);
template void primitiveCopyAnnotationData<Table>(Table& dest, const Table& src);

}  // namespace raco::data_storage