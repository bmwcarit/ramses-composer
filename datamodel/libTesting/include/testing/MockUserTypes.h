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

#include "core/EditorObject.h"
#include "data_storage/BasicTypes.h"

#include <vector>

namespace raco::user_types {
using namespace raco::core;

class MockLuaScript : public EditorObject {
public:
	static inline const TypeDescriptor typeDescription = { "MockLuaScript", true};
	TypeDescriptor const& getTypeDescription() const override {
		return typeDescription;
	}

	MockLuaScript(MockLuaScript const&) = delete;

	MockLuaScript(std::string name = std::string(), std::string id = std::string()) : EditorObject(name, id) {
		fillPropertyDescription();

		luaInputs_->addProperty("in_double", PrimitiveType::Double);

		auto s = luaInputs_->addProperty("in_struct", PrimitiveType::Table);
		s->asTable().addProperty("foo", PrimitiveType::Double);
		s->asTable().addProperty("bar", PrimitiveType::Vec3f);

		auto a = luaInputs_->addProperty("in_array_double", PrimitiveType::Table);
		a->asTable().addProperty(std::string(), PrimitiveType::Double);
		a->asTable().addProperty(std::string(), PrimitiveType::Double);
		a->asTable().addProperty(std::string(), PrimitiveType::Double);

		auto av = luaInputs_->addProperty("in_array_vec3f", PrimitiveType::Table);
		av->asTable().addProperty(std::string(), PrimitiveType::Vec3f);
		av->asTable().addProperty(std::string(), PrimitiveType::Vec3f);
		av->asTable().addProperty(std::string(), PrimitiveType::Vec3f);

		auto as = luaInputs_->addProperty("in_array_struct", PrimitiveType::Table);
		auto as0 = as->asTable().addProperty(std::string(), PrimitiveType::Table);
		as0->asTable().addProperty("foo", PrimitiveType::Double);
		as0->asTable().addProperty("bar", PrimitiveType::Vec3f);
		auto as1 = as->asTable().addProperty(std::string(), PrimitiveType::Table);
		as1->asTable().addProperty("foo", PrimitiveType::Double);
		as1->asTable().addProperty("bar", PrimitiveType::Vec3f);
	}

	void fillPropertyDescription() {
		properties_.emplace_back("luaInputs", &luaInputs_);
		properties_.emplace_back("luaOutputs", &luaOutputs_);
	}

	Property<Table> luaInputs_{ {} };
	Property<Table> luaOutputs_{ {} };
};


class Foo : public EditorObject {
public:
	static inline const TypeDescriptor typeDescription = { "Foo", false};
	TypeDescriptor const& getTypeDescription() const override {
		return typeDescription;
	}

	Foo(Foo const& other) : EditorObject(other), b_(other.b_), i_(other.i_), x_(other.x_), s_(other.s_), ref_(other.ref_), vec_(other.vec_) {
		fillPropertyDescription();
	}

	Foo() : EditorObject() {
		fillPropertyDescription();
	}


	
	void fillPropertyDescription() {
		properties_.emplace_back("flag", &b_);
		properties_.emplace_back("i", &i_);
		properties_.emplace_back("x", &x_);
		properties_.emplace_back("s", &s_);
		properties_.emplace_back("ref", &ref_);
		properties_.emplace_back("vec", &vec_);
	}

	Value<bool> b_{ false };
	Value<int> i_{ 3 };
	Value<double> x_{ 2.5 };
	Value<std::string> s_{ "cat" };
	Value<SEditorObject> ref_;

	Value<Vec3f> vec_{
		{ 2.0 }
	};

	void onBeforeRemoveReferenceToThis(ValueHandle const& sourceReferenceProperty) const override {
		onBeforeRemoveReferenceToThis_.push_back(sourceReferenceProperty);
		EditorObject::onBeforeRemoveReferenceToThis(sourceReferenceProperty);
	}

	void onAfterAddReferenceToThis(ValueHandle const& sourceReferenceProperty) const override {
		onAfterAddReferenceToThis_.push_back(sourceReferenceProperty);
		EditorObject::onAfterAddReferenceToThis(sourceReferenceProperty);		
	}

	mutable std::vector<ValueHandle> onBeforeRemoveReferenceToThis_;
	mutable std::vector<ValueHandle> onAfterAddReferenceToThis_;
};

class ObjectWithTableProperty : public EditorObject {
public:
	static inline const TypeDescriptor typeDescription = {"Foo", false};
	TypeDescriptor const& getTypeDescription() const override {
		return typeDescription;
	}

	ObjectWithTableProperty(ObjectWithTableProperty const& other) : EditorObject(other), t_(other.t_) {
		fillPropertyDescription();
	}

	ObjectWithTableProperty() : EditorObject() {
		fillPropertyDescription();
	}

	void fillPropertyDescription() {
		properties_.emplace_back("t", &t_);
	}

	Value<Table> t_{};
};
}