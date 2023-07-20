/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <core/BasicTypes.h>
#include <core/Handles.h>
#include "user_types/Node.h"
#include "user_types/PerspectiveCamera.h"
#include "user_types/OrthographicCamera.h"

#include "testing/MockUserTypes.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <map>
#include <set>

using namespace raco::core;
using namespace raco::user_types;


TEST(ValueHandle, ValueHandle_std_collections_capable) {
	const std::shared_ptr<Node> editorObject1{std::make_shared<Node>("SomeName1")};
	const std::shared_ptr<Node> editorObject2{std::make_shared<Node>("SomeName2")};

	const ValueHandle valueHandle1{editorObject1};
	const ValueHandle valueHandle2{editorObject2};

	std::map<ValueHandle, int> map{};
	map[valueHandle1] = 1;
	map[valueHandle2] = 2;
	EXPECT_EQ(map[valueHandle1], 1);
	EXPECT_EQ(map[valueHandle2], 2);

	std::set<ValueHandle> set{};
	set.insert(valueHandle1);
	set.insert(valueHandle2);
	EXPECT_EQ(set.size(), 2);
}

TEST(ValueHandle, ValueHandle_comparesByIndex_append) {
	const std::shared_ptr<MockTableObject> tableObject{std::make_shared<MockTableObject>("SomeName1")};

	ValueHandle handle{tableObject};

	tableObject->table_.asTable().addProperty(new Value<Vec2f>());

	ValueHandle child1BeforeAdd = handle.get("table")[0];

	tableObject->table_.asTable().addProperty(new Value<Vec2f>());

	ValueHandle child1AfterAdd = handle.get("table")[0];
	ValueHandle child2AfterAdd = handle.get("table")[1];

	EXPECT_EQ(handle.get("table").size(), 2);

	EXPECT_TRUE(child1BeforeAdd == child1AfterAdd);
	EXPECT_FALSE(child1BeforeAdd == child2AfterAdd);
}

TEST(ValueHandle, ValueHandle_comparesByIndex_prepend) {
	const std::shared_ptr<MockTableObject> tableObject{std::make_shared<MockTableObject>("SomeName1")};

	ValueHandle handle{tableObject};

	tableObject->table_.asTable().addProperty(new Value<Vec2f>());

	ValueHandle child1BeforeAdd = handle.get("table")[0];

	tableObject->table_.asTable().addProperty(new Value<Vec2f>(), 0);

	ValueHandle child1AfterAdd = handle.get("table")[0];
	ValueHandle child2AfterAdd = handle.get("table")[1];

	EXPECT_EQ(handle.get("table").size(), 2);

	// This is weird but expected: the ValueHandle uses indices to identify the
	// element it points to - and therefore now points to the new element.
	EXPECT_FALSE(child1BeforeAdd == child2AfterAdd);
	EXPECT_TRUE(child1BeforeAdd == child1AfterAdd);
}

TEST(ValueHandle, ValueHandle_PropertyPtr) {
	const std::shared_ptr<Node> editorObject{std::make_shared<Node>("SomeName1")};
	const std::shared_ptr<PerspectiveCamera> editorObjectWithBase{std::make_shared<PerspectiveCamera>("SomeName2", std::string{})};

	const ValueHandle valueHandle1a{editorObject, &Node::translation_};
	const ValueHandle valueHandle1b{editorObject, {"translation"}};
	const ValueHandle valueHandle1c{editorObject, {"translation", "y"}};
	const ValueHandle valueHandle1d{editorObject};
	EXPECT_EQ(valueHandle1a, valueHandle1b);
	EXPECT_TRUE(valueHandle1a.isRefToProp(&Node::translation_));
	EXPECT_FALSE(valueHandle1a.isRefToProp(&PerspectiveCamera::frustum_));
	EXPECT_TRUE(valueHandle1b.isRefToProp(&Node::translation_));
	EXPECT_FALSE(valueHandle1b.isRefToProp(&PerspectiveCamera::frustum_));
	EXPECT_FALSE(valueHandle1c.isRefToProp(&Node::translation_));
	EXPECT_FALSE(valueHandle1d.isRefToProp(&Node::translation_));

	const ValueHandle valueHandle2a{editorObject, &Node::translation_, &Vec3f::y};
	const ValueHandle valueHandle2b{editorObject, {"translation", "y"}};
	EXPECT_EQ(valueHandle2a, valueHandle2b);

	const ValueHandle valueHandle3{editorObject, &Node::translation_, &Vec4f::y};
	EXPECT_FALSE(valueHandle3);

	// Compile errors. I'd like to test that. Should be possible with SFINAE?
	// const ValueHandle valueHandle4a{editorObject, &Node::onAfterValueChanged};
	
	const ValueHandle valueHandle5a{editorObjectWithBase, {"translation", "y"}};
	const ValueHandle valueHandle5b{editorObjectWithBase, &PerspectiveCamera::translation_, &Vec3f::y};
	const ValueHandle valueHandle5c{editorObjectWithBase, &Node::translation_, &Vec3f::y};
	EXPECT_EQ(valueHandle5a, valueHandle5b);
	EXPECT_EQ(valueHandle5a, valueHandle5c);

	const ValueHandle valueHandle6{editorObject, &PerspectiveCamera::frustum_};
	EXPECT_FALSE(valueHandle6);
}
