/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "core/Context.h"
#include "core/Handles.h"
#include "core/Project.h"
#include "testing/MockUserTypes.h"
#include "user_types/Node.h"

#include "gtest/gtest.h"

using namespace raco::core;
using namespace raco::user_types;

TEST(HandleTests, ValueHandle)
{
	std::shared_ptr<Foo> foo { new Foo() };
	std::shared_ptr<Foo> bar { new Foo() };
	foo->ref_ = bar;

	ValueHandle o(foo);
	ValueHandle hbar(bar);
	EXPECT_TRUE(o);
	EXPECT_TRUE(o.isObject());
	EXPECT_TRUE(o.hasSubstructure());
	EXPECT_FALSE(o.isProperty());

	ValueHandle invalid = o.get("nosuchproperty");
	EXPECT_FALSE(invalid);
	EXPECT_FALSE(ValueHandle(foo, {"nosuchproperty"}));
	
	ValueHandle vh_x = o.get("x");
	double x_value = vh_x.asDouble();
	EXPECT_EQ(x_value, 2.5);
	EXPECT_EQ(vh_x.as<double>(), 2.5);

	ValueHandle vhi = o.get("i");
	EXPECT_EQ(vhi.as<int>(), 3);

	ValueHandle vhb = o.get("flag");
	EXPECT_EQ(vhb.as<bool>(), false);

	ValueHandle vhs = o.get("s");
	EXPECT_EQ(vhs.as<std::string>(), "cat");

	ValueHandle vh_ref = o.get("ref");
	ValueHandle ref_value = vh_ref.asRef();
	EXPECT_EQ(ref_value, hbar);

	auto refPtr = vh_ref.asTypedRef<Foo>();
	EXPECT_EQ(refPtr, bar);
	
	ValueHandle hbar_ref = hbar.get("ref");
	ValueHandle bar_ref_value = hbar_ref.asRef();
	EXPECT_FALSE(bar_ref_value);

	ValueHandle vh_vec = o.get("vec");
	EXPECT_TRUE(vh_vec.hasSubstructure());
	EXPECT_TRUE(vh_vec.isProperty());
	double vec_y_value = vh_vec.get("y").asDouble();
	EXPECT_EQ(vec_y_value, 2.0);

	EXPECT_TRUE(vh_vec.contains(vh_vec.get("y")));

	EXPECT_EQ(o.depth(), 0);
	EXPECT_EQ(vh_vec.depth(), 1);
	EXPECT_EQ(vh_vec.get("x").depth(), 2);

	EXPECT_EQ(vh_ref.parent(), o);
	EXPECT_EQ(vh_vec.get("y").parent(), vh_vec);
}

TEST(HandleTests, Node)
{
	std::shared_ptr<Node> node { new Node() };

	ValueHandle n(node);

	std::vector<std::string> propNames;
	for (int i = 0; i < n.size(); i++) {
		ValueHandle h = n[i];
		propNames.emplace_back(h.getPropName());
	}
	std::vector<std::string> refPropNames { "objectID", "objectName", "children", "tags", "visibility", "translation", "rotation", "scaling" };
	EXPECT_EQ(propNames, refPropNames);

	ValueHandle n_rot = n.get("rotation");
	ValueHandle n_rot_x = n_rot.get("x");

	EXPECT_EQ(n_rot_x, ValueHandle(node, {"rotation", "x"}));
	EXPECT_EQ(n_rot_x, ValueHandle(node, std::vector<std::string>({"rotation", "x"})));

	EXPECT_FALSE(ValueHandle(node, {"rotation", "nosuchproperty"}));
	
	auto rot_range = n_rot.query<RangeAnnotation<double>>();
	EXPECT_FALSE(rot_range);

	AnnotationHandle<RangeAnnotation<double>> rot_x_range = n_rot_x.query<RangeAnnotation<double>>();
	EXPECT_TRUE(rot_x_range);
	// Direct read access is ok
	auto min = *rot_x_range->min_;
	EXPECT_EQ(min, -360.0);

	// Write access doesn't work; this doesn't compile:
	// rot_x_range->m_max = 2.0;

	AnnotationValueHandle<RangeAnnotation<double>> rot_range_max = rot_x_range.get("max");
	EXPECT_TRUE(rot_range_max);
	auto max = rot_range_max.asDouble();
	EXPECT_EQ(max, 360.0);

	AnnotationValueHandle<RangeAnnotation<double>> rot_range_invalid = rot_x_range.get("invalid");
	EXPECT_FALSE(rot_range_invalid);
}
