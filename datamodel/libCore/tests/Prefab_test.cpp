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
#include "core/MeshCacheInterface.h"
#include "core/PrefabOperations.h"
#include "core/Project.h"
#include "core/Queries.h"
#include "application/RaCoApplication.h"
#include "ramses_base/HeadlessEngineBackend.h"
#include "testing/RacoBaseTest.h"
#include "testing/TestEnvironmentCore.h"
#include "testing/TestUtil.h"
#include "user_types/UserObjectFactory.h"
#include "ramses_adaptor/SceneBackend.h"
#include "ramses_base/BaseEngineBackend.h"

#include "user_types/LuaScriptModule.h"
#include "user_types/Mesh.h"
#include "user_types/MeshNode.h"
#include "user_types/Node.h"
#include "user_types/Prefab.h"
#include "user_types/Texture.h"

#include "gtest/gtest.h"

using namespace raco::core;
using namespace raco::user_types;


class PrefabTest : public TestEnvironmentCore {
public:
};

TEST_F(PrefabTest, check_id_recreate_same_id) {
	auto prefab = create<Prefab>("prefab");
	auto node = create<Node>("node", prefab);
	auto inst = create_prefabInstance("inst", prefab);
	
	EXPECT_EQ(inst->children_->size(), 1);
	auto inst_node_1 = inst->children_->asVector<SEditorObject>()[0];

	commandInterface.set({inst, {"template"}}, SEditorObject{});
	EXPECT_EQ(inst->children_->size(), 0);

	commandInterface.set({inst, {"template"}}, prefab);
	EXPECT_EQ(inst->children_->size(), 1);
	auto inst_node_2 = inst->children_->asVector<SEditorObject>()[0];

	EXPECT_FALSE(inst_node_1 == inst_node_2);
	EXPECT_EQ(inst_node_1->objectID(), inst_node_2->objectID());
}

TEST_F(PrefabTest, check_id_different_inst) {
	auto prefab = create<Prefab>("prefab");
	auto node = create<Node>("node", prefab);
	auto inst_1 = create_prefabInstance("inst", prefab);
	auto inst_2 = create_prefabInstance("inst", prefab);
	
	EXPECT_EQ(inst_1->children_->size(), 1);
	auto inst_node_1 = inst_1->children_->asVector<SEditorObject>()[0];

	EXPECT_EQ(inst_2->children_->size(), 1);
	auto inst_node_2 = inst_2->children_->asVector<SEditorObject>()[0];

	EXPECT_NE(inst_node_1->objectID(), inst_node_2->objectID());
}

TEST_F(PrefabTest, check_id_nesting) {
	//           prefab_2  inst_2
	//  prefab   inst_1    inst_3
	//  node     node_1    node_2

	auto prefab = create<Prefab>("prefab");
	auto node = create<Node>("node", prefab);
	
	auto prefab_2 = create<Prefab>("prefab2");
	auto inst_1 = create_prefabInstance("inst", prefab, prefab_2);

	EXPECT_EQ(inst_1->children_->size(), 1);
	auto node_1 = inst_1->children_->asVector<SEditorObject>()[0];

	auto inst_2 = create_prefabInstance("inst", prefab_2);
	EXPECT_EQ(inst_2->children_->size(), 1);
	auto inst_3 = inst_2->children_->asVector<SEditorObject>()[0];
	EXPECT_EQ(inst_3->children_->size(), 1);
	auto node_2 = inst_3->children_->asVector<SEditorObject>()[0];

	EXPECT_EQ(node_1->objectID(), EditorObject::XorObjectIDs(node->objectID(), inst_1->objectID()));

	EXPECT_EQ(inst_3->objectID(), EditorObject::XorObjectIDs(inst_1->objectID(), inst_2->objectID()));

	EXPECT_EQ(node_2->objectID(), EditorObject::XorObjectIDs(node_1->objectID(), inst_2->objectID()));
	EXPECT_EQ(node_2->objectID(), EditorObject::XorObjectIDs(node->objectID(), inst_3->objectID()));
}

TEST_F(PrefabTest, check_id_copy_paste) {
	auto prefab = create<Prefab>("prefab");
	auto lua = create_lua("lua", "scripts/types-scalar.lua", prefab);

	auto inst = create_prefabInstance("inst", prefab);
	auto inst_lua = raco::select<LuaScript>(inst->children_->asVector<SEditorObject>());

	commandInterface.set({lua, {"luaInputs", "float"}}, 2.0);
	commandInterface.set({inst_lua, {"luaInputs", "float"}}, 3.0);

	auto pasted = commandInterface.pasteObjects(commandInterface.copyObjects({inst}));
	auto inst_2 = raco::select<PrefabInstance>(pasted);
	auto inst_2_lua = inst_2->children_->get(0)->asRef();
	ASSERT_TRUE(inst_2 != nullptr);
	ASSERT_TRUE(inst_2_lua != nullptr);

	EXPECT_TRUE(ValueHandle(inst_2_lua, {"luaInputs"}).hasProperty("float"));
	EXPECT_EQ(ValueHandle(inst_2_lua, {"luaInputs"}).get("float").asDouble(), 3.0);
	EXPECT_EQ(inst_2_lua->objectID(), EditorObject::XorObjectIDs(lua->objectID(), inst_2->objectID()));
}

TEST_F(PrefabTest, check_id_copy_paste_nesting) {
	//           prefab_2  inst_2
	//  prefab   inst_1    inst_3
	//  lua      lua_1     lua_2

	auto prefab = create<Prefab>("prefab");
	auto lua = create_lua("lua", "scripts/types-scalar.lua", prefab);
	commandInterface.set({lua, {"luaInputs", "float"}}, 2.0);
	
	auto prefab_2 = create<Prefab>("prefab2");
	auto inst_1 = create_prefabInstance("inst", prefab, prefab_2);

	EXPECT_EQ(inst_1->children_->size(), 1);
	auto lua_1 = inst_1->children_->asVector<SEditorObject>()[0];
	commandInterface.set({lua_1, {"luaInputs", "float"}}, 3.0);

	auto inst_2 = create_prefabInstance("inst", prefab_2);
	EXPECT_EQ(inst_2->children_->size(), 1);
	auto inst_3 = inst_2->children_->asVector<SEditorObject>()[0];
	EXPECT_EQ(inst_3->children_->size(), 1);
	auto lua_2 = inst_3->children_->asVector<SEditorObject>()[0];
	commandInterface.set({lua_2, {"luaInputs", "float"}}, 4.0);

	EXPECT_EQ(lua_1->objectID(), EditorObject::XorObjectIDs(lua->objectID(), inst_1->objectID()));
	EXPECT_EQ(inst_3->objectID(), EditorObject::XorObjectIDs(inst_1->objectID(), inst_2->objectID()));
	EXPECT_EQ(lua_2->objectID(), EditorObject::XorObjectIDs(lua_1->objectID(), inst_2->objectID()));
	EXPECT_EQ(lua_2->objectID(), EditorObject::XorObjectIDs(lua->objectID(), inst_3->objectID()));

	auto pasted = commandInterface.pasteObjects(commandInterface.copyObjects({inst_2}));
	auto inst_2_copy = raco::select<PrefabInstance>(pasted);
	EXPECT_EQ(inst_2_copy->children_->size(), 1);
	auto inst_3_copy = inst_2_copy->children_->asVector<SEditorObject>()[0];
	EXPECT_EQ(inst_3_copy->children_->size(), 1);
	auto lua_2_copy = inst_3_copy->children_->asVector<SEditorObject>()[0];

	EXPECT_EQ(ValueHandle(lua_2_copy, {"luaInputs"}).get("float").asDouble(), 4.0);
	EXPECT_EQ(inst_3_copy->objectID(), EditorObject::XorObjectIDs(inst_1->objectID(), inst_2_copy->objectID()));
	EXPECT_EQ(lua_2_copy->objectID(), EditorObject::XorObjectIDs(lua_1->objectID(), inst_2_copy->objectID()));
	EXPECT_EQ(lua_2_copy->objectID(), EditorObject::XorObjectIDs(lua->objectID(), inst_3_copy->objectID()));
}

TEST_F(PrefabTest, move_node_in) {
	auto prefab = create<Prefab>("prefab");
	auto inst = create<PrefabInstance>("inst");
	auto node = create<Node>("node");
	commandInterface.set({inst, {"template"}}, prefab);

	checkUndoRedo([this, node, prefab]() { commandInterface.moveScenegraphChildren({node}, prefab); },
		[this, inst]() {
			EXPECT_EQ(inst->children_->size(), 0);
		},
		[this, inst]() {
			EXPECT_EQ(inst->children_->size(), 1);
		});
}

TEST_F(PrefabTest, move_node_out) {
	auto prefab = create<Prefab>("prefab");
	auto inst = create<PrefabInstance>("inst");
	auto node = create<Node>("node");
	commandInterface.set({inst, {"template"}}, prefab);
	commandInterface.moveScenegraphChildren({node}, prefab);

	checkUndoRedo([this, node, prefab]() { commandInterface.moveScenegraphChildren({node}, nullptr); },
		[this, inst]() {
			EXPECT_EQ(inst->children_->size(), 1);
		},
		[this, inst]() {
			EXPECT_EQ(inst->children_->size(), 0);
		});
}

TEST_F(PrefabTest, del_node) {
	auto prefab = create<Prefab>("prefab");
	auto inst = create<PrefabInstance>("inst");
	auto node = create<Node>("node");
	commandInterface.set({inst, {"template"}}, prefab);
	commandInterface.moveScenegraphChildren({node}, prefab);

	checkUndoRedo([this, node, prefab]() { commandInterface.deleteObjects({node}); },
		[this, inst]() {
			EXPECT_EQ(inst->children_->size(), 1);
		},
		[this, inst]() {
			EXPECT_EQ(inst->children_->size(), 0);
		});
}

TEST_F(PrefabTest, set_node_prop) {
	auto prefab = create<Prefab>("prefab");
	auto inst = create<PrefabInstance>("inst");
	auto node = create<Node>("node");
	commandInterface.set({inst, {"template"}}, prefab);
	commandInterface.moveScenegraphChildren({node}, prefab);

	auto instChildren = inst->children_->asVector<SEditorObject>();
	EXPECT_EQ(instChildren.size(), 1);
	auto instNode = instChildren[0]->as<Node>();

	checkUndoRedoMultiStep<2>(
		{[this, node]() { commandInterface.set({node, {"visible"}}, false); },
			[this, node]() { commandInterface.set({node, {"translation", "x"}}, 23.0); }},
		{[this, instNode]() {
			 EXPECT_EQ(*instNode->visible_, true);
			 EXPECT_EQ(*instNode->translation_->x, 0.0);
		 },
			[this, instNode]() {
				EXPECT_EQ(*instNode->visible_, false);
				EXPECT_EQ(*instNode->translation_->x, 0.0);
			},
			[this, instNode]() {
				EXPECT_EQ(*instNode->visible_, false);
				EXPECT_EQ(*instNode->translation_->x, 23.0);
			}});
}

TEST_F(PrefabTest, link_simple) {
	auto prefab = create<Prefab>("prefab");
	auto inst = create<PrefabInstance>("inst");
	commandInterface.set({inst, {"template"}}, prefab);
	auto node = create<Node>("node", prefab);
	auto lua = create<LuaScript>("lua", prefab);

	TextFile scriptFile = makeFile("script.lua", R"(
function interface()
	IN.v = VEC3F
	OUT.v = VEC3F
end
function run()
end
)");
	commandInterface.set({lua, {"uri"}}, scriptFile);
	EXPECT_EQ(inst->children_->size(), 2);
	auto inst_node = raco::select<Node>(inst->children_->asVector<SEditorObject>());
	auto inst_lua = raco::select<LuaScript>(inst->children_->asVector<SEditorObject>());
	EXPECT_TRUE(inst_node);
	EXPECT_TRUE(inst_lua);

	checkUndoRedoMultiStep<2>(
		{[this, lua, node]() {
			 commandInterface.addLink({lua, {"luaOutputs", "v"}}, {node, {"translation"}});
		 },
			[this, lua, node]() {
				commandInterface.addLink({lua, {"luaOutputs", "v"}}, {node, {"rotation"}});
			}},
		{[this]() {
			 EXPECT_EQ(project.links().size(), 0);
		 },
			[this, lua, node, inst_lua, inst_node]() {
				std::vector<Link> refLinks{{{{lua, {"luaOutputs", "v"}}, {node, {"translation"}}},
					{{inst_lua, {"luaOutputs", "v"}}, {inst_node, {"translation"}}}}};
				checkLinks(refLinks);
			},
			[this, lua, node, inst_lua, inst_node]() {
				std::vector<Link> refLinks{{{{lua, {"luaOutputs", "v"}}, {node, {"translation"}}},
					{{lua, {"luaOutputs", "v"}}, {node, {"rotation"}}},
					{{inst_lua, {"luaOutputs", "v"}}, {inst_node, {"translation"}}},
					{{inst_lua, {"luaOutputs", "v"}}, {inst_node, {"rotation"}}}}};
				checkLinks(refLinks);
			}
			});
}

TEST_F(PrefabTest, link_broken_inside_prefab_status_gets_propagated_to_instances) {
	auto prefab = create<Prefab>("prefab");
	auto inst = create<PrefabInstance>("inst");
	commandInterface.set({inst, {"template"}}, prefab);

	auto node = create<Node>("node");
	commandInterface.moveScenegraphChildren({node}, prefab);

	auto luaPrefabGlobal = create<LuaScript>("luaPrefab");
	commandInterface.moveScenegraphChildren({luaPrefabGlobal}, prefab);

	auto luaPrefabNodeChild = create<LuaScript>("luaPrefabNode");
	commandInterface.moveScenegraphChildren({luaPrefabNodeChild}, node);

	commandInterface.set({luaPrefabGlobal, {"uri"}}, test_path().append("scripts/types-scalar.lua").string());
	commandInterface.set({luaPrefabNodeChild, {"uri"}}, test_path().append("scripts/SimpleScript.lua").string());

	commandInterface.addLink({luaPrefabGlobal, {"luaOutputs", "ovector3f"}} , {node, {"translation"}});
	commandInterface.addLink({luaPrefabNodeChild, {"luaOutputs", "out_float"}}, {luaPrefabGlobal, {"luaInputs", "float"}});
	auto inst_node = raco::select<Node>(inst->children_->asVector<SEditorObject>());
	auto inst_lua_prefab = raco::select<LuaScript>(inst->children_->asVector<SEditorObject>());
	std::vector<Link> refLinks{{{{luaPrefabGlobal, {"luaOutputs", "ovector3f"}}, {node, {"translation"}}},
		{{luaPrefabNodeChild, {"luaOutputs", "out_float"}}, {luaPrefabGlobal, {"luaInputs", "float"}}},
		{{inst_lua_prefab, {"luaOutputs", "ovector3f"}}, {inst_node, {"translation"}}}}};
	checkLinks(refLinks);

	commandInterface.set({luaPrefabNodeChild, {"uri"}}, test_path().append("scripts/types-scalar.lua").string());
	commandInterface.set({luaPrefabGlobal, {"uri"}}, test_path().append("scripts/SimpleScript.lua").string());
	ASSERT_EQ(project.links().size(), 3);
	ASSERT_FALSE(std::all_of(project.links().begin(), project.links().end(), [](const auto link) { return link->isValid(); }));

	commandInterface.set({luaPrefabGlobal, {"uri"}}, test_path().append("scripts/types-scalar.lua").string());
	commandInterface.set({luaPrefabNodeChild, {"uri"}}, test_path().append("scripts/SimpleScript.lua").string());
	checkLinks(refLinks);
}

TEST_F(PrefabTest, link_lua_node_delete_lua_in_prefab) {
	auto prefab = create<Prefab>("prefab");
	auto inst = create<PrefabInstance>("inst");
	commandInterface.set({inst, {"template"}}, prefab);
	auto lua_global = create<LuaScript>("global");
	auto node = create<Node>("node", prefab);
	auto lua = create<LuaScript>("lua", prefab);

	TextFile scriptFile = makeFile("script.lua", R"(
function interface()
	IN.v = VEC3F
	OUT.v = VEC3F
end
function run()
end
)");
	commandInterface.set({lua, {"uri"}}, scriptFile);
	commandInterface.set({lua_global, {"uri"}}, scriptFile);
	EXPECT_EQ(inst->children_->size(), 2);
	auto inst_node = raco::select<Node>(inst->children_->asVector<SEditorObject>());
	auto inst_lua = raco::select<LuaScript>(inst->children_->asVector<SEditorObject>());
	EXPECT_TRUE(inst_node);
	EXPECT_TRUE(inst_lua);

	commandInterface.addLink({lua, {"luaOutputs", "v"}}, {node, {"translation"}});
	commandInterface.addLink({lua_global, {"luaOutputs", "v"}}, {inst_lua, {"luaInputs", "v"}});

	ASSERT_EQ(project.links().size(), 3);

	commandInterface.deleteObjects({lua});

	ASSERT_EQ(project.links().size(), 0);
}

TEST_F(PrefabTest, link_lua_lua_delete_lua_in_prefab) {
	auto prefab = create<Prefab>("prefab");
	auto lua_start = create<LuaScript>("lua_start", prefab);
	auto lua_end = create<LuaScript>("lua_end", prefab);

	TextFile scriptFile = makeFile("script.lua", R"(
function interface()
	IN.v = VEC3F
	OUT.v = VEC3F
end
function run()
end
)");
	commandInterface.set({lua_start, {"uri"}}, scriptFile);
	commandInterface.set({lua_end, {"uri"}}, scriptFile);

	commandInterface.addLink({lua_start, {"luaOutputs", "v"}}, {lua_end, {"luaInputs", "v"}});

	auto inst = create<PrefabInstance>("inst");
	commandInterface.set({inst, {"template"}}, prefab);

	ASSERT_EQ(project.links().size(), 2);

	commandInterface.deleteObjects({lua_start});

	ASSERT_EQ(project.links().size(), 0);
}


TEST_F(PrefabTest, link_lua_quaternion_in_prefab) {
	auto prefab = create<Prefab>("prefab");
	auto inst = create<PrefabInstance>("inst");
	commandInterface.set({inst, {"template"}}, prefab);
	auto lua_global = create<LuaScript>("global");
	auto node = create<Node>("node", prefab);
	auto lua = create<LuaScript>("lua", prefab);

	TextFile scriptFile = makeFile("script.lua", R"(
function interface()
	IN.v = VEC4F
	OUT.v = VEC4F
end
function run()
end
)");
	commandInterface.set({lua, {"uri"}}, scriptFile);
	commandInterface.set({lua_global, {"uri"}}, scriptFile);

	commandInterface.addLink({lua, {"luaOutputs", "v"}}, {node, {"rotation"}});

	ASSERT_EQ(project.links().size(), 2);

	commandInterface.deleteObjects({lua});

	ASSERT_EQ(project.links().size(), 0);
}


TEST_F(PrefabTest, nesting_move_node_in) {
	auto prefab1 = create<Prefab>("prefab 1");
	auto prefab2 = create<Prefab>("prefab 2");

	auto inst1 = create<PrefabInstance>("inst 1");
	commandInterface.set({inst1, {"template"}}, prefab1);
	commandInterface.moveScenegraphChildren({inst1}, prefab2);

	auto inst2 = create<PrefabInstance>("inst 2");
	commandInterface.set({inst2, {"template"}}, prefab2);

	auto inst_2_children = inst2->children_->asVector<SEditorObject>();
	EXPECT_EQ(inst_2_children.size(), 1);
	auto inst_2_child = inst_2_children[0]->as<PrefabInstance>();
	EXPECT_TRUE(inst_2_child);

	auto node = create<Node>("node");

	checkUndoRedo([this, node, prefab1]() { commandInterface.moveScenegraphChildren({node}, prefab1); },
		[this, inst_2_child]() {
			EXPECT_EQ(inst_2_child->children_->size(), 0);
		},
		[this, inst_2_child]() {
			EXPECT_EQ(inst_2_child->children_->size(), 1);
			EXPECT_TRUE(inst_2_child->children_->asVector<SEditorObject>()[0]->as<Node>());
		});
}

TEST_F(PrefabTest, nesting_set_node_prop) {
	auto prefab1 = create<Prefab>("prefab 1");
	auto prefab2 = create<Prefab>("prefab 2");

	auto inst1 = create<PrefabInstance>("inst 1");
	commandInterface.set({inst1, {"template"}}, prefab1);
	commandInterface.moveScenegraphChildren({inst1}, prefab2);

	auto inst2 = create<PrefabInstance>("inst 2");
	commandInterface.set({inst2, {"template"}}, prefab2);

	auto inst_2_children = inst2->children_->asVector<SEditorObject>();
	EXPECT_EQ(inst_2_children.size(), 1);
	auto inst_2_child = inst_2_children[0]->as<PrefabInstance>();
	EXPECT_TRUE(inst_2_child);

	auto node = create<Node>("node");
	commandInterface.moveScenegraphChildren({node}, prefab1);

	EXPECT_EQ(inst_2_child->children_->size(), 1);
	auto instNode = inst_2_child->children_->asVector<SEditorObject>()[0]->as<Node>();
	EXPECT_TRUE(instNode);

	checkUndoRedoMultiStep<2>(
		{[this, node]() { commandInterface.set({node, {"visible"}}, false); },
			[this, node]() { commandInterface.set({node, {"translation", "x"}}, 23.0); }},
		{[this, instNode]() {
			 EXPECT_EQ(*instNode->visible_, true);
			 EXPECT_EQ(*instNode->translation_->x, 0.0);
		 },
			[this, instNode]() {
				EXPECT_EQ(*instNode->visible_, false);
				EXPECT_EQ(*instNode->translation_->x, 0.0);
			},
			[this, instNode]() {
				EXPECT_EQ(*instNode->visible_, false);
				EXPECT_EQ(*instNode->translation_->x, 23.0);
			}});
}


TEST_F(PrefabTest, delete_prefab_w_link_w_nested_instances) {
	auto prefab_outer = create<Prefab>("prefab_outer");
	auto node = create<Node>("node", prefab_outer);
	auto lua = create<LuaScript>("lua", prefab_outer);
	TextFile scriptFile = makeFile("script.lua", R"(
function interface()
	IN.v = VEC3F
	OUT.v = VEC3F
end
function run()
end
)");
	commandInterface.set({lua, {"uri"}}, scriptFile);
	commandInterface.addLink({lua, {"luaOutputs", "v"}}, {node, {"translation"}});

	auto prefab_inner = create<Prefab>("prefab_inner");
	auto inst_inner = create<PrefabInstance>("inst_inner", prefab_inner);
	commandInterface.set({inst_inner, {"template"}}, prefab_outer);

	auto inst_scene = create<PrefabInstance>("inst_scene");
	commandInterface.set({inst_scene, {"template"}}, prefab_inner);

	ASSERT_EQ(project.links().size(), 3);

	commandInterface.deleteObjects({prefab_outer});

	ASSERT_EQ(project.links().size(), 0);
}

TEST_F(PrefabTest, loop_instance_no_valid_template_check) {
	auto prefab1 = create<Prefab>("prefab 1");
	auto prefab2 = create<Prefab>("prefab 2");
	auto inst1 = create<PrefabInstance>("inst 1");
	auto inst2 = create<PrefabInstance>("inst 2");
	commandInterface.moveScenegraphChildren({inst1}, prefab1);
	commandInterface.moveScenegraphChildren({inst2}, prefab2);

	commandInterface.set({inst1, {"template"}}, prefab2);
	EXPECT_EQ(*inst1->template_, prefab2);

	auto valid = Queries::findAllValidReferenceTargets(project, {inst2, {"template"}});
	EXPECT_EQ(valid.size(), 0);
}

TEST_F(PrefabTest, delete_prefab_with_node_with_meshnode_while_instance_exists) {
	auto prefab = create<Prefab>("prefab 1");
	auto inst = create<PrefabInstance>("inst 1");
	auto node = create<Node>("node");
	auto meshNode = create<Node>("meshNode");
	commandInterface.set({inst, {"template"}}, prefab);
	commandInterface.moveScenegraphChildren({node}, prefab);
	commandInterface.moveScenegraphChildren({meshNode}, node);

	EXPECT_EQ(*inst->template_, prefab);

	commandInterface.deleteObjects({prefab});

	auto valid = Queries::findAllValidReferenceTargets(project, {inst, {"template"}});
	EXPECT_EQ(valid.size(), 0);
}

TEST_F(PrefabTest, update_inst_from_prefab_after_remove_link) {
	raco::ramses_base::HeadlessEngineBackend backend{};
	raco::application::RaCoApplication app{backend};
	auto& cmd = *app.activeRaCoProject().commandInterface();

	auto prefab = create<Prefab>(cmd, "prefab");
	auto inst = create<PrefabInstance>(cmd, "inst");
	cmd.set({inst, {"template"}}, prefab);
	auto node = create<Node>(cmd, "node", prefab);
	auto lua = create<LuaScript>(cmd, "lua", prefab);
	
	TextFile scriptFile = makeFile("script.lua", R"(
function interface()
	IN.v = VEC3F
	OUT.v = VEC3F
end
function run()
	OUT.v = IN.v
end
)");
	cmd.set({node, {"translation", "x"}}, 27.0);

	cmd.set({lua, {"uri"}}, scriptFile);
	cmd.addLink({lua, {"luaOutputs", "v"}}, {node, {"translation"}});
	app.doOneLoop();

	auto inst_node = raco::select<Node>(inst->children_->asVector<SEditorObject>());
	auto inst_lua = raco::select<LuaScript>(inst->children_->asVector<SEditorObject>());

	cmd.set({lua, {"luaInputs", "v", "x"}}, 2.0);
	cmd.set({inst_lua, {"luaInputs", "v", "x"}}, 3.0);
	app.doOneLoop();

	EXPECT_EQ(*node->translation_->x, 27.0);
	EXPECT_EQ(*inst_node->translation_->x, 3.0);

	cmd.removeLink({node, {"translation"}});

	EXPECT_EQ(*node->translation_->x, 27.0);
	EXPECT_EQ(*inst_node->translation_->x, 27.0);
}

TEST_F(PrefabTest, restore_cached_lua_prop_when_breaking_uri) {
	auto prefab = create<Prefab>("prefab");
	auto lua = create<LuaScript>("lua", prefab);
	commandInterface.set({lua, {"uri"}}, (test_path() / "scripts/types-scalar.lua").string());
	auto inst = create<PrefabInstance>("inst");
	commandInterface.set({inst, {"template"}}, prefab);

	auto inst_lua = raco::select<LuaScript>(inst->children_->asVector<SEditorObject>());

	commandInterface.set({lua, {"luaInputs", "float"}}, 2.0);
	commandInterface.set({inst_lua, {"luaInputs", "float"}}, 3.0);

	commandInterface.set({lua, {"uri"}}, std::string());

	ASSERT_EQ(ValueHandle(lua, {"luaInputs"}).size(), 0);
	ASSERT_EQ(ValueHandle(inst_lua, {"luaInputs"}).size(), 0);

	commandInterface.set({lua, {"uri"}}, (test_path() / "scripts/types-scalar.lua").string());

	ASSERT_TRUE(ValueHandle(lua, {"luaInputs"}).hasProperty("float"));
	ASSERT_TRUE(ValueHandle(inst_lua, {"luaInputs"}).hasProperty("float"));

	ASSERT_EQ(ValueHandle(lua, {"luaInputs"}).get("float").asDouble(), 2.0);
	ASSERT_EQ(ValueHandle(inst_lua, {"luaInputs"}).get("float").asDouble(), 3.0);
}

TEST_F(PrefabTest, update_simultaneous_scenegraph_move_and_delete_parent_node_linked) {
	auto prefab = create<Prefab>("prefab");
	auto inst = create<PrefabInstance>("inst");
	commandInterface.set({inst, {"template"}}, prefab);
	auto node = create<Node>("node", prefab);
	auto meshnode = create<MeshNode>("meshnode", node);
	auto lua = create<LuaScript>("lua", prefab);

	TextFile scriptFile = makeFile("script.lua", R"(
function interface()
	IN.v = VEC3F
	OUT.v = VEC3F
end
function run()
end
)");
	commandInterface.set({lua, {"uri"}}, scriptFile);
	EXPECT_EQ(inst->children_->size(), 2);
	auto inst_node = raco::select<Node>(inst->children_->asVector<SEditorObject>());
	auto inst_meshnode = raco::select<MeshNode>(inst->children_->asVector<SEditorObject>());
	auto inst_lua = raco::select<LuaScript>(inst->children_->asVector<SEditorObject>());
	EXPECT_TRUE(inst_node);
	EXPECT_FALSE(inst_meshnode);
	EXPECT_TRUE(inst_lua);

	commandInterface.addLink({lua, {"luaOutputs", "v"}}, {meshnode, {"translation"}});

	ASSERT_EQ(project.links().size(), 2);

	// Use context here to perform prefab update only after both operations are complete
	context.moveScenegraphChildren({meshnode}, prefab);
	context.deleteObjects({node});
	raco::core::PrefabOperations::globalPrefabUpdate(context);

	EXPECT_EQ(inst->children_->size(), 2);
	inst_meshnode = raco::select<MeshNode>(inst->children_->asVector<SEditorObject>());
	inst_lua = raco::select<LuaScript>(inst->children_->asVector<SEditorObject>());
	EXPECT_TRUE(inst_meshnode);
	EXPECT_TRUE(inst_lua);
	// These are the crucial checks:
	auto found_inst_meshnode = Queries::findById(project, inst_meshnode->objectID());
	ASSERT_TRUE(found_inst_meshnode != nullptr);
	ASSERT_EQ(found_inst_meshnode, inst_meshnode);

	checkLinks({
		{{lua, {"luaOutputs", "v"}}, {meshnode, {"translation"}}},
		{{inst_lua, {"luaOutputs", "v"}}, {inst_meshnode, {"translation"}}}
		});
	ASSERT_EQ(project.links().size(), 2);
}

TEST_F(PrefabTest, update_luascript_module_dependant_in_prefab_no_module) {
	auto prefab = create<Prefab>("prefab");
	auto inst = create<PrefabInstance>("inst");
	commandInterface.set({inst, &PrefabInstance::template_}, prefab);

	auto lua = create<LuaScript>("lua", prefab);
	commandInterface.set({lua, &LuaScript::uri_}, (test_path() / "scripts/moduleDependency.lua").string());
	commandInterface.moveScenegraphChildren({lua}, prefab);

	auto inst_lua = raco::select<LuaScript>(inst->children_->asVector<SEditorObject>());
	ASSERT_NE(inst_lua, nullptr);
	ASSERT_TRUE(commandInterface.errors().hasError(ValueHandle{lua, {"luaModules", "coalas"}}));
	ASSERT_TRUE(commandInterface.errors().hasError(ValueHandle{inst_lua, {"luaModules", "coalas"}}));
}

TEST_F(PrefabTest, update_luascript_module_dependant_in_prefab_add_module) {
	auto prefab = create<Prefab>("prefab");
	auto inst = create<PrefabInstance>("inst");
	commandInterface.set({inst, &PrefabInstance::template_}, prefab);

	auto lua = create<LuaScript>("lua", prefab);
	commandInterface.set({lua, &LuaScript::uri_}, (test_path() / "scripts/moduleDependency.lua").string());
	commandInterface.moveScenegraphChildren({lua}, prefab);

	auto luaModule = create<LuaScriptModule>("luaModule");
	commandInterface.set({luaModule, &LuaScriptModule::uri_}, (test_path() / "scripts/moduleDefinition.lua").string());

	commandInterface.set({lua, {"luaModules", "coalas"}}, luaModule);

	auto inst_lua = raco::select<LuaScript>(inst->children_->asVector<SEditorObject>());
	ASSERT_NE(inst_lua, nullptr);
	ASSERT_FALSE(commandInterface.errors().hasError({lua}));
	ASSERT_FALSE(commandInterface.errors().hasError({inst_lua}));
}

TEST_F(PrefabTest, update_luascript_module_dependant_in_prefab_remove_module) {
	auto prefab = create<Prefab>("prefab");
	auto inst = create<PrefabInstance>("inst");
	commandInterface.set({inst, &PrefabInstance::template_}, prefab);

	auto lua = create<LuaScript>("lua", prefab);
	commandInterface.set({lua, &LuaScript::uri_}, (test_path() / "scripts/moduleDependency.lua").string());
	commandInterface.moveScenegraphChildren({lua}, prefab);

	auto luaModule = create<LuaScriptModule>("luaModule");
	commandInterface.set({luaModule, &LuaScriptModule::uri_}, (test_path() / "scripts/moduleDefinition.lua").string());

	commandInterface.set({lua, {"luaModules", "coalas"}}, luaModule);
	commandInterface.set({lua, {"luaModules", "coalas"}}, SEditorObject{});

	auto inst_lua = raco::select<LuaScript>(inst->children_->asVector<SEditorObject>());
	ASSERT_NE(inst_lua, nullptr);
	ASSERT_TRUE(commandInterface.errors().hasError(ValueHandle{lua, {"luaModules", "coalas"}}));
	ASSERT_TRUE(commandInterface.errors().hasError(ValueHandle{inst_lua, {"luaModules", "coalas"}}));
}

TEST_F(PrefabTest, duplication_gets_propagated) {
	auto prefab = create<Prefab>("prefab");
	auto inst = create<PrefabInstance>("inst");
	commandInterface.set({inst, &PrefabInstance::template_}, prefab);

	auto node = create<Node>("node", prefab);
	commandInterface.moveScenegraphChildren({node}, prefab);

	commandInterface.duplicateObjects({node});

	ASSERT_EQ(inst->children_->asVector<SEditorObject>().size(), 2);
}

TEST_F(PrefabTest, update_private_material_ref_change_with_overlapping_property_names) {
	auto material1 = create<Material>("material1");
	auto material2 = create<Material>("material2");


	auto firstVertFile = makeFile("first.vert", R"(
#version 300 es
precision highp float;

uniform mat4 uWorldViewProjectionMatrix;
uniform vec2 offset;
uniform vec2 flip;

in vec3 a_Position;
in vec2 a_TextureCoordinate;

out vec2 v_TextureCoordinate;

void main() {
	float coordsX = (flip.x == 0.0) ? (a_TextureCoordinate.x) : (1.0 - a_TextureCoordinate.x);
	float coordsY = (flip.y == 0.0) ? (a_TextureCoordinate.y) : (1.0 - a_TextureCoordinate.y);
	v_TextureCoordinate = vec2(coordsX + offset.x, coordsY - offset.y);

	gl_Position = uWorldViewProjectionMatrix * vec4(a_Position, 1.0);
}

)");
	auto firstFragFile = makeFile("first.frag", R"(
#version 300 es
precision highp float;

uniform float u_Opacity;
uniform sampler2D u_Tex;
uniform vec4 u_Color;
uniform float currentTile;
uniform float numberOfTiles;


in vec2 v_TextureCoordinate;
out vec4 FragColor;
	
void main(){
	vec4 clr0 = texture(u_Tex, vec2(v_TextureCoordinate.x/numberOfTiles+(currentTile-1.0)*1.0/numberOfTiles, v_TextureCoordinate.y));
	FragColor.rgb = clr0.rgb * u_Color.rgb* u_Opacity;
	FragColor.a = clr0.a * u_Opacity * u_Color.a;

}
)");
	auto secondVertFile = makeFile("second.vert", R"(
#version 300 es
precision highp float;

uniform mat4 uWorldViewProjectionMatrix;
uniform vec2 offset;
uniform vec2 flip;
uniform float numberOfTilesX;
uniform float numberOfTilesY;
uniform float currentTile;
uniform sampler2D u_Tex;

in vec3 a_Position;
in vec2 a_TextureCoordinate;

out vec2 v_TextureCoordinate;

void main() {
	float coordsX = (flip.x == 0.0) ? (a_TextureCoordinate.x) : (1.0 - a_TextureCoordinate.x);
	float coordsY = (flip.y == 0.0) ? (1.0 - a_TextureCoordinate.y) : (a_TextureCoordinate.y);
	float tileX = mod(currentTile-1.0,numberOfTilesX);
	float tileY = numberOfTilesY-floor((currentTile-1.0)/numberOfTilesX)-1.0;
	float tilingAppliedU = (tileX+coordsX)/numberOfTilesX;
	float tilingAppliedV = 1.0 - ((tileY+coordsY)/numberOfTilesY);
	v_TextureCoordinate = vec2(tilingAppliedU + offset.x, tilingAppliedV - offset.y);
	
	highp vec2 tiling = vec2(numberOfTilesX, numberOfTilesY);
	vec2 texSize = vec2(textureSize(u_Tex,0));
	vec2 autoScale = texSize/tiling*a_Position.xy;
	vec2 alignToRaster = vec2(abs(mod(autoScale.x,1.0)), abs(mod(autoScale.y,1.0))); // in case of centered but odd dimensions it will shift half a pixel to align back to the raster
	vec3 adjustedPosition = vec3(autoScale-alignToRaster, a_Position.z);
	gl_Position = uWorldViewProjectionMatrix * vec4(adjustedPosition, 1.0);
}
)");
	auto secondFragFile = makeFile("second.frag", R"(
#version 300 es
precision highp float;

uniform float u_Opacity;
uniform sampler2D u_Tex;
uniform vec4 u_Color;

in vec2 v_TextureCoordinate;

out vec4 FragColor;

void main(){
	vec4 clr0 = texture(u_Tex, v_TextureCoordinate);
	if (clr0.a <= 0.0)
	{discard;}
	FragColor.rgb = clr0.rgb * u_Color.rgb;
	FragColor.a = clr0.a * u_Opacity * u_Color.a;
}

)");

	commandInterface.set({material1, &Material::uriVertex_}, firstVertFile);
	commandInterface.set({material1, &Material::uriFragment_}, firstFragFile);
	commandInterface.set({material2, &Material::uriVertex_}, secondVertFile);
	commandInterface.set({material2, &Material::uriFragment_}, secondFragFile);

	auto prefab = create<Prefab>("prefab");
	auto inst = create<PrefabInstance>("inst");
	commandInterface.set({inst, &PrefabInstance::template_}, prefab);

	auto mesh = create<Mesh>("mesh");
	commandInterface.set({mesh, &Mesh::uri_}, (test_path() / "meshes" / "Duck.glb").string());

	auto meshNode = create<MeshNode>("meshNode");
	commandInterface.set({meshNode, &MeshNode::mesh_}, mesh);
	commandInterface.set({meshNode, {"materials", "material", "material"}}, material1);
	commandInterface.set({meshNode, {"materials", "material", "private"}}, true);
	commandInterface.moveScenegraphChildren({meshNode}, prefab);

	auto texture = create<Texture>("tex");
	commandInterface.set({meshNode, {"materials", "material", "material"}}, material2);
	ASSERT_NO_THROW(commandInterface.set({meshNode, {"materials", "material", "uniforms", "u_Tex"}}, texture));
}

#ifdef NDEBUG
TEST_F(PrefabTest, prefab_performance_deletion_with_instance_1000_nodes) {
	auto prefab = create<Prefab>("prefab");
	auto inst = create<PrefabInstance>("inst");

	for (auto i = 0; i < 1000; ++i) {
		auto lua = create<LuaScript>("lua", prefab);
		commandInterface.set({lua, &LuaScript::uri_}, (test_path() / "scripts/types-scalar.lua").string());
		commandInterface.moveScenegraphChildren({lua}, prefab);
	}

	commandInterface.set({inst, &PrefabInstance::template_}, prefab);

	assertOperationTimeIsBelow(700, [this, prefab]() {
		commandInterface.deleteObjects({prefab});
	});
}


TEST_F(PrefabTest, prefab_performance_deletion_with_instance_and_10_prefab_instance_children) {
	auto prefab_to_delete = create<Prefab>("prefab");
	auto prefab_with_stuff = create<Prefab>("prefab2");
	auto inst = create<PrefabInstance>("inst");

	for (auto i = 0; i < 500; ++i) {
		auto lua = create<LuaScript>("lua", prefab_with_stuff);
		commandInterface.set({lua, &LuaScript::uri_}, (test_path() / "scripts/types-scalar.lua").string());
	}

	for (auto i = 0; i < 10; ++i) {
		auto delete_inst = create<PrefabInstance>("inst", prefab_to_delete);
		commandInterface.set({delete_inst, &PrefabInstance::template_}, prefab_with_stuff);
	}
	commandInterface.set({inst, &PrefabInstance::template_}, prefab_to_delete);


	assertOperationTimeIsBelow(16000, [this, prefab_to_delete]() {
		commandInterface.deleteObjects({prefab_to_delete});
	});
}

#endif