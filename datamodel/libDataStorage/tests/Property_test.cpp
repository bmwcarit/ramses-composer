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