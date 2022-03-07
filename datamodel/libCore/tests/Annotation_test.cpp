/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "core/BasicTypes.h"
#include "data_storage/Value.h"

#include <memory>
#include <stdexcept>
#include <tuple>
#include <vector>

#include "gtest/gtest.h"

using namespace raco::core;
using namespace raco::data_storage;

class PropTest {
public:
	Property<double, DisplayNameAnnotation, RangeAnnotation<double>> val{0.0, DisplayNameAnnotation{"Foo"}, RangeAnnotation(0.0, 1.0)};

	Property<double> bare{0.0};
};

TEST(AnnotationQueryTest, Scalar)
{

	PropTest mp;
	mp.val = 1.23;

	// Static (compile-time) annotation queries

	RangeAnnotation<double>& anno = mp.val.staticQuery<RangeAnnotation<double>>();
	anno.min_ = (*anno.max_) / 2.0;
	EXPECT_EQ(*anno.min_, 0.5);

	RangeAnnotation<double>* range_ptr = mp.val.query<RangeAnnotation<double>>();
	range_ptr->max_ = *range_ptr->min_ + 3.0;
	EXPECT_EQ(*range_ptr->max_, 3.5);

	// Static queries for non-existing annotations don't compile:
	//RangeAnnotation& bare_anno = mp.bare.staticQuery<RangeAnnotation>();

	// Here query is converted to staticQuery at compile time -> doesn't compile either:
	//RangeAnnotation* bare_range_ptr = mp.bare.query<RangeAnnotation>();

	// dynamic query compiles but will return nullptr at runtime:
	RangeAnnotation<double>* bare_range_ptr = mp.bare.dynamicQuery<RangeAnnotation<double>>();
	EXPECT_EQ(bare_range_ptr, nullptr);

	// Dynamic (run-time) annotation queries
	ValueBase* v = &mp.val;

}

TEST(AnnotationQueryTest, Vec3f)
{
	Value<Vec3f> v { Vec3f(0.0) };
	RangeAnnotation<double>& range = v->x.staticQuery<RangeAnnotation<double>>();
	RangeAnnotation<double>* p_range = v->x.dynamicQuery<RangeAnnotation<double>>();
	EXPECT_EQ(&range, p_range);


	Property<Vec3f> pv { Vec3f(0.0) };
	RangeAnnotation<double>& prange = pv->x.staticQuery<RangeAnnotation<double>>();
	RangeAnnotation<double>* p_prange = pv->x.dynamicQuery<RangeAnnotation<double>>();
	EXPECT_EQ(&prange, p_prange);
}

class ClassWithVec3f {
public:
	Property<Vec3f> v { Vec3f(0.0) };
};

TEST(AnnotationQueryTest, ClassWithVec3f)
{
	ClassWithVec3f c;

	RangeAnnotation<double>* range = c.v->x.dynamicQuery<RangeAnnotation<double>>();
}

