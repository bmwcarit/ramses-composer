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

#include "testing/MockUserTypes.h"
#include "testing/StructTypes.h"

#include <property_browser/PropertyBrowserItem.h>

#include "user_types/AnimationChannel.h"
#include "user_types/CubeMap.h"
#include "user_types/RenderBuffer.h"

#include <QSignalSpy>
#include <gtest/gtest.h>
#include <set>

using namespace raco::core;
using namespace raco::user_types;

namespace raco::property_browser {

template <typename T>
class PropertyBrowserItemTestT : public EditorTestFixtureT<T> {
public:
	PropertyBrowserItemTestT() : EditorTestFixtureT<T>(&TestObjectFactory::getInstance()) {}

	void addProperty(ValueHandle handle, std::string name, PrimitiveType type) {
		this->context.addProperty(handle, name, core::ValueBase::create(type));
		this->dispatch();
	}

	void addProperty(ValueHandle handle, std::string name, std::unique_ptr<ValueBase>&& property) {
		this->context.addProperty(handle, name, std::move(property));
		this->dispatch();
	}

	void addProperty(ValueHandle handle, std::string name, ValueBase* property) {
		this->context.addProperty(handle, name, std::unique_ptr<ValueBase>(property));
		this->dispatch();
	}

	void removeProperty(ValueHandle handle, const std::string& propName) {
		this->context.removeProperty(handle, propName);
		this->dispatch();
	}

	bool hasNamedChild(PropertyBrowserItem& item, const std::string& name) {
		return std::any_of(item.children().begin(), item.children().end(), [name](auto child) {
			return child->getPropertyName() == name;
		});
	}

	bool testMatching(std::unique_ptr<ValueBase>&& property1, std::unique_ptr<ValueBase>&& property2) {
		auto object_1 = this->template create<MockTableObject>("test");
		auto object_2 = this->template create<MockTableObject>("test");
		const ValueHandle tableHandle_1(object_1, {"table"});
		const ValueHandle tableHandle_2(object_2, {"table"});

		addProperty(tableHandle_1, "prop", std::move(property1));
		addProperty(tableHandle_2, "prop", std::move(property2));

		PropertyBrowserItem item({tableHandle_1, tableHandle_2}, this->dataChangeDispatcher, &this->commandInterface, nullptr);

		return hasNamedChild(item, "prop");
	}

	bool testMatching(ValueBase* property1, ValueBase* property2) {
		return testMatching(std::unique_ptr<ValueBase>(property1), std::unique_ptr<ValueBase>(property2));
	}

	bool testMatching(PrimitiveType type1, PrimitiveType type2) {
		return testMatching(ValueBase::create(type1), ValueBase::create(type2));
	}

	bool testNestedMatching(ValueBase* property_1, ValueBase* property_2) {
		data_storage::Table table_1;
		table_1.addProperty("nested", property_1);

		data_storage::Table table_2;
		table_2.addProperty("nested", property_2);

		return testMatching(new Value<Table>(table_1), new Value<Table>(table_2));
	}
};

using PropertyBrowserItemTest = PropertyBrowserItemTestT<::testing::Test>;

TEST_F(PropertyBrowserItemTest, displayName) {
	auto node = create<Node>("node");

	const ValueHandle propertyHandle{node, {"translation"}};
	std::set handles{propertyHandle};
	PropertyBrowserItem itemUnderTest{handles, dataChangeDispatcher, &commandInterface, nullptr};

	EXPECT_EQ(itemUnderTest.displayName(), "Translation");
}

struct MatchingTestParams {
	std::string testName;
	std::function<ValueBase*()> createProperty1;
	std::function<ValueBase*()> createProperty2;
	bool matchExpected;
};

class PropertyBrowserItemMatchingTestFixture : public PropertyBrowserItemTestT<::testing::TestWithParam<MatchingTestParams>> {
public:
	struct PrintToStringParamName {
		template <class ParamType>
		std::string operator()(const testing::TestParamInfo<ParamType>& info) const {
			return static_cast<MatchingTestParams>(info.param).testName;
		}
	};
};

TEST_P(PropertyBrowserItemMatchingTestFixture, flat_simple) {
	EXPECT_EQ(testMatching(GetParam().createProperty1(), GetParam().createProperty2()), GetParam().matchExpected);
}

TEST_P(PropertyBrowserItemMatchingTestFixture, nested_simple) {
	EXPECT_EQ(testNestedMatching(GetParam().createProperty1(), GetParam().createProperty2()), GetParam().matchExpected);
}

INSTANTIATE_TEST_SUITE_P(
	PropertyBrowserItemMatchingTest,
	PropertyBrowserItemMatchingTestFixture,
	::testing::Values(
		MatchingTestParams{
			"display_name_annotation_same",
			[]() { return new Property<double, DisplayNameAnnotation>({}, {"Cat"}); },
			[]() { return new Property<double, DisplayNameAnnotation>({}, {"Cat"}); },
			true},

		MatchingTestParams{
			"display_name_annotation_different",
			[]() { return new Property<double, DisplayNameAnnotation>({}, {"Cat"}); },
			[]() { return new Property<double, DisplayNameAnnotation>({}, {"Dog"}); },
			false},

		MatchingTestParams{
			"double_double",
			[]() { return new Value<double>(); },
			[]() { return new Value<double>(); },
			true},

		MatchingTestParams{
			"double_int",
			[]() { return new Value<double>(); },
			[]() { return new Value<int>(); },
			false},

		MatchingTestParams{
			"int_int",
			[]() { return new Value<int>(); },
			[]() { return new Value<int>(); },
			true},

		MatchingTestParams{
			"int_int64",
			[]() { return new Value<int>(); },
			[]() { return new Value<int64_t>(); },
			false},

		MatchingTestParams{
			"vec2f_vec2f",
			[]() { return new Value<Vec2f>(); },
			[]() { return new Value<Vec2f>(); },
			true},

		MatchingTestParams{
			"vec2f_vec3f",
			[]() { return new Value<Vec2f>(); },
			[]() { return new Value<Vec3f>(); },
			false},

		MatchingTestParams{
			"int_enum",
			[]() { return new Property<int, Dummy>(); },
			[]() { return new Property<int, Dummy, EnumerationAnnotation>(0, {}, {EUserTypeEnumerations::CullMode}); },
			false},

		MatchingTestParams{
			"enum_enum_same",
			[]() { return new Property<int, Dummy, EnumerationAnnotation>(0, {}, {EUserTypeEnumerations::CullMode}); },
			[]() { return new Property<int, Dummy, EnumerationAnnotation>(0, {}, {EUserTypeEnumerations::CullMode}); },
			true},

		MatchingTestParams{
			"enum_enum_different",
			[]() { return new Property<int, Dummy, EnumerationAnnotation>(0, {}, {EUserTypeEnumerations::CullMode}); },
			[]() { return new Property<int, Dummy, EnumerationAnnotation>(0, {}, {EUserTypeEnumerations::BlendFactor}); },
			false},

		MatchingTestParams{
			"string_string",
			[]() { return new Property<std::string, Dummy>(); },
			[]() { return new Property<std::string, Dummy>(); },
			true},

		MatchingTestParams{
			"string_uri",
			[]() { return new Property<std::string, Dummy>(); },
			[]() { return new Property<std::string, Dummy, URIAnnotation>(); },
			false},

		MatchingTestParams{
			"uri_uri_same_filter",
			[]() { return new Property<std::string, URIAnnotation>({}, {"*.png"}); },
			[]() { return new Property<std::string, URIAnnotation>({}, {"*.png"}); },
			true},

		MatchingTestParams{
			"uri_uri_diff_filter",
			[]() { return new Property<std::string, URIAnnotation>({}, {"*.png"}); },
			[]() { return new Property<std::string, URIAnnotation>({}, {"*.lua"}); },
			false},

		MatchingTestParams{
			"ref_ref_same",
			[]() { return new Value<STextureSampler2DBase>(); },
			[]() { return new Value<STextureSampler2DBase>(); },
			true},

		MatchingTestParams{
			"ref_ref_same_type_diff_anno",
			[]() { return new Value<STextureSampler2DBase>(); },
			[]() { return new Property<STextureSampler2DBase, Dummy>(); },
			true},

		MatchingTestParams{
			"ref_ref_diff",
			[]() { return new Value<STextureSampler2DBase>(); },
			[]() { return new Value<SCubeMap>(); },
			false},

		MatchingTestParams{
			"struct_struct_same",
			[]() { return new Value<SimpleStruct>(); },
			[]() { return new Value<SimpleStruct>(); },
			true},

		MatchingTestParams{
			"struct_struct_same_type_diff_anno",
			[]() { return new Value<SimpleStruct>(); },
			[]() { return new Property<SimpleStruct, Dummy>(); },
			true},

		MatchingTestParams{
			"struct_struct_diff",
			[]() { return new Value<SimpleStruct>(); },
			[]() { return new Value<AltStruct>(); },
			false},

		MatchingTestParams{
			"struct_struct_same_ref",
			[]() { return new Value<StructWithRef>(); },
			[]() { return new Value<StructWithRef>(); },
			true},

		MatchingTestParams{
			"struct_ref_vs_table",
			[]() { return new Value<StructWithRef>(); },
			[]() {
				data_storage::Table table;
				table.addProperty("ref", new Value<SEditorObject>());
				return new Value<Table>(table);
			},
			false},

		MatchingTestParams{
			"array_vs_scalar",
			[]() { return new Value<Array<double>>(); },
			[]() { return new Value<double>(); },
			false},

		MatchingTestParams{
			"array_double_vs_double_same_size",
			[]() {
				auto v = new Value<Array<double>>();
				(*v)->addProperty();
				return v;
			},
			[]() {
				auto v = new Value<Array<double>>();
				(*v)->addProperty();
				return v;
			},
			true},

		MatchingTestParams{
			"array_double_vs_double_diff_size",
			[]() {
				auto v = new Value<Array<double>>();
				(*v)->addProperty();
				(*v)->addProperty();
				return v;
			},
			[]() {
				auto v = new Value<Array<double>>();
				(*v)->addProperty();
				return v;
			},
			false},

		MatchingTestParams{
			"array_double_vs_int_same_size",
			[]() {
				auto v = new Value<Array<double>>();
				(*v)->addProperty();
				return v;
			},
			[]() {
				auto v = new Value<Array<int>>();
				(*v)->addProperty();
				return v;
			},
			false}

		),
	PropertyBrowserItemMatchingTestFixture::PrintToStringParamName());

TEST_F(PropertyBrowserItemTest, matching_flat_uri_path_key_animationChannel_skin) {
	auto animationChannel = create<AnimationChannel>("test");
	auto skin = create<Skin>("test");
	PropertyBrowserItem item({{animationChannel}, {skin}}, dataChangeDispatcher, &commandInterface, nullptr);
	// same path key but different display name
	EXPECT_FALSE(hasNamedChild(item, "uri"));
}

TEST_F(PropertyBrowserItemTest, matching_flat_uri_path_key_luascript_luamodule) {
	auto lua = create<LuaScript>("test");
	auto module = create<LuaScriptModule>("test");
	PropertyBrowserItem item({{lua}, {module}}, dataChangeDispatcher, &commandInterface, nullptr);
	// same path key and same display name
	EXPECT_TRUE(hasNamedChild(item, "uri"));
}

TEST_F(PropertyBrowserItemTest, matching_flat_uri_path_key_luascript_luainterface) {
	auto lua = create<LuaScript>("test");
	auto interface = create<LuaInterface>("test");
	PropertyBrowserItem item({{lua}, {interface}}, dataChangeDispatcher, &commandInterface, nullptr);
	// different path key but same display name
	EXPECT_FALSE(hasNamedChild(item, "uri"));
}

TEST_F(PropertyBrowserItemTest, matching_flat_uri_without_annotation) {
	auto luaText = makeFile("script.lua", R"(
function interface(IN,OUT)
	IN.uri = Type:Float()
end
function run(IN,OUT)
end
)");

	auto intfText = makeFile("interface.lua", R"(
function interface(INOUT)
	INOUT.uri = Type:Float()
end
)");

	auto lua = create_lua("test", luaText);
	auto interface = create_lua_interface("test", intfText);
	PropertyBrowserItem item({{lua}, {interface}}, dataChangeDispatcher, &commandInterface, nullptr);
	EXPECT_TRUE(hasNamedChild(item, "inputs"));
}

TEST_F(PropertyBrowserItemTest, signal_childrenChanged_multi_selection_add_value_single_to_single) {
	auto object_1 = this->create<MockTableObject>("test");
	auto object_2 = this->create<MockTableObject>("test");
	const ValueHandle tableHandle_1{object_1, {"table"}};
	const ValueHandle tableHandle_2{object_2, {"table"}};

	PropertyBrowserItem rootItem{{{object_1}, {object_2}}, dataChangeDispatcher, &commandInterface, nullptr};
	PropertyBrowserItem* childItem{rootItem.findNamedChild("table")};
	QSignalSpy spyRoot{&rootItem, SIGNAL(childrenChanged(const QList<PropertyBrowserItem*>&))};
	QSignalSpy spyChild{childItem, SIGNAL(childrenChanged(const QList<PropertyBrowserItem*>&))};

	EXPECT_EQ(childItem->size(), 0);

	context.addProperty(tableHandle_1, "prop", std::unique_ptr<data_storage::ValueBase>(new Value<double>));
	context.addProperty(tableHandle_2, "prop", std::unique_ptr<data_storage::ValueBase>(new Value<double>));
	dispatch();

	ASSERT_EQ(childItem, rootItem.findNamedChild("table"));
	EXPECT_EQ(spyRoot.count(), 0);
	EXPECT_EQ(spyChild.count(), 2);
	EXPECT_EQ(childItem->size(), 1);
}

TEST_F(PropertyBrowserItemTest, signal_childrenChanged_multi_selection_add_value_single_to_multi) {
	auto object_1 = this->create<MockTableObject>("test");
	auto object_2 = this->create<MockTableObject>("test");
	const ValueHandle tableHandle_1{object_1, {"table"}};
	const ValueHandle tableHandle_2{object_2, {"table"}};

	PropertyBrowserItem rootItem{{{object_1}, {object_2}}, dataChangeDispatcher, &commandInterface, nullptr};
	QSignalSpy spyRoot{&rootItem, SIGNAL(childrenChanged(const QList<PropertyBrowserItem*>&))};

	context.addProperty(tableHandle_1, "prop", std::unique_ptr<data_storage::ValueBase>(new Value<double>));
	dispatch();

	EXPECT_EQ(spyRoot.count(), 1);
	ASSERT_EQ(rootItem.findNamedChild("table"), nullptr);
}

TEST_F(PropertyBrowserItemTest, signal_childrenChanged_multi_selection_add_value_multi_to_single) {
	auto object_1 = this->create<MockTableObject>("test");
	auto object_2 = this->create<MockTableObject>("test");
	const ValueHandle tableHandle_1{object_1, {"table"}};
	const ValueHandle tableHandle_2{object_2, {"table"}};

	context.addProperty(tableHandle_1, "prop", std::unique_ptr<data_storage::ValueBase>(new Value<double>));
	dispatch();

	PropertyBrowserItem rootItem{{{object_1}, {object_2}}, dataChangeDispatcher, &commandInterface, nullptr};
	QSignalSpy spyRoot{&rootItem, SIGNAL(childrenChanged(const QList<PropertyBrowserItem*>&))};

	ASSERT_EQ(rootItem.findNamedChild("table"), nullptr);

	context.addProperty(tableHandle_2, "prop", std::unique_ptr<data_storage::ValueBase>(new Value<double>));
	dispatch();

	PropertyBrowserItem* childItem{rootItem.findNamedChild("table")};

	ASSERT_NE(nullptr, rootItem.findNamedChild("table"));
	EXPECT_EQ(spyRoot.count(), 1);
	EXPECT_EQ(childItem->size(), 1);
}

TEST_F(PropertyBrowserItemTest, signal_childrenChanged_multi_selection_add_value_multi_to_multi) {
	auto object_1 = this->create<MockTableObject>("test");
	auto object_2 = this->create<MockTableObject>("test");
	const ValueHandle tableHandle_1{object_1, {"table"}};
	const ValueHandle tableHandle_2{object_2, {"table"}};

	context.addProperty(tableHandle_1, "prop", std::unique_ptr<data_storage::ValueBase>(new Value<double>));
	dispatch();

	PropertyBrowserItem rootItem{{{object_1}, {object_2}}, dataChangeDispatcher, &commandInterface, nullptr};
	QSignalSpy spyRoot{&rootItem, SIGNAL(childrenChanged(const QList<PropertyBrowserItem*>&))};

	ASSERT_EQ(rootItem.findNamedChild("table"), nullptr);

	context.addProperty(tableHandle_2, "prop", std::unique_ptr<data_storage::ValueBase>(new Value<int>));
	dispatch();

	PropertyBrowserItem* childItem{rootItem.findNamedChild("table")};

	ASSERT_EQ(nullptr, rootItem.findNamedChild("table"));
	EXPECT_EQ(spyRoot.count(), 1);
}



TEST_F(PropertyBrowserItemTest, signal_childrenChanged_multi_selection_resize_array_single_to_single) {
	auto object_1 = this->create<ObjectWithArrays>("test");
	auto object_2 = this->create<ObjectWithArrays>("test");
	const ValueHandle arrayHandle_1{object_1, {"array_double"}};
	const ValueHandle arrayHandle_2{object_2, {"array_double"}};

	PropertyBrowserItem rootItem{{{object_1}, {object_2}}, dataChangeDispatcher, &commandInterface, nullptr};
	PropertyBrowserItem* childItem{rootItem.findNamedChild("array_double")};
	QSignalSpy spyRoot{&rootItem, SIGNAL(childrenChanged(const QList<PropertyBrowserItem*>&))};
	QSignalSpy spyChild{childItem, SIGNAL(childrenChanged(const QList<PropertyBrowserItem*>&))};

	EXPECT_EQ(childItem->size(), 0);

	context.resizeArray(arrayHandle_1, 2);
	context.resizeArray(arrayHandle_2, 2);
	dispatch();

	ASSERT_EQ(childItem, rootItem.findNamedChild("array_double"));
	EXPECT_EQ(spyRoot.count(), 0);
	EXPECT_EQ(spyChild.count(), 2);
	EXPECT_EQ(childItem->size(), 2);
}

TEST_F(PropertyBrowserItemTest, signal_childrenChanged_multi_selection_resize_array_single_to_multi) {
	auto object_1 = this->create<ObjectWithArrays>("test");
	auto object_2 = this->create<ObjectWithArrays>("test");
	const ValueHandle arrayHandle_1{object_1, {"array_double"}};
	const ValueHandle arrayHandle_2{object_2, {"array_double"}};

	PropertyBrowserItem rootItem{{{object_1}, {object_2}}, dataChangeDispatcher, &commandInterface, nullptr};
	QSignalSpy spyRoot{&rootItem, SIGNAL(childrenChanged(const QList<PropertyBrowserItem*>&))};

	context.resizeArray(arrayHandle_1, 2);
	dispatch();

	EXPECT_EQ(spyRoot.count(), 1);
	ASSERT_EQ(rootItem.findNamedChild("array_double"), nullptr);
}

TEST_F(PropertyBrowserItemTest, signal_childrenChanged_multi_selection_resize_array_multi_to_single) {
	auto object_1 = this->create<ObjectWithArrays>("test");
	auto object_2 = this->create<ObjectWithArrays>("test");
	const ValueHandle arrayHandle_1{object_1, {"array_double"}};
	const ValueHandle arrayHandle_2{object_2, {"array_double"}};

	context.resizeArray(arrayHandle_1, 2);
	dispatch();

	PropertyBrowserItem rootItem{{{object_1}, {object_2}}, dataChangeDispatcher, &commandInterface, nullptr};
	QSignalSpy spyRoot{&rootItem, SIGNAL(childrenChanged(const QList<PropertyBrowserItem*>&))};

	ASSERT_EQ(rootItem.findNamedChild("array_double"), nullptr);

	context.resizeArray(arrayHandle_2, 2);
	dispatch();

	PropertyBrowserItem* childItem{rootItem.findNamedChild("array_double")};

	ASSERT_NE(nullptr, rootItem.findNamedChild("array_double"));
	EXPECT_EQ(spyRoot.count(), 1);
	EXPECT_EQ(childItem->size(), 2);
}

TEST_F(PropertyBrowserItemTest, signal_childrenChanged_multi_selection_resize_array_multi_to_multi) {
	auto object_1 = this->create<ObjectWithArrays>("test");
	auto object_2 = this->create<ObjectWithArrays>("test");
	const ValueHandle arrayHandle_1{object_1, {"array_double"}};
	const ValueHandle arrayHandle_2{object_2, {"array_double"}};

	context.resizeArray(arrayHandle_1, 2);
	dispatch();

	PropertyBrowserItem rootItem{{{object_1}, {object_2}}, dataChangeDispatcher, &commandInterface, nullptr};
	QSignalSpy spyRoot{&rootItem, SIGNAL(childrenChanged(const QList<PropertyBrowserItem*>&))};

	ASSERT_EQ(rootItem.findNamedChild("tarray_doubleable"), nullptr);

	context.resizeArray(arrayHandle_2, 3);
	dispatch();

	PropertyBrowserItem* childItem{rootItem.findNamedChild("array_double")};

	ASSERT_EQ(nullptr, rootItem.findNamedChild("array_double"));
	EXPECT_EQ(spyRoot.count(), 1);
}


TEST_F(PropertyBrowserItemTest, signal_childrenChanged_multi_selection_renderBuffer_hide_value_single_to_single) {
	auto object_1 = this->create<RenderBuffer>("test");
	auto object_2 = this->create<RenderBuffer>("test");
	const ValueHandle anisotropyHandle_1{object_1, {"anisotropy"}};
	const ValueHandle anisotropyHandle_2{object_1, {"anisotropy"}};

	PropertyBrowserItem rootItem{{{object_1}, {object_2}}, dataChangeDispatcher, &commandInterface, nullptr};
	QSignalSpy spyRoot{&rootItem, SIGNAL(childrenChanged(const QList<PropertyBrowserItem*>&))};

	ASSERT_NE(rootItem.findNamedChild("anisotropy"), nullptr);

	context.set({object_1, {"format"}}, static_cast<int>(ERenderBufferFormat::Depth24));
	context.set({object_2, {"format"}}, static_cast<int>(ERenderBufferFormat::Depth24));
	dispatch();

	ASSERT_EQ(rootItem.findNamedChild("anisotropy"), nullptr);
	EXPECT_EQ(spyRoot.count(), 2);
}

TEST_F(PropertyBrowserItemTest, signal_childrenChanged_multi_selection_renderBuffer_hide_value_single_to_multi) {
	auto object_1 = this->create<RenderBuffer>("test");
	auto object_2 = this->create<RenderBuffer>("test");
	const ValueHandle anisotropyHandle_1{object_1, {"anisotropy"}};
	const ValueHandle anisotropyHandle_2{object_1, {"anisotropy"}};

	PropertyBrowserItem rootItem{{{object_1}, {object_2}}, dataChangeDispatcher, &commandInterface, nullptr};
	QSignalSpy spyRoot{&rootItem, SIGNAL(childrenChanged(const QList<PropertyBrowserItem*>&))};

	ASSERT_NE(rootItem.findNamedChild("anisotropy"), nullptr);

	context.set({object_1, {"format"}}, static_cast<int>(ERenderBufferFormat::Depth24));
	dispatch();

	ASSERT_EQ(rootItem.findNamedChild("anisotropy"), nullptr);
	EXPECT_EQ(spyRoot.count(), 1);
}

TEST_F(PropertyBrowserItemTest, signal_childrenChanged_multi_selection_renderBuffer_hide_value_multi_to_single) {
	auto object_1 = this->create<RenderBuffer>("test");
	auto object_2 = this->create<RenderBuffer>("test");
	const ValueHandle anisotropyHandle_1{object_1, {"anisotropy"}};
	const ValueHandle anisotropyHandle_2{object_1, {"anisotropy"}};

	context.set({object_1, {"format"}}, static_cast<int>(ERenderBufferFormat::Depth24));
	dispatch();

	PropertyBrowserItem rootItem{{{object_1}, {object_2}}, dataChangeDispatcher, &commandInterface, nullptr};
	QSignalSpy spyRoot{&rootItem, SIGNAL(childrenChanged(const QList<PropertyBrowserItem*>&))};

	ASSERT_EQ(rootItem.findNamedChild("anisotropy"), nullptr);

	context.set({object_2, {"format"}}, static_cast<int>(ERenderBufferFormat::Depth24));
	dispatch();

	ASSERT_EQ(rootItem.findNamedChild("anisotropy"), nullptr);
	EXPECT_EQ(spyRoot.count(), 1);
}

TEST_F(PropertyBrowserItemTest, signal_childrenChanged_multi_selection_renderBuffer_hide_value_multi_to_multi) {
	auto object_1 = this->create<RenderBuffer>("test");
	auto object_2 = this->create<RenderBuffer>("test");
	const ValueHandle anisotropyHandle_1{object_1, {"anisotropy"}};
	const ValueHandle anisotropyHandle_2{object_1, {"anisotropy"}};

	context.set({object_1, {"format"}}, static_cast<int>(ERenderBufferFormat::Depth24));
	dispatch();

	PropertyBrowserItem rootItem{{{object_1}, {object_2}}, dataChangeDispatcher, &commandInterface, nullptr};
	QSignalSpy spyRoot{&rootItem, SIGNAL(childrenChanged(const QList<PropertyBrowserItem*>&))};

	ASSERT_EQ(rootItem.findNamedChild("anisotropy"), nullptr);

	context.set({object_1, {"format"}}, static_cast<int>(ERenderBufferFormat::Depth24));
	context.set({object_2, {"format"}}, static_cast<int>(ERenderBufferFormat::RGBA8));
	dispatch();

	ASSERT_EQ(rootItem.findNamedChild("anisotropy"), nullptr);
	EXPECT_EQ(spyRoot.count(), 2);
}

TEST_F(PropertyBrowserItemTest, signal_childrenChanged_multi_selection_meshnode_options_private_changed) {
	auto mesh = create_mesh("Mesh", "meshes/Duck.glb");
	auto meshnode_1 = create_meshnode("MeshNode 1", mesh, nullptr);
	auto meshnode_2 = create_meshnode("MeshNode 2", mesh, nullptr);

	commandInterface.set(meshnode_1->getMaterialPrivateHandle(0), true);
	commandInterface.set(meshnode_2->getMaterialPrivateHandle(0), true);
	dispatch();

	PropertyBrowserItem rootItem{{{meshnode_1}, {meshnode_2}}, dataChangeDispatcher, &commandInterface, nullptr};
	PropertyBrowserItem* materialItem{rootItem.findNamedChild("materials")};
	PropertyBrowserItem* matSlotItem{materialItem->findNamedChild("material")};
	PropertyBrowserItem* optionsItem{matSlotItem->findNamedChild("options")};

	QSignalSpy spy{optionsItem, SIGNAL(childrenChanged(const QList<PropertyBrowserItem*>&))};

	commandInterface.set(meshnode_1->getMaterialPrivateHandle(0), false);
	dispatch();

	ASSERT_EQ(spy.count(), 1);

	undoStack.undo();
	dispatch();

	ASSERT_EQ(spy.count(), 2);
}


TEST_F(PropertyBrowserItemTest, signal_childrenChanged_single_selection_add) {
	auto object = create<MockTableObject>("test");
	const ValueHandle tableHandle{object, {"table"}};

	PropertyBrowserItem itemUnderTest{{tableHandle}, dataChangeDispatcher, &commandInterface, nullptr};
	QSignalSpy spy{&itemUnderTest, SIGNAL(childrenChanged(const QList<PropertyBrowserItem*>&))};

	EXPECT_EQ(itemUnderTest.size(), 0);

	addProperty(tableHandle, "double_prop", PrimitiveType::Double);

	EXPECT_EQ(spy.count(), 1);
	EXPECT_EQ(itemUnderTest.size(), 1);
}

TEST_F(PropertyBrowserItemTest, signal_childrenChanged_single_selection_remove) {
	auto object = create<MockTableObject>("test");
	const ValueHandle tableHandle{object, {"table"}};

	addProperty(tableHandle, "double_prop", PrimitiveType::Double);

	PropertyBrowserItem itemUnderTest{{tableHandle}, dataChangeDispatcher, &commandInterface, nullptr};
	QSignalSpy spy{&itemUnderTest, SIGNAL(childrenChanged(const QList<PropertyBrowserItem*>&))};

	EXPECT_EQ(itemUnderTest.size(), 1);

	removeProperty(tableHandle, "double_prop");

	EXPECT_EQ(spy.count(), 1);
	EXPECT_EQ(itemUnderTest.size(), 0);
}

TEST_F(PropertyBrowserItemTest, signal_childrenChangedOrCollapsedChildChanged_single_selection_add) {
	auto object = create<MockTableObject>("test");
	const ValueHandle tableHandle{object, {"table"}};

	addProperty(tableHandle, "child", PrimitiveType::Table);

	PropertyBrowserItem tableItem{{tableHandle}, dataChangeDispatcher, &commandInterface, nullptr};
	QSignalSpy spyTable{&tableItem, &PropertyBrowserItem::childrenChangedOrCollapsedChildChanged};
	PropertyBrowserItem* childItem{tableItem.children().at(0)};
	QSignalSpy spyChild{childItem, &PropertyBrowserItem::childrenChangedOrCollapsedChildChanged};

	addProperty(tableHandle.get("child"), "double_prop", PrimitiveType::Double);

	EXPECT_EQ(spyTable.count(), 0);
	EXPECT_EQ(spyChild.count(), 1);
}

TEST_F(PropertyBrowserItemTest, structuralModification_inCollapsedStructure_emits_childrenChangedOrCollapsedChildChanged) {
	auto object = create<MockTableObject>("test");
	const ValueHandle tableHandle{object, {"table"}};
	addProperty(tableHandle, "child", PrimitiveType::Table);

	PropertyBrowserItem tableItem{{tableHandle}, dataChangeDispatcher, &commandInterface, nullptr};

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
	std::set handles{tableHandle};

	PropertyBrowserItem itemUnderTest{handles, dataChangeDispatcher, &commandInterface, nullptr};

	// Important: set expanded to the dafault value to overwrite cache
	itemUnderTest.setExpanded(true);

	EXPECT_EQ(itemUnderTest.expanded(), true);
	EXPECT_EQ(itemUnderTest.showChildren(), true);

	itemUnderTest.setExpanded(false);

	EXPECT_EQ(itemUnderTest.expanded(), false);
	EXPECT_EQ(itemUnderTest.showChildren(), false);
}

TEST_F(PropertyBrowserItemTest, setExpanded_doesnt_influence_showChildren_ifItemHasNoChildren) {
	auto object = create<MockTableObject>("test");
	const ValueHandle tableHandle{object, {"table"}};
	std::set handles{tableHandle};

	PropertyBrowserItem itemUnderTest{handles, dataChangeDispatcher, &commandInterface, nullptr};

	// Important: set expanded to the dafault value to overwrite cache
	itemUnderTest.setExpanded(true);

	EXPECT_EQ(itemUnderTest.expanded(), true);
	EXPECT_EQ(itemUnderTest.showChildren(), false);

	itemUnderTest.setExpanded(false);

	EXPECT_EQ(itemUnderTest.expanded(), false);
	EXPECT_EQ(itemUnderTest.showChildren(), false);
}

TEST_F(PropertyBrowserItemTest, expanded_preserved_by_add) {
	auto object = create<MockTableObject>("test");
	const ValueHandle tableHandle{object, {"table"}};
	addProperty(tableHandle, "child", PrimitiveType::Table);

	PropertyBrowserItem rootItem{{{object}}, dataChangeDispatcher, &commandInterface, nullptr};
	PropertyBrowserItem* tableItem(rootItem.findNamedChild("table"));

	// Important: set expanded to the dafault value to overwrite cache
	tableItem->setExpanded(true);

	EXPECT_EQ(tableItem->expanded(), true);

	tableItem->setExpanded(false);
	EXPECT_EQ(tableItem->expanded(), false);

	addProperty(tableHandle, "double_prop", PrimitiveType::Double);

	EXPECT_EQ(rootItem.findNamedChild("table"), tableItem);
	EXPECT_EQ(tableItem->expanded(), false);
}

TEST_F(PropertyBrowserItemTest, setExpandedRecursively) {
	auto object = create<MockTableObject>("test");
	const ValueHandle tableHandle{object, {"table"}};

	addProperty(tableHandle, "vec", new Value<Vec3f>());
	const ValueHandle vecHandle{tableHandle.get("vec")};
	std::set handles{tableHandle};

	PropertyBrowserItem tableItem{handles, dataChangeDispatcher, &commandInterface, nullptr};
	PropertyBrowserItem* vecItem = tableItem.children().front();

	// Important: set expanded to the dafault value to overwrite cache
	tableItem.setExpanded(true);

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

TEST_F(PropertyBrowserItemTest, cacheExpandedState) {
	auto object_1 = create<MockTableObject>("test_1");
	const ValueHandle tableHandle_1{object_1, {"table"}};
	PropertyBrowserItem tableItem_1{{tableHandle_1}, dataChangeDispatcher, &commandInterface, nullptr};

	tableItem_1.setExpanded(true);
	EXPECT_EQ(tableItem_1.expanded(), true);

	auto object_2 = create<MockTableObject>("test_2");
	const ValueHandle tableHandle_2{object_2, {"table"}};
	PropertyBrowserItem tableItem_2{{tableHandle_2}, dataChangeDispatcher, &commandInterface, nullptr};

	EXPECT_EQ(tableItem_2.expanded(), true);
	tableItem_2.setExpanded(false);
	EXPECT_EQ(tableItem_1.expanded(), false);
	EXPECT_EQ(tableItem_2.expanded(), false);

	auto object_3 = create<MockTableObject>("test_3");
	const ValueHandle tableHandle_3{object_3, {"table"}};
	PropertyBrowserItem tableItem_3{{tableHandle_3}, dataChangeDispatcher, &commandInterface, nullptr};

	EXPECT_EQ(tableItem_3.expanded(), false);
	tableItem_3.setExpanded(true);
	EXPECT_EQ(tableItem_1.expanded(), true);
	EXPECT_EQ(tableItem_2.expanded(), true);
	EXPECT_EQ(tableItem_3.expanded(), true);
}

TEST_F(PropertyBrowserItemTest, highlight_property) {
	const SLuaInterface interface = create_lua_interface("interface", "scripts/interface-scalar-types.lua");
	const ValueHandle interfaceHandle{interface};

	PropertyBrowserItem root_item{{interfaceHandle}, dataChangeDispatcher, &commandInterface, nullptr};
	const auto propertyItem = root_item.findNamedChildByPropertyPath("interface.inputs.bool");
	const QSignalSpy spy{propertyItem, SIGNAL(highlighted())};

	root_item.highlightProperty("non_existing_property");
	EXPECT_EQ(spy.count(), 0);

	root_item.highlightProperty("interface.inputs.bool");
	EXPECT_EQ(spy.count(), 1);
}

}  // namespace raco::property_browser
