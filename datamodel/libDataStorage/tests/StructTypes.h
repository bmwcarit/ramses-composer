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

#include "data_storage/BasicAnnotations.h"
#include "data_storage/BasicTypes.h"
#include "data_storage/Table.h"
#include "data_storage/Value.h"

using namespace raco::data_storage;

class SimpleStruct : public ClassWithReflectedMembers {
public:
	static inline const TypeDescriptor typeDescription = { "SimpleStruct", false };
	TypeDescriptor const& getTypeDescription() const override {
		return typeDescription;
	}
	bool serializationRequired() const override {
		return true;
	}

	SimpleStruct() : ClassWithReflectedMembers(getProperties()) {}

	SimpleStruct(const SimpleStruct& other, std::function<SEditorObject(SEditorObject)>* translateRef = nullptr) : ClassWithReflectedMembers(getProperties()), bb(other.bb), dd(other.dd) {}

	SimpleStruct& operator=(const SimpleStruct& other) {
		bb = other.bb;
		dd = other.dd;
		return *this;
	}
	void copyAnnotationData(const SimpleStruct& other) {
		bb.copyAnnotationData(other.bb);
		dd.copyAnnotationData(other.dd);
	}
	std::vector<std::pair<std::string, ValueBase*>> getProperties() {
		return {
			{"bool", &bb},
			{"double", &dd} };
	}

	Value<bool> bb{true};
	Value<double> dd{1.5};
};

class AltStruct : public ClassWithReflectedMembers {
public:
	static inline const TypeDescriptor typeDescription = { "AltStruct", false };
	TypeDescriptor const& getTypeDescription() const override {
		return typeDescription;
	}
	bool serializationRequired() const override {
		return true;
	}

	AltStruct() : ClassWithReflectedMembers(getProperties()) {}

	AltStruct(const AltStruct& other, std::function<SEditorObject(SEditorObject)>* translateRef = nullptr) : ClassWithReflectedMembers(getProperties()), bb(other.bb), dd(other.dd) {}

	AltStruct& operator=(const AltStruct& other) {
		bb = other.bb;
		dd = other.dd;
		return *this;
	}

	void copyAnnotationData(const AltStruct& other) {
		bb.copyAnnotationData(other.bb);
		dd.copyAnnotationData(other.dd);
	}

	std::vector<std::pair<std::string, ValueBase*>> getProperties() {
		return {
			{"bool", &bb},
			{"double", &dd} };
	}

	Value<bool> bb;
	Value<double> dd;
};

#pragma once
