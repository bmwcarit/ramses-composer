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

#include "user_types/Node.h"

#include <memory>
#include <stdexcept>
#include <tuple>
#include <vector>

#include "gtest/gtest.h"

using namespace raco::data_storage;
using namespace raco::user_types;

TEST(NodeTest, Basic) {
	Node n;
	Node m{"bar"};
	Node defnode{"foo", "def_node_id"};

	EXPECT_EQ(n.objectName(), "");
	EXPECT_EQ(m.objectName(), "bar");
	EXPECT_EQ(defnode.objectName(), "foo");

	EXPECT_NE(n.objectID(), m.objectID());
	EXPECT_EQ(defnode.objectID(), "def_node_id");
}

TEST(NodeTest, Properties) {
	Node n;

	n.visibility_ = false;
	EXPECT_EQ(*n.visibility_, false);

	n.rotation_->y = 2.0;
	double b = *n.rotation_->y;
	EXPECT_EQ(b, 2.0);

	RangeAnnotation<double>& rangeX = n.rotation_->y.staticQuery<RangeAnnotation<double>>();
	rangeX.min_ = *rangeX.max_ / 2.0;
	EXPECT_EQ(*rangeX.min_, 180.0);

	RangeAnnotation<double>* p_range_x = n.rotation_->y.dynamicQuery<RangeAnnotation<double>>();
	EXPECT_EQ(&rangeX, p_range_x);
}


TEST(NodeTest, ObjectAnnotation) {
	Node n;

	auto anno = std::make_shared<ArraySemanticAnnotation>();

	n.addAnnotation(anno);
	EXPECT_EQ(n.query<ArraySemanticAnnotation>(), anno);

	EXPECT_EQ(n.query<HiddenProperty>(), nullptr);

	n.removeAnnotation(anno);
	EXPECT_EQ(n.query<ArraySemanticAnnotation>(), nullptr);

	n.addAnnotation(anno);
	EXPECT_EQ(n.query<ArraySemanticAnnotation>(), anno);

	n.removeAnnotation<ArraySemanticAnnotation>();
	EXPECT_EQ(n.query<ArraySemanticAnnotation>(), nullptr);

	// Remove non-existing annotation: no effect
	n.removeAnnotation<HiddenProperty>();
}