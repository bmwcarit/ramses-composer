/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <data_storage/BasicTypes.h>
#include <core/Handles.h>
#include "user_types/Node.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <map>
#include <set>

using namespace raco::core;
using namespace raco::user_types;



TEST(ValueHandle, std_collections_capabalbe) {
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

class MockTableObject : public EditorObject {
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

	Property<Table> table_{{}};
};

TEST(ValueHandle, ValueHandle_comparesByIndex_append) {
	const std::shared_ptr<MockTableObject> tableObject{std::make_shared<MockTableObject>("SomeName1")};

	ValueHandle handle{tableObject};

	tableObject->table_.asTable().addProperty(PrimitiveType::Vec2f);

	ValueHandle child1BeforeAdd = handle.get("table")[0];

	tableObject->table_.asTable().addProperty(PrimitiveType::Vec2f);

	ValueHandle child1AfterAdd = handle.get("table")[0];
	ValueHandle child2AfterAdd = handle.get("table")[1];

	EXPECT_EQ(handle.get("table").size(), 2);

	EXPECT_TRUE(child1BeforeAdd == child1AfterAdd);
	EXPECT_FALSE(child1BeforeAdd == child2AfterAdd);
}

TEST(ValueHandle, ValueHandle_comparesByIndex_prepend) {
	const std::shared_ptr<MockTableObject> tableObject{std::make_shared<MockTableObject>("SomeName1")};

	ValueHandle handle{tableObject};

	tableObject->table_.asTable().addProperty(PrimitiveType::Vec2f);

	ValueHandle child1BeforeAdd = handle.get("table")[0];

	tableObject->table_.asTable().addProperty(PrimitiveType::Vec2f, 0);

	ValueHandle child1AfterAdd = handle.get("table")[0];
	ValueHandle child2AfterAdd = handle.get("table")[1];

	EXPECT_EQ(handle.get("table").size(), 2);

	// This is weird but expected: the ValueHandle uses indices to identify the
	// element it points to - and therefore now points to the new element.
	EXPECT_FALSE(child1BeforeAdd == child2AfterAdd);
	EXPECT_TRUE(child1BeforeAdd == child1AfterAdd);
}
