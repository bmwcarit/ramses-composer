/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "data_storage/BasicTypes.h"
#include "data_storage/Value.h"

#include "StructTypes.h"

#include <memory>
#include <stdexcept>
#include <tuple>
#include <vector>

#include "gtest/gtest.h"

using namespace raco::data_storage;

TEST(PropertyTest, Scalar)
{

	Property<bool> bv { false };
	Property<int> iv { 0 };

	Property<double> dv { 0.0 };
	dv = 2.0;
	EXPECT_EQ(*dv, 2.0);

	EXPECT_EQ(dv.asDouble(), 2.0);
	EXPECT_EQ(dv.as<double>(), 2.0);

	EXPECT_THROW(dv.asBool(), std::runtime_error);
	EXPECT_THROW(dv.as<bool>(), std::runtime_error);

	Value<double> u;
	u = 3.0;
}

TEST(PropertyTest, Vec3f)
{
	Property<Vec3f> v { Vec3f { 0.0 } };

	v->x = 2.0;
	EXPECT_EQ(*(v->x), 2.0);

	double a = *v->x;
	EXPECT_EQ(a, 2.0);

	ValueBase* vp = &v;

	EXPECT_EQ(*vp->as<Vec3f>().x, 2.0);
	EXPECT_THROW(vp->as<bool>(), std::runtime_error);
}

TEST(PropertyTest, clone) {
	Property<int, HiddenProperty> p_int_hidden {42, {}};
	auto pihc = p_int_hidden.clone(nullptr);
	EXPECT_EQ(pihc->asInt(), *p_int_hidden);

	Property<double, RangeAnnotation<double>> pfr { 2.0, {1, 3}};
	auto pfrc = pfr.clone(nullptr);
	EXPECT_EQ(pfrc->asDouble(), *pfr);

	auto range = pfr.query<RangeAnnotation<double>>();
	auto range_clone = pfrc->query<RangeAnnotation<double>>();

	EXPECT_TRUE(range);
	EXPECT_TRUE(range_clone);

	EXPECT_EQ(*range->min_, *range_clone->min_);
	EXPECT_EQ(*range->max_, *range_clone->max_);

}


TEST(PropertyTest, Struct) {
	SimpleStruct s;
	s.bb = false;
	s.dd = 2.0;

	Property<SimpleStruct> vs;
	const Property<SimpleStruct> cvs(s);
	Property<AltStruct> va;

	EXPECT_EQ(vs.type(), PrimitiveType::Struct);
	EXPECT_EQ(vs.typeName(), "SimpleStruct");

	EXPECT_THROW(vs.asBool(), std::runtime_error);
	EXPECT_THROW(vs.as<bool>(), std::runtime_error);
	EXPECT_THROW(vs.asVec3f(), std::runtime_error);
	EXPECT_THROW(vs.as<Vec3f>(), std::runtime_error);

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
