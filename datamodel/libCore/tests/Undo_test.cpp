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
#include "core/Project.h"
#include "core/Queries.h"
#include "core/ExternalReferenceAnnotation.h"
#include "ramses_base/HeadlessEngineBackend.h"
#include "testing/RacoBaseTest.h"
#include "testing/TestEnvironmentCore.h"
#include "user_types/UserObjectFactory.h"

#include "user_types/Mesh.h"
#include "user_types/MeshNode.h"
#include "user_types/Node.h"
#include "user_types/Prefab.h"
#include "user_types/PrefabInstance.h"

#include "gtest/gtest.h"

#include <algorithm>
#include <functional>

using namespace raco::core;
using namespace raco::user_types;

class UndoTest : public TestEnvironmentCore {
public:
	template <typename T>
	void checkSetValue(ValueHandle handle, const T& value) {
		T oldValue = handle.as<T>();
		checkUndoRedo([this, handle, value]() { commandInterface.set(handle, value); },
			[this, handle, oldValue]() {
				EXPECT_EQ(handle.as<T>(), oldValue);
			},
			[this, handle, value]() {
				EXPECT_EQ(handle.as<T>(), value);
			});
	}

	void checkJump(std::function<void()> operation, std::function<void()> preCheck, std::function<void()> postCheck) {
		size_t preIndex = undoStack.getIndex();
		preCheck();
		operation();
		size_t postIndex = undoStack.getIndex();
		postCheck();
		undoStack.setIndex(preIndex);
		preCheck();
		undoStack.setIndex(postIndex);
		postCheck();
	}

	bool findInstance(std::string objectName) {
		return std::find_if(project.instances().begin(), project.instances().end(), [objectName](const SEditorObject& obj) -> bool {
			return objectName == obj->objectName();
		}) != project.instances().end();
	}

	template <class C>
	std::shared_ptr<C> getInstance(std::string objectName) {
		auto it = std::find_if(project.instances().begin(), project.instances().end(), [objectName](const SEditorObject& obj) -> bool {
			return objectName == obj->objectName();
		});
		if (it != project.instances().end()) {
			return std::dynamic_pointer_cast<C>(*it);
		}
		return nullptr;
	}

	void checkInstances(const std::vector<std::string>& present, const std::vector<std::string>& absent) {
		EXPECT_EQ(project.instances().size(), present.size());
		for (auto name : present) {
			EXPECT_TRUE(findInstance(name));
		}
		for (auto name : absent) {
			EXPECT_FALSE(findInstance(name));
		}
	}
};

TEST_F(UndoTest, Node_undoredo_single_op) {
	auto node = commandInterface.createObject(Node::typeDescription.typeName, "node");

	checkSetValue<bool>(ValueHandle(node, {"visible"}), false);
	checkSetValue<double>(ValueHandle(node, {"translation", "x"}), 2.0);
}

TEST_F(UndoTest, Node_undoredo_merge_single_op) {
	auto node = commandInterface.createObject(Node::typeDescription.typeName, "node");

	ValueHandle translation_x{node, {"translation", "x"}};
	checkUndoRedo([this, translation_x]() { 
		commandInterface.set(translation_x, 2.0); 
		commandInterface.set(translation_x, 3.0); },
		[this, translation_x]() {
			EXPECT_EQ(translation_x.asDouble(), 0.0);
		},
		[this, translation_x]() {
			EXPECT_EQ(translation_x.asDouble(), 3.0);
		});
}

TEST_F(UndoTest, Node_undoredo_nomerge_different) {
	auto node = commandInterface.createObject(Node::typeDescription.typeName, "node");

	ValueHandle translation_x{node, {"translation", "x"}};
	ValueHandle translation_y{node, {"translation", "y"}};
	EXPECT_EQ(translation_x.asDouble(), 0.0);
	EXPECT_EQ(translation_y.asDouble(), 0.0);

	commandInterface.set(translation_x, 2.0);
	EXPECT_EQ(translation_x.asDouble(), 2.0);
	EXPECT_EQ(translation_y.asDouble(), 0.0);

	commandInterface.set(translation_y, 3.0);
	EXPECT_EQ(translation_x.asDouble(), 2.0);
	EXPECT_EQ(translation_y.asDouble(), 3.0);

	undoStack.undo();
	EXPECT_EQ(translation_x.asDouble(), 2.0);
	EXPECT_EQ(translation_y.asDouble(), 0.0);

	undoStack.undo();
	EXPECT_EQ(translation_x.asDouble(), 0.0);
	EXPECT_EQ(translation_y.asDouble(), 0.0);

	undoStack.redo();
	EXPECT_EQ(translation_x.asDouble(), 2.0);
	EXPECT_EQ(translation_y.asDouble(), 0.0);

	undoStack.redo();
	EXPECT_EQ(translation_x.asDouble(), 2.0);
	EXPECT_EQ(translation_y.asDouble(), 3.0);
}

TEST_F(UndoTest, Node_jump_single_op) {
	auto node = commandInterface.createObject(Node::typeDescription.typeName, "node");

	ValueHandle visibility{node, {"visible"}};
	checkJump([this, visibility]() { commandInterface.set(visibility, false); },
		[this, visibility]() {
			EXPECT_EQ(visibility.asBool(), true);
		},
		[this, visibility]() {
			EXPECT_EQ(visibility.asBool(), false);
		});

	ValueHandle translation_x{node, {"translation", "x"}};
	checkJump([this, translation_x]() { commandInterface.set(translation_x, 3.0); },
		[this, translation_x]() {
			EXPECT_EQ(translation_x.asDouble(), 0.0);
		},
		[this, translation_x]() {
			EXPECT_EQ(translation_x.asDouble(), 3.0);
		});
}

TEST_F(UndoTest, Node_jump_multi_op) {
	auto node = commandInterface.createObject(Node::typeDescription.typeName, "node");
	ValueHandle visibility{node, {"visible"}};
	ValueHandle translation_x{node, {"translation", "x"}};

	checkJump([this, visibility, translation_x]() { 
		commandInterface.set(visibility, false); 
		commandInterface.set(translation_x, 3.0); },
		[this, visibility, translation_x]() {
			EXPECT_EQ(visibility.asBool(), true);
			EXPECT_EQ(translation_x.asDouble(), 0.0);
		},
		[this, visibility, translation_x]() {
			EXPECT_EQ(visibility.asBool(), false);
			EXPECT_EQ(translation_x.asDouble(), 3.0);
		});
}

TEST_F(UndoTest, ScenegraphMove_simple) {
	auto node = commandInterface.createObject(Node::typeDescription.typeName, "parent");
	auto child = commandInterface.createObject(Node::typeDescription.typeName, "child");

	checkUndoRedo([this, node, child]() { commandInterface.moveScenegraphChild(child, node); },
		[this, node, child]() {
			EXPECT_EQ(node->children_->size(), 0);
			EXPECT_EQ(child->getParent(), nullptr);
		},
		[this, node, child]() {
			EXPECT_EQ(node->children_->asVector<SEditorObject>(), std::vector<SEditorObject>({child}));
			EXPECT_EQ(child->getParent(), node);
		});
}

TEST_F(UndoTest, ScenegraphMove_multiple_children) {
	auto node = commandInterface.createObject(Node::typeDescription.typeName, "parent");
	auto child1 = commandInterface.createObject(Node::typeDescription.typeName, "child1");
	commandInterface.moveScenegraphChild(child1, node);
	auto child2 = commandInterface.createObject(Node::typeDescription.typeName, "child2");

	checkUndoRedoMultiStep<2>(
		{[this, node, child1, child2]() { commandInterface.moveScenegraphChild(child2, node); },
			[this, node, child1, child2]() { commandInterface.moveScenegraphChild(child1, nullptr); }},
		{[this, node, child1, child2]() {
			 EXPECT_EQ(node->children_->asVector<SEditorObject>(), std::vector<SEditorObject>({child1}));
			 EXPECT_EQ(child1->getParent(), node);
			 EXPECT_EQ(child2->getParent(), nullptr);
		 },
			[this, node, child1, child2]() {
				EXPECT_EQ(node->children_->asVector<SEditorObject>(), std::vector<SEditorObject>({child1, child2}));
				EXPECT_EQ(child1->getParent(), node);
				EXPECT_EQ(child2->getParent(), node);
			},
			[this, node, child1, child2]() {
				EXPECT_EQ(node->children_->asVector<SEditorObject>(), std::vector<SEditorObject>({child2}));
				EXPECT_EQ(child1->getParent(), nullptr);
				EXPECT_EQ(child2->getParent(), node);
			}});
}

TEST_F(UndoTest, Create_node) {
	checkUndoRedo([this]() { commandInterface.createObject(Node::typeDescription.typeName, "node"); },
		[this]() {
			EXPECT_EQ(project.instances().size(), 0);
		},
		[this]() {
			EXPECT_EQ(project.instances().size(), 1);
			EXPECT_EQ(project.instances()[0]->objectName(), "node");
		});
}

TEST_F(UndoTest, Mesh_undoredo_setURI) {
	auto mesh = commandInterface.createObject(Mesh::typeDescription.typeName, "node");
#if (defined(__linux__))
	std::string unsanitizedPath = R"(Stuff//Work/folder1//folder2/folder3)";
#else
	std::string unsanitizedPath = R"(Stuff//Work\folder1\folder2/folder3)";
#endif
	std::string sanitizedPath = "Stuff/Work/folder1/folder2/folder3";

	checkUndoRedo([this, mesh, &unsanitizedPath]() { commandInterface.set(ValueHandle{mesh, {"uri"}}, unsanitizedPath); },
		[this, mesh]() {
			auto nodeURI = ValueHandle{mesh, {"uri"}}.asString();
			EXPECT_EQ(nodeURI, "");
		},
		[this, mesh, &sanitizedPath]() {
			auto nodeURI = ValueHandle{mesh, {"uri"}}.asString();
			EXPECT_EQ(nodeURI, sanitizedPath);
		});
}

TEST_F(UndoTest, Delete_single) {
	auto root = commandInterface.createObject("Node", "rootnode");
	auto mnb = commandInterface.createObject("MeshNode", "duck_node");
	auto mnl = commandInterface.createObject("MeshNode", "label_node");

	checkUndoRedo([this, root]() { commandInterface.deleteObjects({root}); },
		[this]() {
			checkInstances({"rootnode", "duck_node", "label_node"}, {});
		},
		[this]() {
			checkInstances({"duck_node", "label_node"}, {"rootnode"});
		});
}

TEST_F(UndoTest, DeleteNodeWithChild) {
	auto node = commandInterface.createObject(Node::typeDescription.typeName, "parent");
	auto child = commandInterface.createObject(Node::typeDescription.typeName, "child");
	commandInterface.moveScenegraphChild(child, node);

	checkUndoRedo([this, node]() { commandInterface.deleteObjects({node}); },
		[this, node, child]() {
			checkInstances({"parent", "child"}, {});
		},
		[this, node, child]() {
			checkInstances({}, {"parent", "child"});
		});
}

TEST_F(UndoTest, DeleteNodeInParent) {
	auto node = commandInterface.createObject(Node::typeDescription.typeName, "parent");
	auto child = commandInterface.createObject(Node::typeDescription.typeName, "child");
	commandInterface.moveScenegraphChild(child, node);

	checkUndoRedo([this, child]() { commandInterface.deleteObjects({child}); },
		[this, node]() {
			checkInstances({"parent", "child"}, {});
			auto child = getInstance<Node>("child");
			EXPECT_EQ(node->children_->asVector<SEditorObject>(), std::vector<SEditorObject>({child}));
			EXPECT_EQ(child->getParent(), node);
		},
		[this, node]() {
			checkInstances({"parent"}, {"child"});
			EXPECT_EQ(node->children_->size(), 0);
		});
}

TEST_F(UndoTest, Prefab_set_template) {
	auto prefab = create<Prefab>("prefab");
	auto inst = create<PrefabInstance>("inst");

	checkUndoRedo([this, prefab, inst]() { commandInterface.set(ValueHandle(inst, {"template"}), prefab); },
		[this, prefab, inst]() {
			EXPECT_EQ(*inst->template_, nullptr);
			EXPECT_EQ(prefab->instances_.size(), 0);
		},
		[this, prefab, inst]() {
			EXPECT_EQ(*inst->template_, prefab);
			EXPECT_EQ(prefab->instances_.size(), 1);
			EXPECT_EQ(prefab->instances_.begin()->lock(), inst);
		});

	checkUndoRedo([this, prefab, inst]() { commandInterface.set(ValueHandle(inst, {"template"}), SEditorObject()); },
		[this, prefab, inst]() {
			EXPECT_EQ(*inst->template_, prefab);
			EXPECT_EQ(prefab->instances_.size(), 1);
			EXPECT_EQ(prefab->instances_.begin()->lock(), inst);
		},
		[this, prefab, inst]() {
			EXPECT_EQ(*inst->template_, nullptr);
			EXPECT_EQ(prefab->instances_.size(), 0);
		});
}

TEST_F(UndoTest, DeletePrefab) {
	auto prefab = create<Prefab>("prefab");
	auto inst = create<PrefabInstance>("inst");

	commandInterface.set(ValueHandle(inst, {"template"}), prefab);
	EXPECT_EQ(*inst->template_, prefab);
	EXPECT_EQ(prefab->instances_.size(), 1);
	EXPECT_EQ(prefab->instances_.begin()->lock(), inst);

	checkUndoRedo([this, prefab]() { commandInterface.deleteObjects({prefab}); },
		[this, inst]() {
			auto prefab = getInstance<Prefab>("prefab");
			EXPECT_EQ(prefab->instances_.size(), 1);
			EXPECT_EQ(prefab->instances_.begin()->lock(), inst);
			EXPECT_EQ(*inst->template_, prefab);
		},
		[this, inst]() {
			EXPECT_FALSE(findInstance("prefab"));
			EXPECT_EQ(*inst->template_, nullptr);
		});
}

TEST_F(UndoTest, DeletePrefabInstance) {
	auto prefab = create<Prefab>("prefab");
	auto inst = create<PrefabInstance>("inst");

	commandInterface.set(ValueHandle(inst, {"template"}), prefab);
	EXPECT_EQ(*inst->template_, prefab);
	EXPECT_EQ(prefab->instances_.size(), 1);
	EXPECT_EQ(prefab->instances_.begin()->lock(), inst);

	checkUndoRedo([this, inst]() { commandInterface.deleteObjects({inst}); },
		[this, prefab]() {
			auto inst = getInstance<PrefabInstance>("inst");
			EXPECT_TRUE(inst);
			EXPECT_EQ(prefab->instances_.size(), 1);
			EXPECT_EQ(prefab->instances_.begin()->lock(), inst);
			EXPECT_EQ(*inst->template_, prefab);
		},
		[this, prefab]() {
			EXPECT_FALSE(findInstance("inst"));
			EXPECT_EQ(prefab->instances_.size(), 0);
		});
}
TEST_F(UndoTest, copyAndPasteObjectSimple) {
	auto node = create<Node>("node");

	checkUndoRedo([this, node]() { commandInterface.pasteObjects(context.copyObjects({node})); },
		[this, node]() {
			EXPECT_EQ(project.instances().size(), 1);
			EXPECT_EQ(project.instances()[0], node);
		},
		[this]() {
			// We have 2 instances, on original and one copy
			ASSERT_EQ(2, project.instances().size());
			// They are not equal to each other
			ASSERT_NE(project.instances().at(0), project.instances().at(1));
		});
}

TEST_F(UndoTest, copyAndPasteShallowSetsReferences) {
	auto meshNode = create<MeshNode>("meshnode");
	auto mesh = create<Mesh>("mesh");
	commandInterface.set({mesh, {"uri"}}, (cwd_path() / "meshes" / "Duck.glb").string());
	commandInterface.set({meshNode, {"mesh"}}, mesh);

	checkUndoRedo([this, meshNode]() { commandInterface.pasteObjects(context.copyObjects({meshNode})); },
		[this, meshNode, mesh]() {
			EXPECT_EQ(project.instances().size(), 2);
			EXPECT_EQ(getInstance<Mesh>("mesh"), mesh);
			EXPECT_EQ(getInstance<MeshNode>("meshnode"), meshNode);
		},
		[this, mesh]() {
			EXPECT_EQ(3, project.instances().size());
			for (auto obj : project.instances()) {
				if (auto meshnode = std::dynamic_pointer_cast<MeshNode>(obj)) {
					EXPECT_EQ(mesh, *meshnode->mesh_);
				}
			}
		});
}

TEST_F(UndoTest, deepCut) {
	auto node = create<Node>("node");
	auto meshNode = create<MeshNode>("meshnode");
	auto mesh = create<Mesh>("mesh");
	commandInterface.moveScenegraphChild(meshNode, node);
	commandInterface.set({meshNode, {"mesh"}}, mesh);

	checkUndoRedo([this, node]() { commandInterface.cutObjects({node}, true); },
		[this]() {
			ASSERT_EQ(3, context.project()->instances().size());
		},
		[this]() {
			ASSERT_EQ(0, context.project()->instances().size());
		});
}

TEST_F(UndoTest, link_broken_changed_output) {
	auto linkBase = create<LuaScript>("script1");
	commandInterface.set(ValueHandle{linkBase, {"uri"}}, cwd_path().append("scripts/types-scalar.lua").string());
	auto linkRecipient = create<LuaScript>("script2");
	commandInterface.set(ValueHandle{linkRecipient, {"uri"}}, cwd_path().append("scripts/SimpleScript.lua").string());

	commandInterface.addLink(ValueHandle{linkBase, {"luaOutputs", "ofloat"}}, ValueHandle{linkRecipient, {"luaInputs", "in_float"}});

	// link gets broken here
	checkUndoRedo([this, linkBase]() { commandInterface.set(ValueHandle{linkBase, {"uri"}}, cwd_path().append("scripts/SimpleScript.lua").string()); },
		[this]() {
			ASSERT_EQ(project.links().size(), 1);
			ASSERT_TRUE(project.links()[0]->isValid());
		},
		[this]() {
			ASSERT_EQ(project.links().size(), 1);
			ASSERT_FALSE(project.links()[0]->isValid());
		});
}

TEST_F(UndoTest, link_broken_changed_input) {
	auto linkBase = create<LuaScript>("script1");
	commandInterface.set(ValueHandle{linkBase, {"uri"}}, cwd_path().append("scripts/types-scalar.lua").string());

	auto linkRecipient = create<LuaScript>("script2");
	commandInterface.set(ValueHandle{linkRecipient, {"uri"}}, cwd_path().append("scripts/SimpleScript.lua").string());

	commandInterface.addLink(ValueHandle{linkBase, {"luaOutputs", "ofloat"}}, ValueHandle{linkRecipient, {"luaInputs", "in_float"}});

	// link gets broken here
	checkUndoRedo([this, linkRecipient]() { commandInterface.set(ValueHandle{linkRecipient, {"uri"}}, cwd_path().append("scripts/types-scalar.lua").string()); },
		[this]() {
			ASSERT_EQ(project.links().size(), 1);
			ASSERT_TRUE(project.links()[0]->isValid());
		},
		[this]() {
			ASSERT_EQ(project.links().size(), 1);
			ASSERT_FALSE(project.links()[0]->isValid());
		});
}

TEST_F(UndoTest, link_broken_fix_link_with_correct_input) {
	auto linkBase = create<LuaScript>("script1");
	commandInterface.set(ValueHandle{linkBase, {"uri"}}, cwd_path().append("scripts/types-scalar.lua").string());

	auto linkRecipient = create<LuaScript>("script2");
	commandInterface.set(ValueHandle{linkRecipient, {"uri"}}, cwd_path().append("scripts/SimpleScript.lua").string());

	commandInterface.addLink(ValueHandle{linkBase, {"luaOutputs", "ofloat"}}, ValueHandle{linkRecipient, {"luaInputs", "in_float"}});

	// link gets broken here
	commandInterface.set(ValueHandle{linkRecipient, {"uri"}}, cwd_path().append("scripts/types-scalar.lua").string());

	// Simulate user doing stuff after changing URI to prevent undo stack merging when changing URI again.
	create<Node>("Node");

	checkUndoRedo([this, linkRecipient]() { commandInterface.set(ValueHandle{linkRecipient, {"uri"}}, cwd_path().append("scripts/SimpleScript.lua").string()); },
		[this]() {
			ASSERT_EQ(project.links().size(), 1);
			ASSERT_FALSE(project.links()[0]->isValid());
		},
		[this]() {
			ASSERT_EQ(project.links().size(), 1);
			ASSERT_TRUE(project.links()[0]->isValid());
		});
}

TEST_F(UndoTest, link_broken_fix_link_with_correct_output) {
	auto linkBase = create<LuaScript>("script1");
	commandInterface.set(ValueHandle{linkBase, {"uri"}}, cwd_path().append("scripts/types-scalar.lua").string());
	auto linkRecipient = create<LuaScript>("script2");
	commandInterface.set(ValueHandle{linkRecipient, {"uri"}}, cwd_path().append("scripts/SimpleScript.lua").string());

	commandInterface.addLink(ValueHandle{linkBase, {"luaOutputs", "ofloat"}}, ValueHandle{linkRecipient, {"luaInputs", "in_float"}});

	// link gets broken here
	commandInterface.set(ValueHandle{linkBase, {"uri"}}, cwd_path().append("scripts/SimpleScript.lua").string());

        // Simulate user doing stuff after changing URI to prevent undo stack merging when changing URI again.
	create<Node>("Node");

	checkUndoRedo([this, linkBase]() { commandInterface.set(ValueHandle{linkBase, {"uri"}}, cwd_path().append("scripts/types-scalar.lua").string()); },
		[this]() {
			ASSERT_EQ(project.links().size(), 1);
			ASSERT_FALSE(project.links()[0]->isValid());
		},
		[this]() {
			ASSERT_EQ(project.links().size(), 1);
			ASSERT_TRUE(project.links()[0]->isValid());
		});
}

TEST_F(UndoTest, link_input_changed_add_another_link) {
	auto linkBase = create<LuaScript>("script1");
	commandInterface.set(ValueHandle{linkBase, {"uri"}}, cwd_path().append("scripts/types-scalar.lua").string());

	auto linkRecipient = create<LuaScript>("script2");
	commandInterface.set(ValueHandle{linkRecipient, {"uri"}}, cwd_path().append("scripts/SimpleScript.lua").string());

	commandInterface.addLink(ValueHandle{linkBase, {"luaOutputs", "ofloat"}}, ValueHandle{linkRecipient, {"luaInputs", "in_float"}});

	// link gets broken here
	commandInterface.set(ValueHandle{linkRecipient, {"uri"}}, cwd_path().append("scripts/types-scalar.lua").string());
	checkUndoRedo([this, linkBase, linkRecipient]() { commandInterface.addLink(ValueHandle{linkBase, {"luaOutputs", "ofloat"}}, ValueHandle{linkRecipient, {"luaInputs", "float"}}); },
		[this]() {
			ASSERT_EQ(project.links().size(), 1);
			ASSERT_FALSE(project.links()[0]->isValid());
		},
		[this]() {
			ASSERT_EQ(project.links().size(), 2);
			ASSERT_FALSE(project.links()[0]->isValid());
			ASSERT_TRUE(project.links()[1]->isValid());
		});
}