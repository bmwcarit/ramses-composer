/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "PropertyBrowserItemTestHelper.h"

#include "core/Project.h"
#include "core/Undo.h"

#include "user_types/UserObjectFactory.h"
#include "user_types/Node.h"

#include "ramses_base/HeadlessEngineBackend.h"
#include <property_browser/PropertyBrowserItem.h>

#include <QSignalSpy>
#include <gtest/gtest.h>
#include <memory>

using namespace raco::core;
using namespace raco::user_types;

namespace raco::property_browser {

class MockMutableTable : public EditorObject {
public:
	static inline const TypeDescriptor typeDescription = {"MockMutableTable", true};
	TypeDescriptor const& getTypeDescription() const override {
		return typeDescription;
	}
	MockMutableTable(MockMutableTable const&) = delete;
	MockMutableTable(std::string name = std::string(), std::string id = std::string()) : EditorObject(name, id) {
		fillPropertyDescription();
	}

	void fillPropertyDescription() {
		properties_.emplace_back("table", &table_);
	}

	Property<Table> table_{{}};
};

TEST(PropertyBrowserItem, displayName) {
	PropertyBrowserItemTestHelper<Node> data{};

	const ValueHandle propertyHandle{data.valueHandle.get("translation")};

	PropertyBrowserItem itemUnderTest{propertyHandle, data.dispatcher, &data.commandInterface, nullptr};

	EXPECT_EQ(itemUnderTest.displayName(), "Translation");
}

TEST(PropertyBrowserItem, addNewChildToTable_emits_childrenChanged) {
	PropertyBrowserItemTestHelper<MockMutableTable> data{};
	const ValueHandle propertyHandle{data.valueHandle.get("table")};

	PropertyBrowserItem itemUnderTest{propertyHandle, data.dispatcher, &data.commandInterface, nullptr};
	QSignalSpy spy{&itemUnderTest, SIGNAL(childrenChanged(const QList<PropertyBrowserItem*>&))};

	EXPECT_EQ(itemUnderTest.size(), 0);

	data.addPropertyTo("table", PrimitiveType::Double);

	EXPECT_EQ(spy.count(), 1);
	EXPECT_EQ(itemUnderTest.size(), 1);
}

TEST(PropertyBrowserItem, removeChildFromTable_emits_childrenChanged) {
	PropertyBrowserItemTestHelper<MockMutableTable> data{};
	const ValueHandle propertyHandle{data.valueHandle.get("table")};
	data.addPropertyTo("table", PrimitiveType::Double);

	PropertyBrowserItem itemUnderTest{propertyHandle, data.dispatcher, &data.commandInterface, nullptr};
	QSignalSpy spy{&itemUnderTest, SIGNAL(childrenChanged(const QList<PropertyBrowserItem*>&))};

	EXPECT_EQ(itemUnderTest.size(), 1);

	data.removePropertyFrom("table");

	EXPECT_EQ(spy.count(), 1);
	EXPECT_EQ(itemUnderTest.size(), 0);
}

TEST(PropertyBrowserItem, structuralModification_emits_childrenChangedOrCollapsedChildChanged) {
	PropertyBrowserItemTestHelper<MockMutableTable> data{};
	const ValueHandle propertyHandle{data.valueHandle.get("table")};
	data.addPropertyTo("table", PrimitiveType::Table, "child");

	PropertyBrowserItem tableItem{propertyHandle, data.dispatcher, &data.commandInterface, nullptr};
	PropertyBrowserItem* itemUnderTest{tableItem.children().at(0)};

	QSignalSpy spy{itemUnderTest, &PropertyBrowserItem::childrenChangedOrCollapsedChildChanged};

	data.addPropertyTo("table", "child", PrimitiveType::Double);

	EXPECT_EQ(spy.count(), 1);
}

TEST(PropertyBrowserItem, structuralModification_inCollapsedStructure_emits_childrenChangedOrCollapsedChildChanged) {
	PropertyBrowserItemTestHelper<MockMutableTable> data{};
	const ValueHandle propertyHandle{data.valueHandle.get("table")};
	data.addPropertyTo("table", PrimitiveType::Table, "child");

	PropertyBrowserItem itemUnderTest{propertyHandle, data.dispatcher, &data.commandInterface, nullptr};
	PropertyBrowserItem* childItem{itemUnderTest.children().at(0)};

	itemUnderTest.toggleExpanded();
	EXPECT_EQ(itemUnderTest.expanded(), false);

	QSignalSpy spy{&itemUnderTest, &PropertyBrowserItem::childrenChangedOrCollapsedChildChanged};

	data.addPropertyTo("table", "child", PrimitiveType::Double);

	EXPECT_EQ(spy.count(), 1);
}

TEST(PropertyBrowserItem, setExpanded_influence_showChildren_ifItemHasChildren) {
	PropertyBrowserItemTestHelper<MockMutableTable> data{};
	const ValueHandle propertyHandle{data.valueHandle.get("table")};
	data.addPropertyTo("table", PrimitiveType::Double);

	PropertyBrowserItem itemUnderTest{propertyHandle, data.dispatcher, &data.commandInterface, nullptr};

	EXPECT_EQ(itemUnderTest.expanded(), true);
	EXPECT_EQ(itemUnderTest.showChildren(), true);

	itemUnderTest.toggleExpanded();

	EXPECT_EQ(itemUnderTest.expanded(), false);
	EXPECT_EQ(itemUnderTest.showChildren(), false);
}

TEST(PropertyBrowserItem, setExpanded_doesnt_influence_showChildren_ifItemHasNoChildren) {
	PropertyBrowserItemTestHelper<MockMutableTable> data{};
	const ValueHandle propertyHandle{data.valueHandle.get("table")};

	PropertyBrowserItem itemUnderTest{propertyHandle, data.dispatcher, &data.commandInterface, nullptr};

	EXPECT_EQ(itemUnderTest.expanded(), true);
	EXPECT_EQ(itemUnderTest.showChildren(), false);

	itemUnderTest.toggleExpanded();

	EXPECT_EQ(itemUnderTest.expanded(), false);
	EXPECT_EQ(itemUnderTest.showChildren(), false);
}

TEST(PropertyBrowserItem, setExpanded_influence_showControl_ifItemHasChildren) {
	PropertyBrowserItemTestHelper<MockMutableTable> data{};
	const ValueHandle propertyHandle{data.valueHandle.get("table")};
	data.addPropertyTo("table", PrimitiveType::Double);

	PropertyBrowserItem itemUnderTest{propertyHandle, data.dispatcher, &data.commandInterface, nullptr};

	EXPECT_EQ(itemUnderTest.expanded(), true);
	EXPECT_EQ(itemUnderTest.showControl(), false);

	itemUnderTest.toggleExpanded();

	EXPECT_EQ(itemUnderTest.expanded(), false);
	EXPECT_EQ(itemUnderTest.showControl(), true);
}

TEST(PropertyBrowserItem, setExpanded_doesnt_influence_showControl_ifItemHasNoChildren) {
	PropertyBrowserItemTestHelper<MockMutableTable> data{};
	const ValueHandle propertyHandle{data.valueHandle.get("table")};

	PropertyBrowserItem itemUnderTest{propertyHandle, data.dispatcher, &data.commandInterface, nullptr};

	EXPECT_EQ(itemUnderTest.expanded(), true);
	EXPECT_EQ(itemUnderTest.showControl(), true);

	itemUnderTest.toggleExpanded();

	EXPECT_EQ(itemUnderTest.expanded(), false);
	EXPECT_EQ(itemUnderTest.showControl(), true);
}

}  // namespace raco::property_browser
