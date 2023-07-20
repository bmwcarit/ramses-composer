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

#include "data_storage/AnnotationBase.h"

#include "testing/StructTypes.h"

#include <memory>
#include <stdexcept>
#include <tuple>
#include <vector>

#include "gtest/gtest.h"

using namespace raco::data_storage;

// Duplicate of Dummy class in MockUserTypes.h:
// Dedpulicating this would need to create a new cmake library which seem overkill for a single helper class only used once here.
class Dummy: public AnnotationBase {
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
TEST(PropertyTest, Scalar)
{

	Property<bool, Dummy> bv{false, {}};
	Property<int, Dummy> iv{0, {}};

	Property<double, Dummy> dv{0.0, {}};
	dv = 2.0;
	EXPECT_EQ(*dv, 2.0);

	EXPECT_EQ(dv.asDouble(), 2.0);
	EXPECT_EQ(dv.as<double>(), 2.0);

	EXPECT_THROW(dv.asBool(), std::runtime_error);
	EXPECT_THROW(dv.as<bool>(), std::runtime_error);

	Value<double> u;
	u = 3.0;
}


TEST(PropertyTest, Struct) {
	SimpleStruct s;
	s.bb = false;
	s.dd = 2.0;

	Property<SimpleStruct, Dummy> vs{{}, {}};
	const Property<SimpleStruct, Dummy> cvs(s, {});
	Property<AltStruct, Dummy> va;

	EXPECT_EQ(vs.type(), PrimitiveType::Struct);
	EXPECT_EQ(vs.typeName(), "SimpleStruct::Dummy");

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
}
