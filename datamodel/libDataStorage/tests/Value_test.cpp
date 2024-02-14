/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "data_storage/Value.h"
#include "data_storage/Array.h"
#include "data_storage/Table.h"

#include "testing/StructTypes.h"

#include "gtest/gtest.h"

using namespace raco::data_storage;

TEST(ValueTest, Scalar) {
	Value<bool> bv;
	Value<int> iv;

	Value<double> dv;
	dv = 2.0;
	EXPECT_EQ(*dv, 2.0);

	EXPECT_EQ(dv.asDouble(), 2.0);
	EXPECT_EQ(dv.as<double>(), 2.0);

	EXPECT_THROW(dv.asBool(), std::runtime_error);
	EXPECT_THROW(dv.as<bool>(), std::runtime_error);

	Value<double> u;
	u = 3.0;
	u = dv;

	Value<std::string> s;
}

TEST(ValueTest, Tables) {
	Value<Table> tv;
	EXPECT_EQ(tv->size(), 0);

	tv->addProperty("intval", PrimitiveType::Int);
	EXPECT_EQ(tv->size(), 1);
	EXPECT_EQ(tv->index("intval"), 0);

	EXPECT_EQ(tv->name(0), "intval");
	EXPECT_EQ((*tv)[0]->asInt(), 0);
	EXPECT_EQ((*tv)["intval"]->asInt(), 0);
	EXPECT_EQ(tv->get("intval")->asInt(), 0);

	tv->addProperty("fval", PrimitiveType::Double);
	EXPECT_EQ(tv->size(), 2);
	EXPECT_EQ(tv->index("fval"), 1);

	EXPECT_EQ(tv->name(1), "fval");
	EXPECT_EQ((*tv)[1]->asDouble(), 0.0);
	EXPECT_EQ((*tv)["fval"]->asDouble(), 0.0);
	EXPECT_EQ(tv->get("fval")->asDouble(), 0.0);
	EXPECT_THROW((*tv)[1]->asInt(), std::runtime_error);

	(*tv)[0]->asInt() = 42;
	EXPECT_EQ(tv->get("intval")->asInt(), 42);

	*tv->get("fval") = 2.0;
	EXPECT_EQ((*tv)[1]->asDouble(), 2.0);

	// Check Table assignment operator
	Value<Table> tu;
	tu = tv;

	EXPECT_EQ(tu->size(), tv->size());
	EXPECT_EQ(tu->get("intval")->asInt(), 42);
	EXPECT_EQ(tv->get("fval")->asDouble(), 2.0);
}

TEST(ValueTest, Table_nested_struct) {
	Value<Table> tv;

	tv->addProperty("struct", std::make_unique<Value<SimpleStruct>>());
	EXPECT_EQ((*tv)["struct"]->getSubstructure().get("bool")->asBool(), true);
	EXPECT_EQ((*tv)["struct"]->getSubstructure().get("double")->asDouble(), 1.5);

	Value<SimpleStruct> s;
	s->bb = false;
	s->dd = 2.0;

	*tv->get("struct") = s;
	EXPECT_EQ((*tv)["struct"]->getSubstructure().get("bool")->asBool(), false);
	EXPECT_EQ((*tv)["struct"]->getSubstructure().get("double")->asDouble(), 2.0);

	Value<Table> tu;
	tu = tv;
	EXPECT_EQ((*tu)["struct"]->getSubstructure().get("bool")->asBool(), false);
	EXPECT_EQ((*tu)["struct"]->getSubstructure().get("double")->asDouble(), 2.0);
}

TEST(ValueTest, clone) {
	Value<int> vint{23};
	auto vint_clone = vint.clone(nullptr);
	EXPECT_EQ(vint_clone->asInt(), *vint);

	Value<Table> vt;
	vt->addProperty("test", PrimitiveType::Double);

	auto vt_clone = vt.clone(nullptr);

	EXPECT_EQ(vt_clone->asTable().size(), vt->size());
	EXPECT_EQ(vt_clone->asTable().name(0), vt->name(0));
}

TEST(ValueTest, Struct) {
	SimpleStruct s;
	s.bb = false;
	s.dd = 2.0;

	Value<SimpleStruct> vs;
	const Value<SimpleStruct> cvs(s);
	Value<AltStruct> va;

	EXPECT_EQ(vs.type(), PrimitiveType::Struct);
	EXPECT_EQ(vs.typeName(), "SimpleStruct");
	EXPECT_EQ(vs.baseTypeName(), "SimpleStruct");

	EXPECT_THROW(vs.asBool(), std::runtime_error);
	EXPECT_THROW(vs.as<bool>(), std::runtime_error);
	EXPECT_THROW(vs.asTable(), std::runtime_error);
	EXPECT_THROW(vs.as<Table>(), std::runtime_error);

	ReflectionInterface& intf = vs.getSubstructure();
	EXPECT_EQ(intf.size(), 2);

	const ReflectionInterface& cintf = cvs.getSubstructure();
	EXPECT_EQ(cintf.size(), 2);

	EXPECT_EQ(vs->size(), 2);

	EXPECT_THROW(vs.asRef(), std::runtime_error);

	// operator=
	vs = cvs;
	EXPECT_EQ(*vs->dd, 2.0);

	Value<AltStruct> v;
	EXPECT_THROW(vs = v, std::runtime_error);

	EXPECT_THROW(vs = va, std::runtime_error);

	vs = s;
	EXPECT_TRUE(*vs == s);

	// operator==
	EXPECT_TRUE(vs == cvs);
	EXPECT_FALSE(vs == va);

	// compare
	auto translateRef = [](SEditorObject obj) { return obj; };
	EXPECT_TRUE(vs.compare(cvs, translateRef));
	EXPECT_FALSE(vs.compare(va, translateRef));

	// assign
	EXPECT_FALSE(vs.assign(cvs));
	EXPECT_THROW(vs.assign(va), std::runtime_error);

	// copyAnnotationData
	EXPECT_NO_THROW(vs.copyAnnotationData(cvs));
	EXPECT_THROW(vs.copyAnnotationData(va), std::runtime_error);

	// clone
	auto vsclone = vs.clone(nullptr);

	EXPECT_TRUE(vs == *vsclone.get());

	// Value::asArray-> throw
	EXPECT_THROW(vs.asArray(), std::runtime_error);
}

TEST(ValueTest, Array) {
	Array<double> ad;
	*ad.addProperty() = -1.0;
	*ad.addProperty() = -2.0;
	Array<int> ai;
	*ai.addProperty() = 13;
	*ai.addProperty() = 17;

	Value<Array<double>> vad;
	Value<Array<int>> vai;
	const Value<Array<double>>& cvad{vad};

	// ValueBase::type
	EXPECT_EQ(vad.type(), PrimitiveType::Array);
	// Array::elementType
	EXPECT_EQ(vad->elementType(), PrimitiveType::Double);

	// Value::baseTypeName
	EXPECT_EQ(vad.baseTypeName(), "Array[Double]");
	// Value::typeName
	EXPECT_EQ(vad.typeName(), "Array[Double]");

	// Array::typeName
	EXPECT_EQ(vad->typeName(), "Array[Double]");
	// Array::elementTypeName
	EXPECT_EQ(vad->elementTypeName(), "Double");

	// Array::addProperty(int index_before)
	EXPECT_THROW(vad->addProperty(-2), std::out_of_range);
	EXPECT_THROW(vad->addProperty(1), std::out_of_range);
	auto prop1 = vad->addProperty();
	auto prop2 = vad->addProperty(-1);
	auto prop3 = vad->addProperty(1);
	EXPECT_EQ(vad->size(), 3);

	// Array::get(index)
	EXPECT_EQ(vad->get(0), prop1);
	EXPECT_EQ(vad->get(1), prop3);
	EXPECT_EQ(vad->get(2), prop2);


	// Array::get(index) const
	EXPECT_EQ(cvad->get(0), prop1);
	EXPECT_EQ(cvad->get(1), prop3);
	EXPECT_EQ(cvad->get(2), prop2);


	// Array:get(string)
	EXPECT_THROW(vad->get("0"), std::out_of_range);
	EXPECT_THROW(vad->get("nonumber"), std::out_of_range);
	EXPECT_EQ(vad->get("1"), prop1);
	EXPECT_EQ(vad->get("2"), prop3);
	EXPECT_EQ(vad->get("3"), prop2);


	// Array:get(string) const
	EXPECT_THROW(cvad->get("0"), std::out_of_range);
	EXPECT_THROW(cvad->get("nonumber"), std::out_of_range);
	EXPECT_EQ(cvad->get("1"), prop1);
	EXPECT_EQ(cvad->get("2"), prop3);
	EXPECT_EQ(cvad->get("3"), prop2);


	// Array::index(string)
	EXPECT_EQ(vad->index("0"), -1);
	EXPECT_EQ(vad->index("nonumber"), -1);
	EXPECT_EQ(vad->index("1"), 0);
	EXPECT_EQ(vad->index("2"), 1);
	EXPECT_EQ(vad->index("3"), 2);


	// Array::name(index)
	EXPECT_THROW(vad->name(-1), std::out_of_range);
	EXPECT_THROW(vad->name(4), std::out_of_range);
	EXPECT_EQ(vad->name(0), "1");
	EXPECT_EQ(vad->name(1), "2");
	EXPECT_EQ(vad->name(2), "3");



	// Array::removeProperty
	EXPECT_THROW(vad->addProperty(-2), std::out_of_range);
	EXPECT_THROW(vad->addProperty(4), std::out_of_range);

	vad->removeProperty(0);
	EXPECT_EQ(vad->size(), 2);
	EXPECT_EQ(vad->get(0), prop3);
	EXPECT_EQ(vad->get(1), prop2);

	vad->removeProperty(1);
	EXPECT_EQ(vad->size(), 1);
	EXPECT_EQ(vad->get(0), prop3);

	vad->removeProperty(0);
	EXPECT_EQ(vad->size(), 0);


	// Array::addProperty(ValueBase * property, int index_before)
	auto vd = new Value<double>();
	EXPECT_THROW(vad->addProperty(vd, -2), std::out_of_range);
	EXPECT_THROW(vad->addProperty(vd, 2), std::out_of_range);

	prop1 = vad->addProperty(new Value<double>());
	prop2 = vad->addProperty(new Value<double>(), -1);
	prop3 = vad->addProperty(new Value<double>(), 1);
	EXPECT_EQ(vad->size(), 3);
	EXPECT_EQ(vad->get(0), prop1);
	EXPECT_EQ(vad->get(1), prop3);
	EXPECT_EQ(vad->get(2), prop2);


	// element Value::operator=
	EXPECT_EQ((*vad)[0]->asDouble(), 0.0);
	EXPECT_EQ((*vad)[1]->asDouble(), 0.0);
	EXPECT_EQ((*vad)[2]->asDouble(), 0.0);
	*prop1 = 1.0;
	*prop2 = 2.0;
	*prop3 = 3.0;
	EXPECT_EQ((*vad)[0]->asDouble(), 1.0);
	EXPECT_EQ((*vad)[1]->asDouble(), 3.0);
	EXPECT_EQ((*vad)[2]->asDouble(), 2.0);


	// Array::compare(std::vector<T> const&)
	EXPECT_TRUE(vad->compare(std::vector<double>({1.0, 3.0, 2.0})));
	EXPECT_FALSE(vad->compare(std::vector<double>({1.0, 2.0, 3.0})));
	EXPECT_FALSE(vad->compare(std::vector<double>({1.0, 3.0})));
	EXPECT_FALSE(vad->compare(std::vector<double>({1.0, 3.0, 2.0, 0.0})));

	// Array::compare(const Array&)
	{
		Value<Array<double>> vad2;
		EXPECT_FALSE(vad2->compare(*vad));
		EXPECT_FALSE(vad->compare(*vad2));
		vad2 = vad;
		EXPECT_TRUE(vad2->compare(*vad));
		EXPECT_TRUE(vad->compare(*vad2));
	}

	// Array::set
	{
		Array<double> ad2;
		std::vector<double> vec({1.0, 2.0, 3.0});
		ad2.set(vec);
		EXPECT_TRUE(ad2.compare(vec));
	}

	// Array:get results in statically known Value<T>*
	EXPECT_EQ(**vad->get(0), 1.0);
	EXPECT_EQ(**vad->get(1), 3.0);
	EXPECT_EQ(**vad->get(2), 2.0);

	// Value::operator=(Value&)
	{
		Value<Array<double>> vad2;
		vad2 = vad;

		EXPECT_EQ(vad2->size(), 3);
		EXPECT_EQ((*vad2)[0]->asDouble(), 1.0);
		EXPECT_EQ((*vad2)[1]->asDouble(), 3.0);
		EXPECT_EQ((*vad2)[2]->asDouble(), 2.0);
	}

	// Value<T>::operator=(T&)
	{
		Value<Array<double>> vad2;
		vad2 = ad;
		EXPECT_TRUE(*vad2 == ad);
	}

	// Value::operator=(ValueBase&)
	EXPECT_THROW(vad = vai, std::runtime_error);

	Value<Array<double>> vad2;
	vad2 = *(static_cast<ValueBase*>(&vad));

	EXPECT_EQ(vad2->size(), 3);
	EXPECT_EQ((*vad2)[0]->asDouble(), 1.0);
	EXPECT_EQ((*vad2)[1]->asDouble(), 3.0);
	EXPECT_EQ((*vad2)[2]->asDouble(), 2.0);

	// Value::assign
	Value<Array<double>> vad3;
	EXPECT_TRUE(vad3.assign(vad));

	EXPECT_EQ(vad3->size(), 3);
	EXPECT_EQ((*vad3)[0]->asDouble(), 1.0);
	EXPECT_EQ((*vad3)[1]->asDouble(), 3.0);
	EXPECT_EQ((*vad3)[2]->asDouble(), 2.0);

	// Value::operator==
	EXPECT_FALSE(vad == vai);
	EXPECT_TRUE(vad == vad2);

	// Value::compare
	EXPECT_FALSE(vad.compare(vai, nullptr));
	EXPECT_TRUE(vad.compare(vad2, nullptr));

	// Value::copyAnnotationData -> only check exception behaviour for now
	vad2.copyAnnotationData(vad);
	// type mismatch:
	EXPECT_THROW(vad2.copyAnnotationData(vai), std::runtime_error);
	vad2 = ad;
	EXPECT_EQ(vad2->size(), 2);
	EXPECT_EQ(vad->size(), 3);
	// array size mismatch
	EXPECT_THROW(vad2.copyAnnotationData(vai), std::runtime_error);

	// Value::asStruct -> throw
	EXPECT_THROW(vad.asStruct(), std::runtime_error);

	// Value::asArray -> check no throw
	vad.asArray();
	(static_cast<const Value<Array<double>>&>(vad)).asArray();

	// Value::setArray
	vad.setArray(ad);
	EXPECT_EQ(vad->size(), 2);
	EXPECT_EQ((*vad)[0]->asDouble(), -1.0);
	EXPECT_EQ((*vad)[1]->asDouble(), -2.0);
	EXPECT_TRUE(*vad == ad);

	EXPECT_THROW(vad.setArray(ai), std::runtime_error);

	// Value::clone
	auto vad_clone = vad.clone(nullptr);
	EXPECT_TRUE(vad == *vad_clone);
}

TEST(ValueTest, ArrayNested) {
	Value<Array<Array<double>>> vaad;
	auto prop1 = vaad->addProperty();

	EXPECT_EQ(vaad.type(), PrimitiveType::Array);
	EXPECT_EQ(vaad->elementType(), PrimitiveType::Array);

	EXPECT_EQ(prop1->type(), PrimitiveType::Array);
	EXPECT_EQ((*prop1)->elementType(), PrimitiveType::Double);

	// Value::typeName
	EXPECT_EQ(vaad.typeName(), "Array[Array[Double]]");
	// Value::baseTypeName
	EXPECT_EQ(vaad.baseTypeName(), "Array[Array[Double]]");
	// Array::typeName
	EXPECT_EQ(vaad->typeName(), "Array[Array[Double]]");
	// Array::elementTypeName
	EXPECT_EQ(vaad->elementTypeName(), "Array[Double]");
	EXPECT_EQ(prop1->typeName(), "Array[Double]");
	EXPECT_EQ((*prop1)->elementTypeName(), "Double");
}