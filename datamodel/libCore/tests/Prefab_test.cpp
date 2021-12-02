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


#include "user_types/Mesh.h"
#include "user_types/MeshNode.h"
#include "user_types/Node.h"
#include "user_types/Prefab.h"
#include "user_types/PrefabInstance.h"

#include "gtest/gtest.h"

using namespace raco::core;
using namespace raco::user_types;


class PrefabTest : public TestEnvironmentCore {
public:
};

TEST_F(PrefabTest, move_node_in) {
	auto prefab = create<Prefab>("prefab");
	auto inst = create<PrefabInstance>("inst");
	auto node = create<Node>("node");
	commandInterface.set({inst, {"template"}}, prefab);

	checkUndoRedo([this, node, prefab]() { commandInterface.moveScenegraphChild(node, prefab); },
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
	commandInterface.moveScenegraphChild(node, prefab);

	checkUndoRedo([this, node, prefab]() { commandInterface.moveScenegraphChild(node, nullptr); },
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
	commandInterface.moveScenegraphChild(node, prefab);

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
	commandInterface.moveScenegraphChild(node, prefab);

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
	commandInterface.moveScenegraphChild(node, prefab);

	auto luaPrefabGlobal = create<LuaScript>("luaPrefab");
	commandInterface.moveScenegraphChild(luaPrefabGlobal, prefab);

	auto luaPrefabNodeChild = create<LuaScript>("luaPrefabNode");
	commandInterface.moveScenegraphChild(luaPrefabNodeChild, node);

	commandInterface.set({luaPrefabGlobal, {"uri"}}, cwd_path().append("scripts/types-scalar.lua").string());
	commandInterface.set({luaPrefabNodeChild, {"uri"}}, cwd_path().append("scripts/SimpleScript.lua").string());

	commandInterface.addLink({luaPrefabGlobal, {"luaOutputs", "ovector3f"}} , {node, {"translation"}});
	commandInterface.addLink({luaPrefabNodeChild, {"luaOutputs", "out_float"}}, {luaPrefabGlobal, {"luaInputs", "float"}});
	auto inst_node = raco::select<Node>(inst->children_->asVector<SEditorObject>());
	auto inst_lua_prefab = raco::select<LuaScript>(inst->children_->asVector<SEditorObject>());
	std::vector<Link> refLinks{{{{luaPrefabGlobal, {"luaOutputs", "ovector3f"}}, {node, {"translation"}}},
		{{luaPrefabNodeChild, {"luaOutputs", "out_float"}}, {luaPrefabGlobal, {"luaInputs", "float"}}},
		{{inst_lua_prefab, {"luaOutputs", "ovector3f"}}, {inst_node, {"translation"}}}}};
	checkLinks(refLinks);

	commandInterface.set({luaPrefabNodeChild, {"uri"}}, cwd_path().append("scripts/types-scalar.lua").string());
	commandInterface.set({luaPrefabGlobal, {"uri"}}, cwd_path().append("scripts/SimpleScript.lua").string());
	ASSERT_EQ(project.links().size(), 3);
	ASSERT_FALSE(std::all_of(project.links().begin(), project.links().end(), [](const auto link) { return link->isValid(); }));

	commandInterface.set({luaPrefabGlobal, {"uri"}}, cwd_path().append("scripts/types-scalar.lua").string());
	commandInterface.set({luaPrefabNodeChild, {"uri"}}, cwd_path().append("scripts/SimpleScript.lua").string());
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
	auto prefab_1 = create<Prefab>("prefab 1");
	auto prefab_2 = create<Prefab>("prefab 2");

	auto inst_1 = create<PrefabInstance>("inst 1");
	commandInterface.set({inst_1, {"template"}}, prefab_1);
	commandInterface.moveScenegraphChild(inst_1, prefab_2);

	auto inst_2 = create<PrefabInstance>("inst 2");
	commandInterface.set({inst_2, {"template"}}, prefab_2);

	auto inst_2_children = inst_2->children_->asVector<SEditorObject>();
	EXPECT_EQ(inst_2_children.size(), 1);
	auto inst_2_child = inst_2_children[0]->as<PrefabInstance>();
	EXPECT_TRUE(inst_2_child);

	auto node = create<Node>("node");

	checkUndoRedo([this, node, prefab_1]() { commandInterface.moveScenegraphChild(node, prefab_1); },
		[this, inst_2_child]() {
			EXPECT_EQ(inst_2_child->children_->size(), 0);
		},
		[this, inst_2_child]() {
			EXPECT_EQ(inst_2_child->children_->size(), 1);
			EXPECT_TRUE(inst_2_child->children_->asVector<SEditorObject>()[0]->as<Node>());
		});
}

TEST_F(PrefabTest, nesting_set_node_prop) {
	auto prefab_1 = create<Prefab>("prefab 1");
	auto prefab_2 = create<Prefab>("prefab 2");

	auto inst_1 = create<PrefabInstance>("inst 1");
	commandInterface.set({inst_1, {"template"}}, prefab_1);
	commandInterface.moveScenegraphChild(inst_1, prefab_2);

	auto inst_2 = create<PrefabInstance>("inst 2");
	commandInterface.set({inst_2, {"template"}}, prefab_2);

	auto inst_2_children = inst_2->children_->asVector<SEditorObject>();
	EXPECT_EQ(inst_2_children.size(), 1);
	auto inst_2_child = inst_2_children[0]->as<PrefabInstance>();
	EXPECT_TRUE(inst_2_child);

	auto node = create<Node>("node");
	commandInterface.moveScenegraphChild(node, prefab_1);

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
	auto prefab_1 = create<Prefab>("prefab 1");
	auto prefab_2 = create<Prefab>("prefab 2");
	auto inst_1 = create<PrefabInstance>("inst 1");
	auto inst_2 = create<PrefabInstance>("inst 2");
	commandInterface.moveScenegraphChild(inst_1, prefab_1);
	commandInterface.moveScenegraphChild(inst_2, prefab_2);

	commandInterface.set({inst_1, {"template"}}, prefab_2);
	EXPECT_EQ(*inst_1->template_, prefab_2);

	auto valid = Queries::findAllValidReferenceTargets(project, {inst_2, {"template"}});
	EXPECT_EQ(valid.size(), 0);
}

TEST_F(PrefabTest, delete_prefab_with_node_with_meshnode_while_instance_exists) {
	auto prefab = create<Prefab>("prefab 1");
	auto inst = create<PrefabInstance>("inst 1");
	auto node = create<Node>("node");
	auto meshNode = create<Node>("meshNode");
	commandInterface.set({inst, {"template"}}, prefab);
	commandInterface.moveScenegraphChild(node, prefab);
	commandInterface.moveScenegraphChild(meshNode, node);

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
	commandInterface.set({lua, {"uri"}}, (cwd_path() / "scripts/types-scalar.lua").string());
	auto inst = create<PrefabInstance>("inst");
	commandInterface.set({inst, {"template"}}, prefab);

	auto inst_lua = raco::select<LuaScript>(inst->children_->asVector<SEditorObject>());

	commandInterface.set({lua, {"luaInputs", "float"}}, 2.0);
	commandInterface.set({inst_lua, {"luaInputs", "float"}}, 3.0);

	commandInterface.set({lua, {"uri"}}, std::string());

	ASSERT_EQ(ValueHandle(lua, {"luaInputs"}).size(), 0);
	ASSERT_EQ(ValueHandle(inst_lua, {"luaInputs"}).size(), 0);

	commandInterface.set({lua, {"uri"}}, (cwd_path() / "scripts/types-scalar.lua").string());

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
	context.moveScenegraphChild(meshnode, prefab);
	context.deleteObjects({node});
	raco::core::PrefabOperations::globalPrefabUpdate(context, context.modelChanges());

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
