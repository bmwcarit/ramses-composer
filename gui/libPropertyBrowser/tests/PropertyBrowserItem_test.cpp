/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "EditorTestFixture.h"

#include <property_browser/PropertyBrowserItem.h>

#include <QSignalSpy>
#include <gtest/gtest.h>

using namespace raco::core;
using namespace raco::user_types;

namespace raco::property_browser {

class PropertyBrowserItemTest : public EditorTestFixture {
public:
	PropertyBrowserItemTest() : EditorTestFixture(&TestObjectFactory::getInstance()) {}

 	void addProperty(ValueHandle handle, std::string name, PrimitiveType type) {
		context.addProperty(handle, name, raco::core::ValueBase::create(type));
		dispatch();
	}

	void addProperty(ValueHandle handle, std::string name, ValueBase* property) {
		context.addProperty(handle, name, std::unique_ptr<ValueBase>(property));
		dispatch();
	}

	void removeProperty(ValueHandle handle, const std::string& propName) {
		context.removeProperty(handle, propName);
		dispatch();
	}
};

TEST_F(PropertyBrowserItemTest, displayName) {
	auto node = create<Node>("node");

	const ValueHandle propertyHandle{node, {"translation"}};
	PropertyBrowserItem itemUnderTest{propertyHandle, dataChangeDispatcher, &commandInterface, nullptr};

	EXPECT_EQ(itemUnderTest.displayName(), "Translation");
}

TEST_F(PropertyBrowserItemTest, addNewChildToTable_emits_childrenChanged) {
	auto object = create<MockTableObject>("test");

	const ValueHandle tableHandle{object, {"table"}};
	PropertyBrowserItem itemUnderTest{tableHandle, dataChangeDispatcher, &commandInterface, nullptr};
	QSignalSpy spy{&itemUnderTest, SIGNAL(childrenChanged(const QList<PropertyBrowserItem*>&))};

	EXPECT_EQ(itemUnderTest.size(), 0);

	addProperty(tableHandle, "double_prop", PrimitiveType::Double);

	EXPECT_EQ(spy.count(), 1);
	EXPECT_EQ(itemUnderTest.size(), 1);
}

TEST_F(PropertyBrowserItemTest, removeChildFromTable_emits_childrenChanged) {
	auto object = create<MockTableObject>("test");

	const ValueHandle tableHandle{object, {"table"}};
	addProperty(tableHandle, "double_prop", PrimitiveType::Double);

	PropertyBrowserItem itemUnderTest{tableHandle, dataChangeDispatcher, &commandInterface, nullptr};
	QSignalSpy spy{&itemUnderTest, SIGNAL(childrenChanged(const QList<PropertyBrowserItem*>&))};

	EXPECT_EQ(itemUnderTest.size(), 1);

	removeProperty(tableHandle, "double_prop");

	EXPECT_EQ(spy.count(), 1);
	EXPECT_EQ(itemUnderTest.size(), 0);
}

TEST_F(PropertyBrowserItemTest, structuralModification_emits_childrenChangedOrCollapsedChildChanged) {
	auto object = create<MockTableObject>("test");
	const ValueHandle tableHandle{object, {"table"}};
	addProperty(tableHandle, "child", PrimitiveType::Table);

	PropertyBrowserItem tableItem{tableHandle, dataChangeDispatcher, &commandInterface, nullptr};
	PropertyBrowserItem* childItem{tableItem.children().at(0)};

	QSignalSpy spy{childItem, &PropertyBrowserItem::childrenChangedOrCollapsedChildChanged};

	addProperty(tableHandle.get("child"), "double_prop", PrimitiveType::Double);

	EXPECT_EQ(spy.count(), 1);
}

TEST_F(PropertyBrowserItemTest, structuralModification_inCollapsedStructure_emits_childrenChangedOrCollapsedChildChanged) {
	auto object = create<MockTableObject>("test");
	const ValueHandle tableHandle{object, {"table"}};
	addProperty(tableHandle, "child", PrimitiveType::Table);

	PropertyBrowserItem tableItem{tableHandle, dataChangeDispatcher, &commandInterface, nullptr};
	PropertyBrowserItem* childItem{tableItem.children().at(0)};

	tableItem.setExpanded(false);
	EXPECT_EQ(tableItem.expanded(), false);

	QSignalSpy spy{&tableItem, &PropertyBrowserItem::childrenChangedOrCollapsedChildChanged};

	addProperty(tableHandle, "double_prop", PrimitiveType::Double);

	EXPECT_EQ(spy.count(), 1);
}

TEST_F(PropertyBrowserItemTest, setExpanded_influence_showChildren_ifItemHasChildren) {
	auto object = create<MockTableObject>("test");
	const ValueHandle tableHandle{object, {"table"}};
	addProperty(tableHandle, "double_prop", PrimitiveType::Double);

	PropertyBrowserItem itemUnderTest{tableHandle, dataChangeDispatcher, &commandInterface, nullptr};

	EXPECT_EQ(itemUnderTest.expanded(), true);
	EXPECT_EQ(itemUnderTest.showChildren(), true);

	itemUnderTest.setExpanded(false);

	EXPECT_EQ(itemUnderTest.expanded(), false);
	EXPECT_EQ(itemUnderTest.showChildren(), false);
}

TEST_F(PropertyBrowserItemTest, setExpanded_doesnt_influence_showChildren_ifItemHasNoChildren) {
	auto object = create<MockTableObject>("test");
	const ValueHandle tableHandle{object, {"table"}};

	PropertyBrowserItem itemUnderTest{tableHandle, dataChangeDispatcher, &commandInterface, nullptr};

	EXPECT_EQ(itemUnderTest.expanded(), true);
	EXPECT_EQ(itemUnderTest.showChildren(), false);

	itemUnderTest.setExpanded(false);

	EXPECT_EQ(itemUnderTest.expanded(), false);
	EXPECT_EQ(itemUnderTest.showChildren(), false);
}

TEST_F(PropertyBrowserItemTest, setExpandedRecursively) {
	auto object = create<MockTableObject>("test");
	const ValueHandle tableHandle{object, {"table"}};

	addProperty(tableHandle, "vec", new Value<Vec3f>());
	const ValueHandle vecHandle{tableHandle.get("vec")};

	PropertyBrowserItem tableItem{tableHandle, dataChangeDispatcher, &commandInterface, nullptr};
	PropertyBrowserItem * vecItem = tableItem.children().front();

	EXPECT_EQ(tableItem.expanded(), true);
	EXPECT_EQ(vecItem->expanded(), true);

	tableItem.setExpanded(false);

	EXPECT_EQ(tableItem.expanded(), false);
	EXPECT_EQ(vecItem->expanded(), true);

	tableItem.setExpanded(true);

	EXPECT_EQ(tableItem.expanded(), true);
	EXPECT_EQ(vecItem->expanded(), true);

	tableItem.setExpandedRecursively(false);

	EXPECT_EQ(tableItem.expanded(), false);
	EXPECT_EQ(vecItem->expanded(), false);

	tableItem.setExpandedRecursively(true);

	EXPECT_EQ(tableItem.expanded(), true);
	EXPECT_EQ(vecItem->expanded(), true);
}

}  // namespace raco::property_browser
