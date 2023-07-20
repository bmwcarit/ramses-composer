/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "testing/TestEnvironmentCore.h"
#include "testing/TestUtil.h"

#include "user_types/Node.h"
#include "user_types/LuaScript.h"
#include "user_types/Timer.h"

#include "gtest/gtest.h"

using namespace raco::core;
using namespace raco::user_types;

class CommandInterfaceTest : public TestEnvironmentCore {
public:
	CommandInterfaceTest() : TestEnvironmentCore(&TestObjectFactory::getInstance()) {
	}
};

TEST_F(CommandInterfaceTest, set_bool_fail) {
	auto obj = create<Foo>("name");

	EXPECT_THROW(commandInterface.set({obj, &Foo::i_}, true), std::runtime_error);
	EXPECT_THROW(commandInterface.set({obj, {"no_such_property"}}, true), std::runtime_error);

	commandInterface.deleteObjects({obj});
	EXPECT_THROW(commandInterface.set({obj, &Foo::b_}, true), std::runtime_error);
}

TEST_F(CommandInterfaceTest, set_int_fail) {
	auto obj = create<Foo>("name");

	EXPECT_THROW(commandInterface.set({obj, &Foo::b_}, 1), std::runtime_error);
	EXPECT_THROW(commandInterface.set({obj, {"no_such_property"}}, 1), std::runtime_error);

	commandInterface.deleteObjects({obj});
	EXPECT_THROW(commandInterface.set({obj, &Foo::i_}, 1), std::runtime_error);
}

TEST_F(CommandInterfaceTest, set_multi_int_fail) {
	auto obj = create<Foo>("name");

	EXPECT_THROW(commandInterface.set(std::set<ValueHandle>({ValueHandle(obj, &Foo::b_)}), 1), std::runtime_error);
	EXPECT_THROW(commandInterface.set(std::set<ValueHandle>({ValueHandle(obj, {"no_such_property"})}), 1), std::runtime_error);

	commandInterface.deleteObjects({obj});
	EXPECT_THROW(commandInterface.set(std::set<ValueHandle>({ValueHandle(obj, &Foo::i_)}), 1), std::runtime_error);
}

TEST_F(CommandInterfaceTest, set_int64_fail) {
	auto obj = create<Foo>("name");
	int64_t value = 42;

	EXPECT_THROW(commandInterface.set({obj, &Foo::b_}, value), std::runtime_error);
	EXPECT_THROW(commandInterface.set({obj, {"no_such_property"}}, value), std::runtime_error);

	commandInterface.deleteObjects({obj});
	EXPECT_THROW(commandInterface.set({obj, &Foo::i64_}, value), std::runtime_error);
}

TEST_F(CommandInterfaceTest, set_multi_int64_fail) {
	auto obj = create<Foo>("name");
	int64_t value = 42;

	EXPECT_THROW(commandInterface.set(std::set<ValueHandle>({ValueHandle(obj, &Foo::b_)}), value), std::runtime_error);
	EXPECT_THROW(commandInterface.set(std::set<ValueHandle>({ValueHandle(obj, {"no_such_property"})}), value), std::runtime_error);

	commandInterface.deleteObjects({obj});
	EXPECT_THROW(commandInterface.set(std::set<ValueHandle>({ValueHandle(obj, &Foo::i64_)}), value), std::runtime_error);
}

TEST_F(CommandInterfaceTest, set_double_fail) {
	auto obj = create<Foo>("name");

	EXPECT_THROW(commandInterface.set({obj, &Foo::b_}, 2.0), std::runtime_error);
	EXPECT_THROW(commandInterface.set({obj, {"no_such_property"}}, 2.0), std::runtime_error);

	commandInterface.deleteObjects({obj});
	EXPECT_THROW(commandInterface.set({obj, &Foo::x_}, 2.0), std::runtime_error);
}

TEST_F(CommandInterfaceTest, set_multi_double_fail) {
	auto obj = create<Foo>("name");

	EXPECT_THROW(commandInterface.set(std::set<ValueHandle>({ValueHandle(obj, &Foo::b_)}), 2.0), std::runtime_error);
	EXPECT_THROW(commandInterface.set(std::set<ValueHandle>({ValueHandle(obj, {"no_such_property"})}), 2.0), std::runtime_error);

	commandInterface.deleteObjects({obj});
	EXPECT_THROW(commandInterface.set(std::set<ValueHandle>({ValueHandle(obj, &Foo::x_)}), 2.0), std::runtime_error);
}

TEST_F(CommandInterfaceTest, set_multi_double) {
	auto obj = create<Foo>("name");

	commandInterface.set(
		std::set<ValueHandle>({ValueHandle(obj, {"v3f", "x"}),
			ValueHandle(obj, {"v3f", "y"}),
			ValueHandle(obj, {"v3f", "z"})}),
		2.0);
	EXPECT_EQ(*obj->v3f_->x, 2.0);
	EXPECT_EQ(*obj->v3f_->y, 2.0);
	EXPECT_EQ(*obj->v3f_->z, 2.0);
}

TEST_F(CommandInterfaceTest, set_string_fail) {
	auto obj = create<Foo>("name");

	EXPECT_THROW(commandInterface.set({obj, &Foo::b_}, std::string("abc")), std::runtime_error);
	EXPECT_THROW(commandInterface.set({obj, {"no_such_property"}}, std::string("abc")), std::runtime_error);

	commandInterface.deleteObjects({obj});
	EXPECT_THROW(commandInterface.set({obj, &Foo::s_}, std::string("abc")), std::runtime_error);
}

TEST_F(CommandInterfaceTest, set_ref_fail) {
	auto obj = create<Foo>("name");

	EXPECT_THROW(commandInterface.set({obj, &Foo::b_}, SEditorObject{}), std::runtime_error);
	EXPECT_THROW(commandInterface.set({obj, {"no_such_property"}}, SEditorObject{}), std::runtime_error);

	commandInterface.deleteObjects({obj});
	EXPECT_THROW(commandInterface.set({obj, &Foo::ref_}, SEditorObject{}), std::runtime_error);
}

TEST_F(CommandInterfaceTest, set_ref_fail_wrong_type) {
	auto inst = create<PrefabInstance>("inst");
	auto node = create<Node>("node");

	EXPECT_THROW(commandInterface.set({inst, &PrefabInstance::template_}, node), std::runtime_error);
}

TEST_F(CommandInterfaceTest, set_vec2f_fail) {
	auto obj = create<Foo>("name");
	std::array<double, 2> value{1, 2};

	EXPECT_THROW(commandInterface.set({obj, &Foo::b_}, value), std::runtime_error);
	EXPECT_THROW(commandInterface.set({obj, {"no_such_property"}}, value), std::runtime_error);

	commandInterface.deleteObjects({obj});
	EXPECT_THROW(commandInterface.set({obj, &Foo::v2f_}, value), std::runtime_error);
}

TEST_F(CommandInterfaceTest, set_vec3f_fail) {
	auto obj = create<Foo>("name");
	std::array<double, 3> value{1, 2, 3};

	EXPECT_THROW(commandInterface.set({obj, &Foo::b_}, value), std::runtime_error);
	EXPECT_THROW(commandInterface.set({obj, {"no_such_property"}}, value), std::runtime_error);

	commandInterface.deleteObjects({obj});
	EXPECT_THROW(commandInterface.set({obj, &Foo::v3f_}, value), std::runtime_error);
}

TEST_F(CommandInterfaceTest, set_vec4f_fail) {
	auto obj = create<Foo>("name");
	std::array<double, 4> value{1, 2, 3, 4};

	EXPECT_THROW(commandInterface.set({obj, &Foo::b_}, value), std::runtime_error);
	EXPECT_THROW(commandInterface.set({obj, {"no_such_property"}}, value), std::runtime_error);

	commandInterface.deleteObjects({obj});
	EXPECT_THROW(commandInterface.set({obj, &Foo::v4f_}, value), std::runtime_error);
}

TEST_F(CommandInterfaceTest, set_vec2i_fail) {
	auto obj = create<Foo>("name");
	std::array<int, 2> value{1, 2};

	EXPECT_THROW(commandInterface.set({obj, &Foo::b_}, value), std::runtime_error);
	EXPECT_THROW(commandInterface.set({obj, {"no_such_property"}}, value), std::runtime_error);

	commandInterface.deleteObjects({obj});
	EXPECT_THROW(commandInterface.set({obj, &Foo::v2i_}, value), std::runtime_error);
}

TEST_F(CommandInterfaceTest, set_vec3i_fail) {
	auto obj = create<Foo>("name");
	std::array<int, 3> value{1, 2, 3};

	EXPECT_THROW(commandInterface.set({obj, &Foo::b_}, value), std::runtime_error);
	EXPECT_THROW(commandInterface.set({obj, {"no_such_property"}}, value), std::runtime_error);

	commandInterface.deleteObjects({obj});
	EXPECT_THROW(commandInterface.set({obj, &Foo::v3i_}, value), std::runtime_error);
}

TEST_F(CommandInterfaceTest, set_vec4i_fail) {
	auto obj = create<Foo>("name");
	std::array<int, 4> value{1, 2, 3, 4};

	EXPECT_THROW(commandInterface.set({obj, &Foo::b_}, value), std::runtime_error);
	EXPECT_THROW(commandInterface.set({obj, {"no_such_property"}}, value), std::runtime_error);

	commandInterface.deleteObjects({obj});
	EXPECT_THROW(commandInterface.set({obj, &Foo::v4i_}, value), std::runtime_error);
}

TEST_F(CommandInterfaceTest, set_future_feature_level_fail) {
	auto obj = create<Foo>("name");

	EXPECT_THROW(commandInterface.set({obj, &Foo::future_}, 42.0), std::runtime_error);
}

TEST_F(CommandInterfaceTest, set_tags_fail) {
	auto obj = create<ObjectWithTableProperty>("name");
	auto foo = create<Foo>("name");
	std::vector<std::string> newTags{"abc", "def"};

	EXPECT_THROW(commandInterface.setTags({foo, &Foo::b_}, newTags), std::runtime_error);
	EXPECT_THROW(commandInterface.setTags({obj, {"no_such_property"}}, newTags), std::runtime_error);

	EXPECT_THROW(commandInterface.setTags({obj, &ObjectWithTableProperty::t_}, newTags), std::runtime_error);

	EXPECT_NO_THROW(commandInterface.setTags({obj, &ObjectWithTableProperty::tags_}, newTags));

	commandInterface.deleteObjects({obj});
	EXPECT_THROW(commandInterface.setTags({obj, &ObjectWithTableProperty::tags_}, newTags), std::runtime_error);
}

TEST_F(CommandInterfaceTest, set_renderable_tags_fail) {
	auto obj = create<ObjectWithTableProperty>("name");
	auto foo = create<Foo>("name");
	std::vector<std::pair<std::string, int>> newTags{{"abc", 5}, {"def", 3}};

	EXPECT_THROW(commandInterface.setRenderableTags({foo, &Foo::b_}, newTags), std::runtime_error);
	EXPECT_THROW(commandInterface.setRenderableTags({obj, {"no_such_property"}}, newTags), std::runtime_error);

	EXPECT_THROW(commandInterface.setRenderableTags({obj, &ObjectWithTableProperty::t_}, newTags), std::runtime_error);
	EXPECT_THROW(commandInterface.setRenderableTags({obj, &ObjectWithTableProperty::tags_}, newTags), std::runtime_error);
	EXPECT_NO_THROW(commandInterface.setRenderableTags({obj, &ObjectWithTableProperty::renderableTags_}, newTags));

	commandInterface.deleteObjects({obj});
	EXPECT_THROW(commandInterface.setRenderableTags({obj, &ObjectWithTableProperty::renderableTags_}, newTags), std::runtime_error);
}

TEST_F(CommandInterfaceTest, set_tags_fail_loop) {
	auto layer = create<RenderLayer>("layer");
	std::vector<std::string> tags{"abc"};
	std::vector<std::pair<std::string, int>> renderables{{"abc", 5}, {"def", 3}};

	commandInterface.setRenderableTags({layer, &RenderLayer::renderableTags_}, renderables);
	EXPECT_THROW(commandInterface.setTags({layer, &RenderLayer::tags_}, tags), std::runtime_error);
}

TEST_F(CommandInterfaceTest, set_usertags_renderlayer_no_loop) {
	auto layer = create<RenderLayer>("layer");
	std::vector<std::string> tags{"abc"};
	std::vector<std::pair<std::string, int>> renderables{{"abc", 5}, {"def", 3}};

	commandInterface.setRenderableTags({layer, &RenderLayer::renderableTags_}, renderables);
	commandInterface.setTags({layer, &RenderLayer::userTags_}, tags);
	EXPECT_EQ(layer->userTags_->asVector<std::string>(), tags);
}

TEST_F(CommandInterfaceTest, set_tags_fail_loop_multi_hop) {
	auto layer_1 = create<RenderLayer>("layer1");
	auto layer_2 = create<RenderLayer>("layer2");
	std::vector<std::string> tags_abc{"abc"};
	std::vector<std::string> tags_def{"def"};
	std::vector<std::pair<std::string, int>> renderables_abc{{"abc", 5}};
	std::vector<std::pair<std::string, int>> renderables_def{{"def", 3}};

	commandInterface.setRenderableTags({layer_1, &RenderLayer::renderableTags_}, renderables_def);
	commandInterface.setTags({layer_2, &RenderLayer::tags_}, tags_def);
	commandInterface.setRenderableTags({layer_2, &RenderLayer::renderableTags_}, renderables_abc);
	EXPECT_THROW(commandInterface.setTags({layer_1, &RenderLayer::tags_}, tags_abc), std::runtime_error);
}

TEST_F(CommandInterfaceTest, set_renderable_tags_fail_loop) {
	auto layer = create<RenderLayer>("layer");
	std::vector<std::string> tags{"abc"};
	std::vector<std::pair<std::string, int>> renderables{{"abc", 5}, {"def", 3}};

	commandInterface.setTags({layer, &RenderLayer::tags_}, tags);
	EXPECT_THROW(commandInterface.setRenderableTags({layer, &RenderLayer::renderableTags_}, renderables), std::runtime_error);
}

TEST_F(CommandInterfaceTest, set_renderable_tags_fail_loop_multi_hop) {
	auto layer_1 = create<RenderLayer>("layer1");
	auto layer_2 = create<RenderLayer>("layer2");
	std::vector<std::string> tags_abc{"abc"};
	std::vector<std::string> tags_def{"def"};
	std::vector<std::pair<std::string, int>> renderables_abc{{"abc", 5}};
	std::vector<std::pair<std::string, int>> renderables_def{{"def", 3}};

	commandInterface.setTags({layer_1, &RenderLayer::tags_}, tags_abc);
	commandInterface.setRenderableTags({layer_1, &RenderLayer::renderableTags_}, renderables_def);
	commandInterface.setTags({layer_2, &RenderLayer::tags_}, tags_def);
	EXPECT_THROW(commandInterface.setRenderableTags({layer_2, &RenderLayer::renderableTags_}, renderables_abc), std::runtime_error);
}

TEST_F(CommandInterfaceTest, set_ref_fail_prefab_loop) {
	auto prefab = create<Prefab>("prefab");
	auto inst = create<PrefabInstance>("inst", prefab);

	EXPECT_THROW(commandInterface.set({inst, &PrefabInstance::template_}, prefab), std::runtime_error);
}

TEST_F(CommandInterfaceTest, set_ref_fail_prefab_loop_nested) {
	auto prefab_1 = create<Prefab>("prefab 1");
	auto prefab_2 = create<Prefab>("prefab 2");
	auto inst_1 = create<PrefabInstance>("inst 1", prefab_1);
	auto inst_2 = create<PrefabInstance>("inst 2", prefab_2);

	commandInterface.set({inst_1, &PrefabInstance::template_}, prefab_2);
	EXPECT_THROW(commandInterface.set({inst_2, &PrefabInstance::template_}, prefab_1), std::runtime_error);
}

TEST_F(CommandInterfaceTest, set_ref_fail_ref_loop) {
	auto meshnode = create<Node>("meshnode");
	auto skin = create<Skin>("skin", meshnode);
	EXPECT_THROW(commandInterface.set(ValueHandle(skin, &Skin::targets_)[0], meshnode), std::runtime_error);
	EXPECT_THROW(commandInterface.set(ValueHandle(skin, &Skin::joints_)[0], meshnode), std::runtime_error);
}

TEST_F(CommandInterfaceTest, set_fail_read_only_prop_lua_output) {
	auto lua = create_lua("lua", "scripts/types-scalar.lua");

	EXPECT_THROW(commandInterface.set({lua, {"outputs", "ofloat"}}, 2.0), std::runtime_error);
}

TEST_F(CommandInterfaceTest, set_fail_read_only_prop_timer_output) {
	auto timer = create<Timer>("timer");
	
	EXPECT_THROW(commandInterface.set({timer, {"outputs", "ticker_us"}}, int64_t{0}), std::runtime_error);
}

TEST_F(CommandInterfaceTest, set_fail_linked) {
	auto node = create<Node>("node");
	auto lua = create_lua("lua", "scripts/types-scalar.lua");

	commandInterface.addLink({lua, {"outputs", "obool"}}, {node, &Node::visibility_});
	EXPECT_EQ(project.links().size(), 1);
	EXPECT_THROW(commandInterface.set({node, &Node::visibility_}, false), std::runtime_error);
}

TEST_F(CommandInterfaceTest, set_fail_parent_linked) {
	auto node = create<Node>("node");
	auto lua = create_lua("lua", "scripts/types-scalar.lua");

	commandInterface.addLink({lua, {"outputs", "ovector3f"}}, {node, &Node::translation_});
	EXPECT_EQ(project.links().size(), 1);
	EXPECT_THROW(commandInterface.set({node, {"translation", "x"}}, 2.0), std::runtime_error);
}

TEST_F(CommandInterfaceTest, set_fail_prefab_instance_child) {
	auto prefab = create<Prefab>("prefab");
	auto node = create<Node>("node", prefab);

	auto inst = create_prefabInstance("inst", prefab);
	auto inst_node = inst->children_->get(0)->asRef();

	EXPECT_THROW(commandInterface.set({inst_node, &Node::visibility_}, false), std::runtime_error);
}

TEST_F(CommandInterfaceTest, set_int_fail_invalid_enum) {
	auto material = create<Material>("mat");

	EXPECT_EQ(material->options_->get("cullmode")->asInt(), 2);
	commandInterface.set({material, {"options", "cullmode"}}, 1);
	EXPECT_EQ(material->options_->get("cullmode")->asInt(), 1);
	EXPECT_THROW(commandInterface.set({material, {"options", "cullmode"}}, 42), std::runtime_error);
}

TEST_F(CommandInterfaceTest, set_multi_int_fail_invalid_enum) {
	auto material = create<Material>("mat");

	EXPECT_EQ(material->options_->get("cullmode")->asInt(), 2);
	commandInterface.set(std::set<ValueHandle>({ValueHandle(material, {"options", "cullmode"})}), 1);
	EXPECT_EQ(material->options_->get("cullmode")->asInt(), 1);

	EXPECT_THROW(commandInterface.set(std::set<ValueHandle>({ValueHandle(material, {"options", "cullmode"})}), 42), std::runtime_error);
}

TEST_F(CommandInterfaceTest, set_int_fail_read_only) {
	auto obj = create<Foo>("name");
		
	EXPECT_THROW(commandInterface.set({obj, {"readOnly"}}, 27), std::runtime_error);
}

TEST_F(CommandInterfaceTest, set_multi_int_fail_read_only) {
	auto obj = create<Foo>("name");

	EXPECT_THROW(commandInterface.set(std::set<ValueHandle>({ValueHandle(obj, {"readOnly"})}), 27), std::runtime_error);
}

TEST_F(CommandInterfaceTest, move_scenegraph_fail_prefab_loop) {
	auto prefab = create<Prefab>("prefab");
	auto inst = create<PrefabInstance>("inst");

	commandInterface.set({inst, &PrefabInstance::template_}, prefab);
	EXPECT_EQ(commandInterface.moveScenegraphChildren({inst}, prefab), 0);

	EXPECT_TRUE(inst->getParent() == nullptr);
	EXPECT_EQ(prefab->children_->size(), 0);
}

TEST_F(CommandInterfaceTest, move_scenegraph_fail_prefab_loop_nested) {
	auto prefab_1 = create<Prefab>("prefab 1");
	auto prefab_2 = create<Prefab>("prefab 2");
	auto inst_1 = create<PrefabInstance>("inst 1", prefab_1);
	auto inst_2 = create<PrefabInstance>("inst 2");

	commandInterface.set({inst_1, &PrefabInstance::template_}, prefab_2);
	commandInterface.set({inst_2, &PrefabInstance::template_}, prefab_1);
	
	EXPECT_EQ(commandInterface.moveScenegraphChildren({inst_2}, prefab_2), 0);

	EXPECT_TRUE(inst_2->getParent() == nullptr);
	EXPECT_EQ(prefab_2->children_->size(), 0);
}

TEST_F(CommandInterfaceTest, create_object_fail) {
	EXPECT_THROW(commandInterface.createObject(ProjectSettings::typeDescription.typeName, "name"),
		std::runtime_error);

	EXPECT_THROW(commandInterface.createObject("UnknownType", "name"),
		std::runtime_error);
}

TEST_F(CommandInterfaceTest, create_object_fail_invalid_parent) {
	auto node = create<Node>("node");
	commandInterface.deleteObjects({node});

	EXPECT_THROW(commandInterface.createObject("Node", "new_node", node), std::runtime_error);
}

TEST_F(CommandInterfaceTest, create_object_fail_future_type) {
	EXPECT_THROW(commandInterface.createObject("FutureType", "future"), std::runtime_error);
}

TEST_F(CommandInterfaceTest, double_delete_fail) {
	auto node = create<Node>("node");
	EXPECT_NO_THROW(commandInterface.deleteObjects({node}));
	EXPECT_THROW(commandInterface.deleteObjects({node}), std::runtime_error);
}

TEST_F(CommandInterfaceTest, move_to_self_fail) {
	auto node = create<Node>("node");

	EXPECT_EQ(commandInterface.moveScenegraphChildren({node}, node, -1), 0);
	EXPECT_TRUE(node->getParent() == nullptr);
	EXPECT_EQ(node->children_->size(), 0);
}

TEST_F(CommandInterfaceTest, move_to_child_fail) {
	auto node = create<Node>("node");
	auto child = create<Node>("child", node);

	EXPECT_EQ(commandInterface.moveScenegraphChildren({node}, child, -1), 0);
	EXPECT_TRUE(node->getParent() == nullptr);
	EXPECT_EQ(child->children_->size(), 0);
}

TEST_F(CommandInterfaceTest, move_fail_newparent_deleted) {
	auto node1 = create<Node>("node1");
	auto node2 = create<Node>("node2");

	commandInterface.deleteObjects({node1});

	EXPECT_THROW(commandInterface.moveScenegraphChildren({node1}, node2, -1), std::runtime_error);
}

TEST_F(CommandInterfaceTest, move_fail_object_deleted) {
	auto node1 = create<Node>("node1");
	auto node2 = create<Node>("node2");

	commandInterface.deleteObjects({node2});

	EXPECT_THROW(commandInterface.moveScenegraphChildren({node1}, node2, -1), std::runtime_error);
}

TEST_F(CommandInterfaceTest, move_fail_insertion_index_too_small) {
	auto node1 = create<Node>("node1");
	auto node2 = create<Node>("node2");

	EXPECT_THROW(commandInterface.moveScenegraphChildren({node1}, node2, -2), std::runtime_error);
}

TEST_F(CommandInterfaceTest, move_ok_insertion_index_end) {
	auto node1 = create<Node>("node1");
	auto node2 = create<Node>("node2");
	auto parent = create<Node>("parent");

	EXPECT_EQ(commandInterface.moveScenegraphChildren({node1}, parent, -1), 1);
	EXPECT_EQ(commandInterface.moveScenegraphChildren({node1}, nullptr), 1);
	EXPECT_EQ(commandInterface.moveScenegraphChildren({node1}, parent, 0), 1);
	EXPECT_EQ(commandInterface.moveScenegraphChildren({node2}, parent, 1), 1);
}

TEST_F(CommandInterfaceTest, move_fail_insertion_index_too_big) {
	auto node1 = create<Node>("node1");
	auto node2 = create<Node>("node2");

	EXPECT_THROW(commandInterface.moveScenegraphChildren({node1}, node2, 1), std::runtime_error);
}

TEST_F(CommandInterfaceTest, move_fail_insertion_index_invalid_to_root) {
	auto settings = project.settings();
	auto node1 = create<Node>("node1");
	
	ASSERT_EQ(project.instances(), std::vector<SEditorObject>({settings, node1}));

	EXPECT_EQ(commandInterface.moveScenegraphChildren({node1}, nullptr, 0), 1);
	EXPECT_EQ(commandInterface.moveScenegraphChildren({node1}, nullptr, 2), 1);
	EXPECT_THROW(commandInterface.moveScenegraphChildren({node1}, nullptr, 3), std::runtime_error);
}

TEST_F(CommandInterfaceTest, addLink_fail_no_start) {
	auto node = create<Node>("node");
	auto lua = create_lua("lua", "scripts/types-scalar.lua");

	commandInterface.deleteObjects({lua});
	EXPECT_THROW(commandInterface.addLink({lua, {"outputs", "ovector3f"}}, {node, {"translation"}}), std::runtime_error);
}

TEST_F(CommandInterfaceTest, addLink_fail_no_end) {
	auto node = create<Node>("node");
	auto lua = create_lua("lua", "scripts/types-scalar.lua");

	commandInterface.deleteObjects({node});
	EXPECT_THROW(commandInterface.addLink({lua, {"outputs", "ovector3f"}}, {node, {"translation"}}), std::runtime_error);
}

TEST_F(CommandInterfaceTest, addLink_fail_loop) {
	auto lua = create_lua("lua", "scripts/types-scalar.lua");
	EXPECT_THROW(commandInterface.addLink({lua, {"outputs", "ofloat"}}, {lua, {"inputs", "float"}}), std::runtime_error);
	EXPECT_THROW(commandInterface.addLink({lua, {"outputs", "ofloat"}}, {lua, {"inputs", "float"}}, true), std::runtime_error);
}

TEST_F(CommandInterfaceTest, addLink_fail_end_object_readonly) {
	auto lua = create_lua("lua", "scripts/types-scalar.lua");
	auto prefab = create<Prefab>("prefab");
	auto node = create<Node>("node", prefab);

	auto inst = create_prefabInstance("inst", prefab);
	auto inst_node = inst->children_->get(0)->asRef();

	EXPECT_THROW(commandInterface.addLink({lua, {"outputs", "ovector3f"}}, {inst_node, &Node::translation_}), std::runtime_error);
}

TEST_F(CommandInterfaceTest, addLink_fail_lua_output_as_end) {
	auto lua_start = create_lua("start", "scripts/types-scalar.lua");
	auto lua_end = create_lua("end", "scripts/types-scalar.lua");

	EXPECT_THROW(commandInterface.addLink({lua_start, {"outputs", "ovector3f"}}, {lua_end, {"outputs", "ovector3f"}}), std::runtime_error);
}

TEST_F(CommandInterfaceTest, addLink_fail_parent_prop_linked) {
	auto lua_start = create_lua("start", "scripts/struct-simple.lua");
	auto lua_end = create_lua("end", "scripts/struct-simple.lua");

	commandInterface.addLink({lua_start, {"outputs", "s"}}, {lua_end, {"inputs", "s"}});
	EXPECT_THROW(commandInterface.addLink({lua_start, {"outputs", "s", "float"}}, {lua_end, {"inputs", "s", "float"}}), std::runtime_error);
}

TEST_F(CommandInterfaceTest, addLink_fail_global_to_prefab) {
	auto lua_global = create_lua("lua", "scripts/types-scalar.lua");
	auto prefab = create<Prefab>("prefab");
	auto lua_prefab = create_lua("lua", "scripts/types-scalar.lua", prefab);

	EXPECT_THROW(commandInterface.addLink({lua_global, {"outputs", "ofloat"}}, {lua_prefab, {"inputs", "float"}}), std::runtime_error);
}

TEST_F(CommandInterfaceTest, addLink_fail_end_interface_prop) {
	auto prefab = create<Prefab>("prefab");
	auto lua = create_lua("lua", "scripts/types-scalar.lua", prefab);
	auto intf = create_lua_interface("interface", "scripts/interface-scalar-types.lua", prefab);

	EXPECT_THROW(commandInterface.addLink({lua, {"outputs", "ofloat"}}, {intf, {"inputs", "float"}}), std::runtime_error);
	EXPECT_NO_THROW(commandInterface.addLink({intf, {"inputs", "float"}}, {lua, {"inputs", "float"}}));
}

TEST_F(CommandInterfaceTest, addLink_fail_future_linkable) {
	auto start = create<Foo>("start");
	auto end = create<Foo>("end");

	EXPECT_THROW(commandInterface.addLink({start, &Foo::x_}, {end, &Foo::futureLinkable_}), std::runtime_error);
}


TEST_F(CommandInterfaceTest, removeLink_fail_no_end) {
	auto node = create<Node>("node");
	auto lua = create_lua("lua", "scripts/types-scalar.lua");

	auto [sprop, eprop] = link(lua, {"outputs", "ovector3f"}, node, {"translation"});
	commandInterface.deleteObjects({node});
	EXPECT_THROW(commandInterface.removeLink(eprop), std::runtime_error);
}

TEST_F(CommandInterfaceTest, removeLink_fail_end_object_readonly) {
	auto prefab = create<Prefab>("prefab");
	auto lua = create_lua("lua", "scripts/types-scalar.lua", prefab);
	auto node = create<Node>("node", prefab);

	auto inst = create_prefabInstance("inst", prefab);
	auto inst_node = raco::select<raco::user_types::Node>(inst->children_->asVector<SEditorObject>(), "node");
	auto inst_lua = raco::select<raco::user_types::Node>(inst->children_->asVector<SEditorObject>(), "lua");

	auto [sprop, eprop] = link(lua, {"outputs", "ovector3f"}, node, {"translation"});
	EXPECT_THROW(commandInterface.removeLink({inst_node, {"translation"}}), std::runtime_error);
}


TEST_F(CommandInterfaceTest, copy_objects_fail_deleted) {
	auto node = create<Node>("node");
	commandInterface.deleteObjects({node});
	EXPECT_THROW(commandInterface.copyObjects({node}), std::runtime_error);
}

TEST_F(CommandInterfaceTest, cut_objects_fail_deleted) {
	auto node = create<Node>("node");
	commandInterface.deleteObjects({node});
	EXPECT_THROW(commandInterface.cutObjects({node}), std::runtime_error);
}

TEST_F(CommandInterfaceTest, duplicate_objects_fail_deleted) {
	auto node = create<Node>("node");
	commandInterface.deleteObjects({node});
	EXPECT_THROW(commandInterface.duplicateObjects({node}), std::runtime_error);
}

TEST_F(CommandInterfaceTest, duplicate_objects_fail_invalid) {
	auto node = create<Node>("node");
	auto child = create<Node>("child", node);
	EXPECT_THROW(commandInterface.duplicateObjects({node, child}), std::runtime_error);
}

TEST_F(CommandInterfaceTest, paste_objects_fail_deleted_target) {
	auto node = create<Node>("node");
	auto data = commandInterface.copyObjects({node});
	commandInterface.deleteObjects({node});
	EXPECT_THROW(commandInterface.pasteObjects(data, node), std::runtime_error);
}

TEST_F(CommandInterfaceTest, paste_objects_fail_invalid_target) {
	auto node = create<Node>("node");
	auto lua = create<LuaScript>("lua");
	auto data = commandInterface.copyObjects({node});

	EXPECT_THROW(commandInterface.pasteObjects(data, lua), std::runtime_error);
}

TEST_F(CommandInterfaceTest, paste_no_json) {
	EXPECT_THROW(commandInterface.pasteObjects("no json data", nullptr), std::runtime_error);
}

TEST_F(CommandInterfaceTest, paste_invalid_json) {
	auto node = create<Node>("node");

	std::string data = R"___(
{
    "key": {
    },
    "anotherKey": 42
}
)___";

	EXPECT_THROW(commandInterface.pasteObjects(data, nullptr), std::runtime_error);
}