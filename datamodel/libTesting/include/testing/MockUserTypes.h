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

#include "core/EditorObject.h"
#include "core/BasicTypes.h"
#include "core/Link.h"

#include <vector>

namespace raco::user_types {
using namespace raco::core;

class Dummy : public AnnotationBase {
public:
	static inline const TypeDescriptor typeDescription = {"Dummy", false};
	TypeDescriptor const& getTypeDescription() const override {
		return typeDescription;
	}
	bool serializationRequired() const override {
		return false;
	}

	Dummy() : AnnotationBase({}) {}

	Dummy(const Dummy& other) : AnnotationBase({}) {}

	Dummy& operator=(const Dummy& other) {
		return *this;
	}
};

class MockTableObject : public raco::core::EditorObject {
public:
	static inline const TypeDescriptor typeDescription = {"MockTableObject", true};
	TypeDescriptor const& getTypeDescription() const override {
		return typeDescription;
	}
	MockTableObject(MockTableObject const&) = delete;
	MockTableObject(std::string name = std::string(), std::string id = std::string()) : EditorObject(name, id) {
		fillPropertyDescription();
	}

	void fillPropertyDescription() {
		properties_.emplace_back("table", &table_);
	}

	Property<Table, Dummy> table_{{}, {}};
};

class MockLuaScript : public EditorObject {
public:
	static inline const TypeDescriptor typeDescription = {"MockLuaScript", true};
	TypeDescriptor const& getTypeDescription() const override {
		return typeDescription;
	}

	MockLuaScript(MockLuaScript const&) = delete;

	MockLuaScript(std::string name = std::string(), std::string id = std::string()) : EditorObject(name, id) {
		fillPropertyDescription();

		inputs_->addProperty("in_double", PrimitiveType::Double);

		auto s = inputs_->addProperty("in_struct", PrimitiveType::Table);
		s->asTable().addProperty("foo", PrimitiveType::Double);
		s->asTable().addProperty("bar", std::make_unique<Value<Vec3f>>());

		auto a = inputs_->addProperty("in_array_double", PrimitiveType::Table);
		a->asTable().addProperty(std::string(), PrimitiveType::Double);
		a->asTable().addProperty(std::string(), PrimitiveType::Double);
		a->asTable().addProperty(std::string(), PrimitiveType::Double);

		auto av = inputs_->addProperty("in_array_vec3f", PrimitiveType::Table);
		av->asTable().addProperty(std::string(), new Value<Vec3f>());
		av->asTable().addProperty(std::string(), new Value<Vec3f>());
		av->asTable().addProperty(std::string(), new Value<Vec3f>());

		auto as = inputs_->addProperty("in_array_struct", PrimitiveType::Table);
		auto as0 = as->asTable().addProperty(std::string(), PrimitiveType::Table);
		as0->asTable().addProperty("foo", PrimitiveType::Double);
		as0->asTable().addProperty("bar", new Value<Vec3f>());
		auto as1 = as->asTable().addProperty(std::string(), PrimitiveType::Table);
		as1->asTable().addProperty("foo", PrimitiveType::Double);
		as1->asTable().addProperty("bar", new Value<Vec3f>());
	}

	void fillPropertyDescription() {
		properties_.emplace_back("inputs", &inputs_);
		properties_.emplace_back("outputs", &outputs_);
	}

	Property<Table, Dummy> inputs_{{}, {}};
	Property<Table, Dummy> outputs_{{}, {}};
};

class StructWithRef : public StructBase {
public:
	static inline const TypeDescriptor typeDescription = {"StructWithRef", false};
	TypeDescriptor const& getTypeDescription() const override {
		return typeDescription;
	}
	bool serializationRequired() const override {
		return true;
	}

	StructWithRef() : StructBase(getProperties()) {}

	StructWithRef(const StructWithRef& other, std::function<SEditorObject(SEditorObject)>* translateRef = nullptr) : StructBase(getProperties()) {
		if (translateRef) {
			ref = (*translateRef)(*other.ref);
		} else {
			ref = other.ref;
		}
	}

	StructWithRef& operator=(const StructWithRef& other) {
		ref = other.ref;
		return *this;
	}
	void copyAnnotationData(const StructWithRef& other) {
		ref.copyAnnotationData(other.ref);
	}
	std::vector<std::pair<std::string, ValueBase*>> getProperties() {
		return {
			{"ref", &ref}};
	}

	Value<SEditorObject> ref;
};

class Foo : public EditorObject {
public:
	static inline const TypeDescriptor typeDescription = {"Foo", false};
	TypeDescriptor const& getTypeDescription() const override {
		return typeDescription;
	}

	Foo(Foo const& other) : EditorObject(other), b_(other.b_), i_(other.i_), i64_(other.i64_), x_(other.x_), s_(other.s_), ref_(other.ref_), vec_(other.vec_), v2f_(other.v2f_), v3f_(other.v3f_), v4f_(other.v4f_), v2i_(other.v2i_), v3i_(other.v3i_), v4i_(other.v4i_), future_(other.future_), readOnly_(other.readOnly_) {
		fillPropertyDescription();
	}

	Foo(std::string name = {}, std::string id = {}) : EditorObject(name, id) {
		fillPropertyDescription();
	}

	void fillPropertyDescription() {
		properties_.emplace_back("flag", &b_);
		properties_.emplace_back("i", &i_);
		properties_.emplace_back("i64", &i64_);
		properties_.emplace_back("x", &x_);
		properties_.emplace_back("s", &s_);
		properties_.emplace_back("ref", &ref_);
		properties_.emplace_back("vec", &vec_);
		properties_.emplace_back("v2f", &v2f_);
		properties_.emplace_back("v3f", &v3f_);
		properties_.emplace_back("v4f", &v4f_);
		properties_.emplace_back("v2i", &v2i_);
		properties_.emplace_back("v3i", &v3i_);
		properties_.emplace_back("v4i", &v4i_);
		properties_.emplace_back("future", &future_);
		properties_.emplace_back("futureLinkable", &futureLinkable_);
		properties_.emplace_back("readOnly", &readOnly_);
	}

	Value<bool> b_{false};
	Value<int> i_{3};
	Value<int64_t> i64_{0};
	Property<double, LinkStartAnnotation, LinkEndAnnotation> x_{2.5, {}, {}};
	Value<std::string> s_{"cat"};
	Value<SEditorObject> ref_;

	Value<Vec3f> vec_{
		{2.0}};

	Value<Vec2f> v2f_{};
	Value<Vec3f> v3f_{};
	Value<Vec4f> v4f_{};

	Value<Vec2i> v2i_{};
	Value<Vec3i> v3i_{};
	Value<Vec4i> v4i_{};

	Property<double, LinkEndAnnotation> futureLinkable_{2, {0x12345678}};
	Property<double, FeatureLevel> future_{1, {0x12345678}};

	Property<int, ReadOnlyAnnotation> readOnly_{42, {}};

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
	static inline const TypeDescriptor typeDescription = {"ObjectWithTableProperty", false};
	TypeDescriptor const& getTypeDescription() const override {
		return typeDescription;
	}

	ObjectWithTableProperty(ObjectWithTableProperty const& other)
		: EditorObject(other),
		  t_(other.t_),
		  array_(other.array_),
		  tags_(other.tags_),
		  renderableTags_(other.renderableTags_) {
		fillPropertyDescription();
	}

	ObjectWithTableProperty(std::string name = {}, std::string id = {}) : EditorObject(name, id) {
		fillPropertyDescription();
	}

	void fillPropertyDescription() {
		properties_.emplace_back("t", &t_);
		properties_.emplace_back("array", &array_);
		properties_.emplace_back("tags", &tags_);
		properties_.emplace_back("renderableTags", &renderableTags_);
	}

	Value<Table> t_{};
	Property<Table, ArraySemanticAnnotation> array_{};
	Property<Table, TagContainerAnnotation> tags_{};
	Property<Table, RenderableTagContainerAnnotation> renderableTags_{};
};

class ObjectWithStructProperty : public EditorObject {
public:
	static inline const TypeDescriptor typeDescription = {"ObjectWithStructProperty", false};
	TypeDescriptor const& getTypeDescription() const override {
		return typeDescription;
	}

	ObjectWithStructProperty(ObjectWithStructProperty const& other) : EditorObject(other), s_(other.s_) {
		fillPropertyDescription();
	}

	ObjectWithStructProperty(std::string name = {}, std::string id = {}) : EditorObject(name, id) {
		fillPropertyDescription();
	}

	ObjectWithStructProperty() : EditorObject() {
		fillPropertyDescription();
	}

	void fillPropertyDescription() {
		properties_.emplace_back("s", &s_);
	}

	Value<StructWithRef> s_{};
};

class FutureType : public EditorObject {
public:
	static inline const TypeDescriptor typeDescription = {"FutureType", false, 0x12345678};
	TypeDescriptor const& getTypeDescription() const override {
		return typeDescription;
	}

	FutureType(FutureType const& other) : EditorObject(other), s_(other.s_) {
		fillPropertyDescription();
	}

	FutureType(std::string name = {}, std::string id = {}) : EditorObject(name, id) {
		fillPropertyDescription();
	}

	FutureType() : EditorObject() {
		fillPropertyDescription();
	}

	void fillPropertyDescription() {
		properties_.emplace_back("s", &s_);
	}

	Value<StructWithRef> s_{};
};

}  // namespace raco::user_types
