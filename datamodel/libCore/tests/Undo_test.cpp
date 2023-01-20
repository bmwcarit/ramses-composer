/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "core/Undo.h"
#include "core/Context.h"
#include "core/Handles.h"
#include "core/MeshCacheInterface.h"
#include "core/Project.h"
#include "core/Queries.h"
#include "core/ExternalReferenceAnnotation.h"
#include "ramses_base/HeadlessEngineBackend.h"
#include "testing/RacoBaseTest.h"
#include "testing/TestEnvironmentCore.h"
#include "testing/TestUtil.h"
#include "user_types/UserObjectFactory.h"
#include "user_types/Animation.h"
#include "user_types/AnimationChannel.h"
#include "user_types/LuaScriptModule.h"
#include "user_types/Mesh.h"
#include "user_types/MeshNode.h"
#include "user_types/Node.h"
#include "user_types/PerspectiveCamera.h"
#include "user_types/Prefab.h"
#include "user_types/PrefabInstance.h"
#include "user_types/RenderBuffer.h"
#include "user_types/RenderPass.h"
#include "user_types/RenderTarget.h"

#include "gtest/gtest.h"

#include <algorithm>
#include <functional>

using namespace raco::core;
using namespace raco::user_types;

template <class T = ::testing::Test>
class UndoTestT : public TestEnvironmentCoreT<T> {
public:
	UndoTestT() : TestEnvironmentCoreT<T>(&TestObjectFactory::getInstance()) {
	}

	template <typename C>
	void checkSetValue(ValueHandle handle, const C& value) {
		C oldValue = handle.as<C>();
		this->checkUndoRedo([this, handle, value]() { this->commandInterface.set(handle, value); },
			[this, handle, oldValue]() {
				EXPECT_EQ(handle.as<C>(), oldValue);
			},
			[this, handle, value]() {
				EXPECT_EQ(handle.as<C>(), value);
			});
	}

	void checkJump(std::function<void()> operation, std::function<void()> preCheck, std::function<void()> postCheck) {
		size_t preIndex = this->undoStack.getIndex();
		preCheck();
		operation();
		size_t postIndex = this->undoStack.getIndex();
		postCheck();
		this->undoStack.setIndex(preIndex);
		preCheck();
		this->undoStack.setIndex(postIndex);
		postCheck();
	}

	bool findInstance(std::string objectName) {
		return std::find_if(this->project.instances().begin(), this->project.instances().end(), [objectName](const SEditorObject& obj) -> bool {
			return objectName == obj->objectName();
		}) != this->project.instances().end();
	}

	template <class C>
	std::shared_ptr<C> getInstance(Project& project, std::string objectName) {
		auto it = std::find_if(project.instances().begin(), project.instances().end(), [objectName](const SEditorObject& obj) -> bool {
			return objectName == obj->objectName();
		});
		if (it != project.instances().end()) {
			return std::dynamic_pointer_cast<C>(*it);
		}
		return nullptr;
	}

	template <class C>
	std::shared_ptr<C> getInstance(std::string objectName) {
		return getInstance<C>(this->project, objectName);
	}

	void checkInstances(const std::vector<std::string>& present, const std::vector<std::string>& absent) {
		EXPECT_EQ(this->project.instances().size(), present.size());
		for (auto name : present) {
			EXPECT_TRUE(findInstance(name));
		}
		for (auto name : absent) {
			EXPECT_FALSE(findInstance(name));
		}
	}
};

class UndoTest : public UndoTestT<> {
public:
	std::vector<std::unique_ptr<TestUndoStack::Entry>>& stack() {
		return undoStack.stack();
	}
};

TEST_F(UndoTest, Node_undoredo_single_op) {
	auto node = commandInterface.createObject(Node::typeDescription.typeName, "node");

	checkSetValue<bool>(ValueHandle(node, {"visibility"}), false);
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

	ValueHandle visibility{node, {"visibility"}};
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
	ValueHandle visibility{node, {"visibility"}};
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

	checkUndoRedo([this, node, child]() { commandInterface.moveScenegraphChildren({child}, node); },
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
	commandInterface.moveScenegraphChildren({child1}, node);
	auto child2 = commandInterface.createObject(Node::typeDescription.typeName, "child2");

	checkUndoRedoMultiStep<2>(
		{[this, node, child1, child2]() { commandInterface.moveScenegraphChildren({child2}, node); },
			[this, node, child1, child2]() { commandInterface.moveScenegraphChildren({child1}, nullptr); }},
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
			EXPECT_EQ(project.instances().size(), 1);
		},
		[this]() {
			EXPECT_EQ(project.instances().size(), 2);
			EXPECT_EQ(project.instances()[1]->objectName(), "node");
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
			checkInstances({"ProjectSettings", "rootnode", "duck_node", "label_node"}, {});
		},
		[this]() {
			checkInstances({"ProjectSettings", "duck_node", "label_node"}, {"rootnode"});
		});
}

TEST_F(UndoTest, DeleteNodeWithChild) {
	auto node = commandInterface.createObject(Node::typeDescription.typeName, "parent");
	auto child = commandInterface.createObject(Node::typeDescription.typeName, "child");
	commandInterface.moveScenegraphChildren({child}, node);

	checkUndoRedo([this, node]() { commandInterface.deleteObjects({node}); },
		[this, node, child]() {
			checkInstances({"ProjectSettings", "parent", "child"}, {});
		},
		[this, node, child]() {
			checkInstances({"ProjectSettings"}, {"parent", "child"});
		});
}

TEST_F(UndoTest, DeleteNodeInParent) {
	auto node = commandInterface.createObject(Node::typeDescription.typeName, "parent");
	auto child = commandInterface.createObject(Node::typeDescription.typeName, "child");
	commandInterface.moveScenegraphChildren({child}, node);

	checkUndoRedo([this, child]() { commandInterface.deleteObjects({child}); },
		[this, node]() {
			checkInstances({"ProjectSettings", "parent", "child"}, {});
			auto child = getInstance<Node>("child");
			EXPECT_EQ(node->children_->asVector<SEditorObject>(), std::vector<SEditorObject>({child}));
			EXPECT_EQ(child->getParent(), node);
		},
		[this, node]() {
			checkInstances({"ProjectSettings", "parent"}, {"child"});
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
			EXPECT_EQ(project.instances().size(), 2);
			EXPECT_EQ(project.instances()[1], node);
		},
		[this]() {
			// We have 3 instances, on original and one copy
			ASSERT_EQ(project.instances().size(), 3);
			// They are not equal to each other
			ASSERT_NE(project.instances().at(1), project.instances().at(2));
		});
}

TEST_F(UndoTest, copyAndPasteShallowSetsReferences) {
	auto meshNode = create<MeshNode>("meshnode");
	auto mesh = create<Mesh>("mesh");
	commandInterface.set({mesh, {"uri"}}, (test_path() / "meshes" / "Duck.glb").string());
	commandInterface.set({meshNode, {"mesh"}}, mesh);

	checkUndoRedo([this, meshNode]() { commandInterface.pasteObjects(context.copyObjects({meshNode})); },
		[this, meshNode, mesh]() {
			EXPECT_EQ(project.instances().size(), 3);
			EXPECT_EQ(getInstance<Mesh>("mesh"), mesh);
			EXPECT_EQ(getInstance<MeshNode>("meshnode"), meshNode);
		},
		[this, mesh]() {
			EXPECT_EQ(project.instances().size(), 4);
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
	commandInterface.moveScenegraphChildren({meshNode}, node);
	commandInterface.set({meshNode, {"mesh"}}, mesh);

	checkUndoRedo([this, node]() { commandInterface.cutObjects({node}, true); },
		[this]() {
			ASSERT_EQ(context.project()->instances().size(), 4);
		},
		[this]() {
			ASSERT_EQ(context.project()->instances().size(), 1);
		});
}

TEST_F(UndoTest, no_change_records_for_deleted_objects) {
	auto start = create_lua("start", "scripts/types-scalar.lua");

	auto index = undoStack.getIndex();

	auto end = create_lua("end", "scripts/types-scalar.lua");
	commandInterface.addLink(ValueHandle{start, {"outputs", "ofloat"}}, ValueHandle{end, {"inputs", "float"}});

	std::vector<Link> refLinks{{{{start, {"outputs", "ofloat"}}, {end, {"inputs", "float"}}}}};
	checkLinks(refLinks);

	undoStack.setIndex(index);

	auto changedSet = recorder.getAllChangedObjects();
	EXPECT_TRUE(changedSet.find(end) == changedSet.end());
}

TEST_F(UndoTest, link_strong_to_weak_transition) {
	auto start = create_lua("start", "scripts/types-scalar.lua");
	auto end = create_lua("end", "scripts/types-scalar.lua");
	commandInterface.addLink(ValueHandle{start, {"outputs", "ofloat"}}, ValueHandle{end, {"inputs", "float"}});

	checkJump(
		[this, start, end]() {
			commandInterface.removeLink({end, {"inputs", "float"}});
			commandInterface.addLink(ValueHandle{start, {"outputs", "ofloat"}}, ValueHandle{end, {"inputs", "float"}}, true);
		},
		[this, start, end]() {
			std::vector<Link> refLinks{{{{start, {"outputs", "ofloat"}}, {end, {"inputs", "float"}}, true, false}}};
			checkLinks(refLinks);
			EXPECT_FALSE(Queries::linkWouldBeAllowed(project, {end, {"outputs", "ofloat"}}, {start, {"inputs", "float"}}, false));
		},
		[this, start, end]() {
			std::vector<Link> refLinks{{{{start, {"outputs", "ofloat"}}, {end, {"inputs", "float"}}, true, true}}};
			checkLinks(refLinks);
			EXPECT_TRUE(Queries::linkWouldBeAllowed(project, {end, {"outputs", "ofloat"}}, {start, {"inputs", "float"}}, false));
		});
}

TEST_F(UndoTest, link_strong_valid_to_weak_invalid_transition) {
	auto start = create_lua("start", "scripts/types-scalar.lua");
	auto end = create_lua("end", "scripts/types-scalar.lua");
	commandInterface.addLink(ValueHandle{start, {"outputs", "ofloat"}}, ValueHandle{end, {"inputs", "float"}});

	checkJump(
		[this, start, end]() {
			commandInterface.removeLink({end, {"inputs", "float"}});
			commandInterface.addLink(ValueHandle{start, {"outputs", "ofloat"}}, ValueHandle{end, {"inputs", "float"}}, true);
			commandInterface.set(ValueHandle{end, {"uri"}}, (test_path() / "scripts/SimpleScript.lua").string());
		},
		[this, start, end]() {
			std::vector<Link> refLinks{{{{start, {"outputs", "ofloat"}}, {end, {"inputs", "float"}}, true, false}}};
			checkLinks(refLinks);
			EXPECT_FALSE(Queries::linkWouldBeAllowed(project, {end, {"outputs", "ofloat"}}, {start, {"inputs", "float"}}, false));
		},
		[this, start, end]() {
			std::vector<Link> refLinks{{{{start, {"outputs", "ofloat"}}, {end, {"inputs", "float"}}, false, true}}};
			checkLinks(refLinks);
			EXPECT_TRUE(Queries::linkWouldBeAllowed(project, {end, {"outputs", "ofloat"}}, {start, {"inputs", "float"}}, false));
		});
}

TEST_F(UndoTest, link_broken_changed_output) {
	auto linkBase = create<LuaScript>("script1");
	commandInterface.set(ValueHandle{linkBase, {"uri"}}, test_path().append("scripts/types-scalar.lua").string());
	auto linkRecipient = create<LuaScript>("script2");
	commandInterface.set(ValueHandle{linkRecipient, {"uri"}}, test_path().append("scripts/SimpleScript.lua").string());

	commandInterface.addLink(ValueHandle{linkBase, {"outputs", "ofloat"}}, ValueHandle{linkRecipient, {"inputs", "in_float"}});

	// link gets broken here
	checkUndoRedo([this, linkBase]() { commandInterface.set(ValueHandle{linkBase, {"uri"}}, test_path().append("scripts/SimpleScript.lua").string()); },
		[this]() {
			ASSERT_EQ(project.links().size(), 1);
			ASSERT_TRUE((*project.links().begin())->isValid());
		},
		[this]() {
			ASSERT_EQ(project.links().size(), 1);
			ASSERT_FALSE((*project.links().begin())->isValid());
		});
}

TEST_F(UndoTest, link_broken_changed_input) {
	auto linkBase = create<LuaScript>("script1");
	commandInterface.set(ValueHandle{linkBase, {"uri"}}, test_path().append("scripts/types-scalar.lua").string());

	auto linkRecipient = create<LuaScript>("script2");
	commandInterface.set(ValueHandle{linkRecipient, {"uri"}}, test_path().append("scripts/SimpleScript.lua").string());

	commandInterface.addLink(ValueHandle{linkBase, {"outputs", "ofloat"}}, ValueHandle{linkRecipient, {"inputs", "in_float"}});

	// link gets broken here
	checkUndoRedo([this, linkRecipient]() { commandInterface.set(ValueHandle{linkRecipient, {"uri"}}, test_path().append("scripts/types-scalar.lua").string()); },
		[this]() {
			ASSERT_EQ(project.links().size(), 1);
			ASSERT_TRUE((*project.links().begin())->isValid());
		},
		[this]() {
			ASSERT_EQ(project.links().size(), 1);
			ASSERT_FALSE((*project.links().begin())->isValid());
		});
}

TEST_F(UndoTest, link_broken_fix_link_with_correct_input) {
	auto linkBase = create<LuaScript>("script1");
	commandInterface.set(ValueHandle{linkBase, {"uri"}}, test_path().append("scripts/types-scalar.lua").string());

	auto linkRecipient = create<LuaScript>("script2");
	commandInterface.set(ValueHandle{linkRecipient, {"uri"}}, test_path().append("scripts/SimpleScript.lua").string());

	commandInterface.addLink(ValueHandle{linkBase, {"outputs", "ofloat"}}, ValueHandle{linkRecipient, {"inputs", "in_float"}});

	// link gets broken here
	commandInterface.set(ValueHandle{linkRecipient, {"uri"}}, test_path().append("scripts/types-scalar.lua").string());

	// Simulate user doing stuff after changing URI to prevent undo stack merging when changing URI again.
	create<Node>("Node");

	checkUndoRedo([this, linkRecipient]() { commandInterface.set(ValueHandle{linkRecipient, {"uri"}}, test_path().append("scripts/SimpleScript.lua").string()); },
		[this]() {
			ASSERT_EQ(project.links().size(), 1);
			ASSERT_FALSE((*project.links().begin())->isValid());
		},
		[this]() {
			ASSERT_EQ(project.links().size(), 1);
			ASSERT_TRUE((*project.links().begin())->isValid());
		});
}

TEST_F(UndoTest, link_broken_fix_link_with_correct_output) {
	auto linkBase = create<LuaScript>("script1");
	commandInterface.set(ValueHandle{linkBase, {"uri"}}, test_path().append("scripts/types-scalar.lua").string());
	auto linkRecipient = create<LuaScript>("script2");
	commandInterface.set(ValueHandle{linkRecipient, {"uri"}}, test_path().append("scripts/SimpleScript.lua").string());

	commandInterface.addLink(ValueHandle{linkBase, {"outputs", "ofloat"}}, ValueHandle{linkRecipient, {"inputs", "in_float"}});

	// link gets broken here
	commandInterface.set(ValueHandle{linkBase, {"uri"}}, test_path().append("scripts/SimpleScript.lua").string());

	// Simulate user doing stuff after changing URI to prevent undo stack merging when changing URI again.
	create<Node>("Node");

	checkUndoRedo([this, linkBase]() { commandInterface.set(ValueHandle{linkBase, {"uri"}}, test_path().append("scripts/types-scalar.lua").string()); },
		[this]() {
			ASSERT_EQ(project.links().size(), 1);
			ASSERT_FALSE((*project.links().begin())->isValid());
		},
		[this]() {
			ASSERT_EQ(project.links().size(), 1);
			ASSERT_TRUE((*project.links().begin())->isValid());
		});
}

TEST_F(UndoTest, link_input_changed_add_another_link) {
	auto linkBase = create<LuaScript>("script1");
	commandInterface.set(ValueHandle{linkBase, {"uri"}}, test_path().append("scripts/types-scalar.lua").string());

	auto linkRecipient = create<LuaScript>("script2");
	commandInterface.set(ValueHandle{linkRecipient, {"uri"}}, test_path().append("scripts/SimpleScript.lua").string());

	commandInterface.addLink(ValueHandle{linkBase, {"outputs", "ofloat"}}, ValueHandle{linkRecipient, {"inputs", "in_float"}});

	// link gets broken here

	commandInterface.set(ValueHandle{linkRecipient, {"uri"}}, test_path().append("scripts/types-scalar.lua").string());
	checkUndoRedo([this, linkBase, linkRecipient]() { commandInterface.addLink(ValueHandle{linkBase, {"outputs", "ofloat"}}, ValueHandle{linkRecipient, {"inputs", "float"}}); },
		[this, linkBase, linkRecipient]() { 
			checkLinks({{{linkBase, {"outputs", "ofloat"}}, {linkRecipient, {"inputs", "in_float"}}, false}});
		},
		[this, linkBase, linkRecipient]() {
			checkLinks({{{linkBase, {"outputs", "ofloat"}}, {linkRecipient, {"inputs", "in_float"}}, false},
				{{linkBase, {"outputs", "ofloat"}}, {linkRecipient, {"inputs", "float"}}, true}});
		});
}

TEST_F(UndoTest, lua_module_added) {
	auto script = create<LuaScript>("script");
	commandInterface.set(ValueHandle{script, &raco::user_types::LuaScript::uri_}, test_path().append("scripts/moduleDependency.lua").string());

	auto module = create<LuaScriptModule>("module");
	commandInterface.set(ValueHandle{module, &raco::user_types::LuaScriptModule::uri_}, test_path().append("scripts/moduleDefinition.lua").string());

	checkUndoRedo([this, script, module]() {
		commandInterface.set(ValueHandle{script, {"luaModules", "coalas"}}, module);
	},
		[this, script, module]() {
			ASSERT_FALSE(commandInterface.errors().hasError({script}));
			ASSERT_TRUE(commandInterface.errors().hasError(ValueHandle{script, {"luaModules", "coalas"}}));
			auto coalasRef = ValueHandle{script, {"luaModules", "coalas"}}.asRef();
			ASSERT_EQ(coalasRef, nullptr);
		},
		[this, script, module]() {
			ASSERT_FALSE(commandInterface.errors().hasError({script}));
			ASSERT_FALSE(commandInterface.errors().hasError(ValueHandle{script, {"luaModules", "coalas"}}));
			auto coalasRef = ValueHandle{script, {"luaModules", "coalas"}}.asRef();
			ASSERT_EQ(coalasRef, coalasRef);
		});
}

TEST_F(UndoTest, lua_module_script_uri_changed) {
	auto script = create<LuaScript>("script");
	commandInterface.set(ValueHandle{script, &raco::user_types::LuaScript::uri_}, test_path().append("scripts/moduleDependency.lua").string());

	auto module = create<LuaScriptModule>("module");
	commandInterface.set(ValueHandle{module, &raco::user_types::LuaScriptModule::uri_}, test_path().append("scripts/moduleDefinition.lua").string());
	commandInterface.set(ValueHandle{script, {"luaModules", "coalas"}}, module);

	checkUndoRedo([this, script, module]() { commandInterface.set(ValueHandle{script, &raco::user_types::LuaScript::uri_}, test_path().append("scripts/types-scalar.lua").string()); },
		[this, script]() {
			ASSERT_FALSE(commandInterface.errors().hasError({script}));
			auto coalasRef = ValueHandle{script, {"luaModules", "coalas"}}.asRef();
			ASSERT_EQ(coalasRef, coalasRef);
		},
		[this, script]() {
			ASSERT_FALSE(commandInterface.errors().hasError({script}));
			ASSERT_EQ(script->get("luaModules")->asTable().size(), 0);
		});
}

TEST_F(UndoTest, lua_module_script_module_made_invalid) {
	auto script = create<LuaScript>("script");
	commandInterface.set(ValueHandle{script, &raco::user_types::LuaScript::uri_}, test_path().append("scripts/moduleDependency.lua").string());

	auto module = create<LuaScriptModule>("module");
	commandInterface.set(ValueHandle{module, &raco::user_types::LuaScriptModule::uri_}, test_path().append("scripts/moduleDefinition.lua").string());
	commandInterface.set(ValueHandle{script, {"luaModules", "coalas"}}, module);

	checkUndoRedo([this, script, module]() { commandInterface.set(ValueHandle{module, &raco::user_types::LuaScriptModule::uri_}, std::string()); },
		[this, script]() {
			ASSERT_FALSE(commandInterface.errors().hasError({script}));
			ASSERT_FALSE(commandInterface.errors().hasError(ValueHandle{script, {"luaModules", "coalas"}}));
			auto coalasRef = ValueHandle{script, {"luaModules", "coalas"}}.asRef();
			ASSERT_EQ(coalasRef, coalasRef);
		},
		[this, script, module]() {
			ASSERT_FALSE(commandInterface.errors().hasError({script}));
			ASSERT_TRUE(commandInterface.errors().hasError(ValueHandle{script, {"luaModules", "coalas"}}));
			ASSERT_TRUE(commandInterface.errors().hasError({module, &raco::user_types::LuaScriptModule::uri_}));
		});
}

TEST_F(UndoTest, link_quaternion_euler_change) {
	raco::utils::file::write((test_path() / "lua_script_out1.lua").string(), R"(
function interface(IN,OUT)
	IN.vec = Type:Vec4f()
	OUT.vec = Type:Vec4f()
end
function run(IN,OUT)
    OUT.vec = IN.vec
end
)");

	raco::utils::file::write((test_path() / "lua_script_out2.lua").string(), R"(
function interface(IN,OUT)
	IN.vec = Type:Vec3f()
	OUT.vec = Type:Vec3f()
end
function run(IN,OUT)
    OUT.vec = IN.vec
end
)");
	auto lua = create<LuaScript>("script1");
	commandInterface.set(ValueHandle{lua, {"uri"}}, test_path().append("lua_script_out1.lua").string());

	auto node = create<Node>("node");
	commandInterface.addLink(ValueHandle{lua, {"outputs", "vec"}}, ValueHandle{node, {"rotation"}});

	checkUndoRedo([this, lua, node]() { 
		commandInterface.set(ValueHandle{lua, {"uri"}}, test_path().append("lua_script_out2.lua").string());
	},
		[this, node, lua]() {
			ASSERT_EQ(project.links().size(), 1);
			ASSERT_TRUE((*project.links().begin())->isValid());
			ASSERT_TRUE(ValueHandle(node, {"rotation"}).isVec3f());
			ASSERT_TRUE(ValueHandle(lua, {"outputs", "vec"}).isVec4f());
		},
		[this, node, lua]() {
			ASSERT_EQ(project.links().size(), 1);
			ASSERT_TRUE((*project.links().begin())->isValid());
			ASSERT_TRUE(ValueHandle(node, {"rotation"}).isVec3f());
			ASSERT_TRUE(ValueHandle(lua, {"outputs", "vec"}).isVec3f());
		});
}


TEST_F(UndoTest, mesh_asset_with_anims_import_multiple_undo_redo) {
	raco::core::MeshDescriptor desc;
	desc.absPath = test_path().append("meshes/InterpolationTest/InterpolationTest.gltf").string();
	desc.bakeAllSubmeshes = false;

	auto [scenegraph, dummyCacheEntry] = raco::getMeshSceneGraphWithHandler(commandInterface.meshCache(), desc);
	
	commandInterface.insertAssetScenegraph(scenegraph, desc.absPath, nullptr);

	for (auto i = 0; i < 10; ++i) {
		commandInterface.undoStack().undo();
		ASSERT_EQ(project.links().size(), 0);
		commandInterface.undoStack().redo();
		ASSERT_EQ(project.links().size(), 9);
	}
}

class UndoTestWithIDParams : public UndoTestT<testing::TestWithParam<std::vector<std::string>>> {

};

TEST_P(UndoTestWithIDParams, animation_with_animation_channel) {
	auto channelID = GetParam()[0];
	auto animID = GetParam()[1];

	auto absPath = test_path().append("meshes/InterpolationTest/InterpolationTest.gltf").string();

	checkJump([this, absPath, channelID, animID]() { 
		auto channel = context.createObject(AnimationChannel::typeDescription.typeName, "channel", channelID);
		undoStack.push("Create channel object");
		commandInterface.set({channel, &AnimationChannel::uri_}, absPath);

		auto anim = context.createObject(Animation::typeDescription.typeName, "anim", animID);
		undoStack.push("Create animation object");

		EXPECT_EQ(anim->get("outputs")->asTable().size(), 0);
		commandInterface.set(ValueHandle(anim, &Animation::animationChannels_)[0], channel); 
		},

		[this]() {
		},

		[this]() {
			auto anim = getInstance<Animation>("anim");
			EXPECT_EQ(anim->get("outputs")->asTable().size(), 1);
		});
}

INSTANTIATE_TEST_SUITE_P(
	UndoTest,
	UndoTestWithIDParams,
	testing::Values(
		std::vector<std::string>({"AAA", "BBB"}),
		std::vector<std::string>({"BBB", "AAA"})));

TEST_F(UndoTest, meshnode_uniform_update) {
	TextFile vertShader = makeFile("shader.vert", R"(
#version 300 es
precision mediump float;
in vec3 a_Position;
uniform mat4 u_MVPMatrix;
void main() {
    float offset = float(gl_InstanceID) * 0.2;
    gl_Position = u_MVPMatrix * vec4(a_Position.x + offset, a_Position.yz, 1.0);
}
)");

	TextFile fragShader = makeFile("shader.frag", R"(
#version 300 es
precision mediump float;
out vec4 FragColor;
uniform vec3 u_color;
void main() {
    FragColor = vec4(u_color.r, u_color.g, u_color.b, 1.0);
})");


	std::string altFragShaderText = R"(
#version 300 es
precision mediump float;
out vec4 FragColor;
uniform vec3 alt_name;
void main() {
    FragColor = vec4(alt_name.r, alt_name.g, alt_name.b, 1.0);
})";

	auto mesh = create_mesh("mesh", "meshes/Duck.glb");

	size_t preIndex = undoStack.getIndex();

	auto material = context.createObject(Material::typeDescription.typeName, "material", "CCC");
	undoStack.push("Create object");
	auto meshnode = context.createObject(MeshNode::typeDescription.typeName, "meshnode", "BBB")->as<MeshNode>();
	undoStack.push("Create object");
	commandInterface.set({meshnode, {"mesh"}}, mesh);
	commandInterface.set({meshnode, {"materials", "material", "material"}}, material);
	commandInterface.set(meshnode->getMaterialPrivateHandle(0), true);
	commandInterface.set({material, &Material::uriVertex_}, vertShader);
	commandInterface.set({material, &Material::uriFragment_}, fragShader);

	size_t postIndex = undoStack.getIndex();

	auto postCheck = [this](const std::string& uniformName) {
		auto material = getInstance<Material>("material");
		auto meshnode = getInstance<MeshNode>("meshnode");
		ValueHandle matUniforms{material, {"uniforms"}};
		ValueHandle meshnodeUniforms{meshnode, {"materials", "material", "uniforms"}};
		EXPECT_TRUE(matUniforms.hasProperty(uniformName));
		EXPECT_TRUE(meshnodeUniforms.hasProperty(uniformName));
	};

	postCheck("u_color");

	undoStack.setIndex(preIndex);

	raco::utils::file::write(fragShader.path.string(), altFragShaderText);

	undoStack.setIndex(postIndex);

	postCheck("alt_name");
}

TEST_F(UndoTest, meshnode_uniform_value_update) {
	TextFile vertShader = makeFile("shader.vert", R"(
#version 300 es
precision mediump float;
in vec3 a_Position;
uniform mat4 u_MVPMatrix;
void main() {
    float offset = float(gl_InstanceID) * 0.2;
    gl_Position = u_MVPMatrix * vec4(a_Position.x + offset, a_Position.yz, 1.0);
}
)");

	TextFile fragShader = makeFile("shader.frag", R"(
#version 300 es
precision mediump float;
out vec4 FragColor;
uniform float u_color;
void main() {
})");

	std::string altFragShader = makeFile("altshader.frag", R"(
#version 300 es
precision mediump float;
out vec4 FragColor;
uniform float alt_name;
void main() {
})");

	auto mesh = create_mesh("mesh", "meshes/Duck.glb");

	auto material = context.createObject(Material::typeDescription.typeName, "material", "CCC");
	undoStack.push("Create object");
	auto meshnode = context.createObject(MeshNode::typeDescription.typeName, "meshnode", "BBB")->as<MeshNode>();
	undoStack.push("Create object");
	commandInterface.set({meshnode, {"mesh"}}, mesh);
	commandInterface.set({meshnode, {"materials", "material", "material"}}, material);
	commandInterface.set(meshnode->getMaterialPrivateHandle(0), true);
	commandInterface.set({material, &Material::uriVertex_}, vertShader);
	commandInterface.set({material, &Material::uriFragment_}, fragShader);

	size_t preIndex = undoStack.getIndex();

	commandInterface.set(raco::core::ValueHandle{meshnode, {"materials", "material", "uniforms", "u_color"}}, 5.0);

	size_t postIndex = undoStack.getIndex();

	undoStack.setIndex(preIndex);
	commandInterface.set({material, &Material::uriFragment_}, altFragShader);
	commandInterface.set({material, &Material::uriFragment_}, fragShader);

	undoStack.setIndex(preIndex);
	undoStack.setIndex(postIndex);
	ASSERT_NO_THROW((raco::core::ValueHandle{meshnode, {"materials", "material", "uniforms", "u_color"}}.asDouble()));	
}

#if (!defined(__linux__))
// awaitPreviewDirty does not work in Linux as expected. See RAOS-692

TEST_F(UndoTest, lua_resync_after_undo) {
	TextFile luaFile = makeFile("test.lua", R"(
function interface(IN,OUT)
    OUT.vec = Type:Vec3f()
end

function run(IN,OUT)
end
)");

	std::string altLuaScript = R"(
function interface(IN,OUT)
    OUT.renamed = Type:Vec3f()
end

function run(IN,OUT)
end
)";

	auto lua = create_lua("lua", luaFile);
	auto node = create<Node>("node");

	ValueHandle luaOutputs(lua, &LuaScript::outputs_);
	EXPECT_TRUE(luaOutputs.hasProperty("vec"));
	EXPECT_FALSE(luaOutputs.hasProperty("renamed"));

	recorder.reset();
	raco::utils::file::write(luaFile.path.string(), altLuaScript);
	EXPECT_TRUE(raco::awaitPreviewDirty(recorder, lua));

	EXPECT_FALSE(luaOutputs.hasProperty("vec"));
	EXPECT_TRUE(luaOutputs.hasProperty("renamed"));

	// undo node creation
	commandInterface.undoStack().undo();
	EXPECT_FALSE(luaOutputs.hasProperty("vec"));
	EXPECT_TRUE(luaOutputs.hasProperty("renamed"));

	commandInterface.undoStack().redo();
	EXPECT_FALSE(luaOutputs.hasProperty("vec"));
	EXPECT_TRUE(luaOutputs.hasProperty("renamed"));
}


TEST_F(UndoTest, link_remove_break_on_undo) {
	TextFile luaFile = makeFile("test.lua", R"(
function interface(IN,OUT)
    OUT.vec = Type:Vec3f()
end

function run(IN,OUT)
end
)");

	std::string altLuaScript = R"(
function interface(IN,OUT)
    OUT.renamed = Type:Vec3f()
end

function run(IN,OUT)
end
)";

	auto lua = create_lua("lua", luaFile);
	auto node = create<Node>("node");

	// #1: create link
	auto [sprop, eprop] = link(lua, {"outputs", "vec"}, node, {"translation"});
	checkLinks({{sprop, eprop, true}});

	// #2: remove link
	commandInterface.removeLink(eprop);
	checkLinks({});

	ValueHandle luaOutputs(lua, &LuaScript::outputs_);
	EXPECT_TRUE(luaOutputs.hasProperty("vec"));
	EXPECT_FALSE(luaOutputs.hasProperty("renamed"));

	recorder.reset();
	raco::utils::file::write(luaFile.path.string(), altLuaScript);
	EXPECT_TRUE(raco::awaitPreviewDirty(recorder, lua));

	EXPECT_FALSE(luaOutputs.hasProperty("vec"));
	EXPECT_TRUE(luaOutputs.hasProperty("renamed"));

	// undo #2
	commandInterface.undoStack().undo();
	checkLinks({{sprop, eprop, false}});
	ASSERT_TRUE(commandInterface.errors().hasError({node}));

	// undo #1
	commandInterface.undoStack().undo();
	checkLinks({});
	ASSERT_FALSE(commandInterface.errors().hasError({node}));

	// redo #1 
	commandInterface.undoStack().redo();
	checkLinks({{sprop, eprop, false}});
	ASSERT_TRUE(commandInterface.errors().hasError({node}));

	// redo #2
	commandInterface.undoStack().redo();
	checkLinks({});
	ASSERT_FALSE(commandInterface.errors().hasError({node}));
}

TEST_F(UndoTest, lua_link_create_inconsistent_undo_stack) {
	TextFile luaFile = makeFile("test.lua", R"(
function interface(IN,OUT)
    OUT.vec = Type:Vec3f()
end

function run(IN,OUT)
end
)");

	std::string altLuaScript = R"(
function interface(IN,OUT)
    OUT.renamed = Type:Vec3f()
end

function run(IN,OUT)
end
)";

	auto lua = create_lua("lua", luaFile);
	auto node = create<Node>("node");

	ValueHandle luaOutputs(lua, &LuaScript::outputs_);
	EXPECT_TRUE(luaOutputs.hasProperty("vec"));
	EXPECT_FALSE(luaOutputs.hasProperty("renamed"));

	recorder.reset();
	raco::utils::file::write(luaFile.path.string(), altLuaScript);
	EXPECT_TRUE(raco::awaitPreviewDirty(recorder, lua));

	EXPECT_FALSE(luaOutputs.hasProperty("vec"));
	EXPECT_TRUE(luaOutputs.hasProperty("renamed"));

	// force restore from undo stack
	commandInterface.undoStack().setIndex(commandInterface.undoStack().getIndex(), true);
	EXPECT_FALSE(luaOutputs.hasProperty("vec"));
	EXPECT_TRUE(luaOutputs.hasProperty("renamed"));

	auto [sprop, eprop] = link(lua, {"outputs", "renamed"}, node, {"translation"});
	checkLinks({{sprop, eprop, true}});

	{
		Project& stackProject = stack().back()->state;

		auto stackLua = getInstance<LuaScript>(stackProject, "lua");
		ASSERT_EQ(stackProject.links().size(), 1);
		auto stackLink = *stackProject.links().begin();

		ValueHandle startProp{stackLua, stackLink->startPropertyNamesVector()};
		EXPECT_TRUE(startProp && *stackLink->isValid_ || !startProp && !*stackLink->isValid_);
	}
}

TEST_F(UndoTest, link_redo_creates_impossible_link) {
	std::string origLuaScript = R"(
function interface(IN,OUT)
    OUT.vec = Type:Vec3f()
end

function run(IN,OUT)
end
)";

	std::string altLuaScript = R"(
function interface(IN,OUT)
    OUT.renamed = Type:Vec3f()
end

function run(IN,OUT)
end
)";

	TextFile luaFile = makeFile("test.lua", origLuaScript);
	auto lua = create_lua("lua", luaFile);
	auto node = create<Node>("node");

	ValueHandle luaOutputs(lua, &LuaScript::outputs_);
	EXPECT_TRUE(luaOutputs.hasProperty("vec"));
	EXPECT_FALSE(luaOutputs.hasProperty("renamed"));

	recorder.reset();
	raco::utils::file::write(luaFile.path.string(), altLuaScript);
	EXPECT_TRUE(raco::awaitPreviewDirty(recorder, lua));

	EXPECT_FALSE(luaOutputs.hasProperty("vec"));
	EXPECT_TRUE(luaOutputs.hasProperty("renamed"));

	// force restore from undo stack
	commandInterface.undoStack().setIndex(commandInterface.undoStack().getIndex(), true);
	EXPECT_FALSE(luaOutputs.hasProperty("vec"));
	EXPECT_TRUE(luaOutputs.hasProperty("renamed"));

	auto [sprop, eprop] = link(lua, {"outputs", "renamed"}, node, {"translation"});
	checkLinks({{sprop, eprop, true}});

	raco::utils::file::write(luaFile.path.string(), origLuaScript);
	EXPECT_TRUE(raco::awaitPreviewDirty(recorder, lua));

	undoStack.undo();
	checkLinks({});

	undoStack.redo();
	checkLinks({{sprop, eprop, false}});
	ASSERT_FALSE(static_cast<bool>(ValueHandle(sprop)));
}

#endif

TEST_F(UndoTest, add_remove_property_ref) {
	auto refTarget = create<Foo>("foo");
	auto refSource = create<ObjectWithTableProperty>("sourceObject");
	ValueHandle tableHandle{refSource, &ObjectWithTableProperty::t_};
	
	checkUndoRedoMultiStep<2>(
		{[this, tableHandle, refTarget]() {
			 context.addProperty(tableHandle, "ref", std::make_unique<Value<SEditorObject>>(refTarget));
			 this->undoStack.push("step 1");
		 },
			[this, tableHandle]() {
				context.removeProperty(tableHandle, "ref");
				this->undoStack.push("step 2");
			}},
		{[this, refTarget]() {
			 EXPECT_EQ(refTarget->referencesToThis().size(), 0);
		 },
			[this, refTarget, refSource]() {
				ASSERT_EQ(refTarget->referencesToThis().size(), 1);
				EXPECT_EQ(refTarget->referencesToThis().begin()->lock(), refSource);
			},
			[this, refTarget]() {
				EXPECT_EQ(refTarget->referencesToThis().size(), 0);
			}});
}

TEST_F(UndoTest, add_remove_multiple_property_ref) {
	auto refTarget = create<Foo>("foo");
	auto refSource = create<ObjectWithTableProperty>("sourceObject");
	ValueHandle tableHandle{refSource, &ObjectWithTableProperty::t_};

	checkUndoRedoMultiStep<4>(
		{[this, tableHandle, refTarget]() {
			 context.addProperty(tableHandle, "ref1", std::make_unique<Value<SEditorObject>>(refTarget));
			 this->undoStack.push("step 1");
		 },
			[this, tableHandle, refTarget]() {
				context.addProperty(tableHandle, "ref2", std::make_unique<Value<SEditorObject>>(refTarget));
				this->undoStack.push("step 2");
			},
			[this, tableHandle]() {
				context.removeProperty(tableHandle, "ref1");
				this->undoStack.push("step 3");
			},
			[this, tableHandle]() {
				context.removeProperty(tableHandle, "ref2");
				this->undoStack.push("step 4");
			}},
		{[this, refTarget]() {
			 EXPECT_EQ(refTarget->referencesToThis().size(), 0);
		 },
			[this, refTarget, refSource]() {
				ASSERT_EQ(refTarget->referencesToThis().size(), 1);
				EXPECT_EQ(refTarget->referencesToThis().begin()->lock(), refSource);
			},
			[this, refTarget, refSource]() {
				ASSERT_EQ(refTarget->referencesToThis().size(), 1);
				EXPECT_EQ(refTarget->referencesToThis().begin()->lock(), refSource);
			},
			[this, refTarget, refSource]() {
				ASSERT_EQ(refTarget->referencesToThis().size(), 1);
				EXPECT_EQ(refTarget->referencesToThis().begin()->lock(), refSource);
			},
			[this, refTarget]() {
				EXPECT_EQ(refTarget->referencesToThis().size(), 0);
			}});
}

TEST_F(UndoTest, add_remove_property_table_with_ref) {
	auto refTarget = create<Foo>("foo");
	auto refSource = create<ObjectWithTableProperty>("sourceObject");
	ValueHandle tableHandle{refSource, &ObjectWithTableProperty::t_};

	checkUndoRedoMultiStep<2>(
		{[this, tableHandle, refTarget]() {
			 Table innerTable;
			 innerTable.addProperty("ref", std::make_unique<Value<SEditorObject>>(refTarget));
			 context.addProperty(tableHandle, "innerTable", std::make_unique<Value<Table>>(innerTable));
			 this->undoStack.push("step 1");
		 },
			[this, tableHandle]() {
				context.removeProperty(tableHandle, "innerTable");
				this->undoStack.push("step 2");
			}},
		{[this, refTarget]() {
			 EXPECT_EQ(refTarget->referencesToThis().size(), 0);
		 },
			[this, refTarget, refSource]() {
				ASSERT_EQ(refTarget->referencesToThis().size(), 1);
				EXPECT_EQ(refTarget->referencesToThis().begin()->lock(), refSource);
			},
			[this, refTarget]() {
				EXPECT_EQ(refTarget->referencesToThis().size(), 0);
			}});
}

TEST_F(UndoTest, add_remove_property_struct_with_ref) {
	auto refTarget = create<Foo>("foo");
	auto refSource = create<ObjectWithTableProperty>("sourceObject");
	ValueHandle tableHandle{refSource, &ObjectWithTableProperty::t_};

	checkUndoRedoMultiStep<2>(
		{[this, tableHandle, refTarget]() {
			 Value<StructWithRef>* value{new Value<StructWithRef>()};
			 (*value)->ref = refTarget;
			 context.addProperty(tableHandle, "innerStruct", std::unique_ptr<Value<StructWithRef>>(value));
			 this->undoStack.push("step 1");
		 },
			[this, tableHandle]() {
				context.removeProperty(tableHandle, "innerStruct");
				this->undoStack.push("step 2");
			}},
		{[this, refTarget]() {
			 EXPECT_EQ(refTarget->referencesToThis().size(), 0);
		 },
			[this, refTarget, refSource]() {
				ASSERT_EQ(refTarget->referencesToThis().size(), 1);
				EXPECT_EQ(refTarget->referencesToThis().begin()->lock(), refSource);
			},
			[this, refTarget]() {
				EXPECT_EQ(refTarget->referencesToThis().size(), 0);
			}});
}

TEST_F(UndoTest, setTable_with_ref) {
	auto refTarget = create<Foo>("foo");
	auto refSource = create<ObjectWithTableProperty>("sourceObject");
	ValueHandle tableHandle{refSource, &ObjectWithTableProperty::t_};

	checkUndoRedoMultiStep<2>(
		{[this, tableHandle, refTarget]() {
			 Table newTable;
			 newTable.addProperty("ref", std::make_unique<Value<SEditorObject>>(refTarget));
			 context.set(tableHandle, newTable);
			 this->undoStack.push("step 1");
		 },
			[this, tableHandle]() {
				Table emptyTable;
				context.set(tableHandle, emptyTable);
				this->undoStack.push("step 2");
			}},
		{[this, refTarget]() {
			 EXPECT_EQ(refTarget->referencesToThis().size(), 0);
		 },
			[this, refTarget, refSource]() {
				ASSERT_EQ(refTarget->referencesToThis().size(), 1);
				EXPECT_EQ(refTarget->referencesToThis().begin()->lock(), refSource);
			},
			[this, refTarget]() {
				EXPECT_EQ(refTarget->referencesToThis().size(), 0);
			}});
}

TEST_F(UndoTest, setArray_with_ref) {
	auto refTarget = create<Foo>("foo");
	auto refSource = create<ObjectWithTableProperty>("sourceObject");
	ValueHandle arrayHandle{refSource, &ObjectWithTableProperty::array_};

	checkUndoRedoMultiStep<2>(
		{[this, arrayHandle, refTarget]() {
			 Table newTable;
			 newTable.addProperty("ref", std::make_unique<Value<SEditorObject>>(refTarget));
			 context.set(arrayHandle, newTable);
			 this->undoStack.push("step 1");
		 },
			[this, arrayHandle]() {
				Table emptyTable;
				context.set(arrayHandle, emptyTable);
				this->undoStack.push("step 2");
			}},
		{[this, refTarget]() {
			 EXPECT_EQ(refTarget->referencesToThis().size(), 0);
		 },
			[this, refTarget, refSource]() {
				ASSERT_EQ(refTarget->referencesToThis().size(), 1);
				EXPECT_EQ(refTarget->referencesToThis().begin()->lock(), refSource);
			},
			[this, refTarget]() {
				EXPECT_EQ(refTarget->referencesToThis().size(), 0);
			}});
}

TEST_F(UndoTest, setStruct_with_ref) {
	auto refTarget = create<Foo>("foo");
	auto refSource = create<ObjectWithStructProperty>("sourceObject");
	const ValueHandle structHandle{refSource, &ObjectWithStructProperty::s_};

	checkUndoRedoMultiStep<2>(
		{[this, structHandle, refTarget]() {
			 StructWithRef newStruct;
			 newStruct.ref = refTarget;

			 context.set(structHandle, newStruct);
			 this->undoStack.push("step 1");
		 },
			[this, structHandle]() {
				StructWithRef emptyStruct;
				context.set(structHandle, emptyStruct);
				this->undoStack.push("step 2");
			}},
		{[this, refTarget]() {
			 EXPECT_EQ(refTarget->referencesToThis().size(), 0);
		 },
			[this, refTarget, refSource]() {
				ASSERT_EQ(refTarget->referencesToThis().size(), 1);
				EXPECT_EQ(refTarget->referencesToThis().begin()->lock(), refSource);
			},
			[this, refTarget]() {
				EXPECT_EQ(refTarget->referencesToThis().size(), 0);
			}});
}

TEST_F(UndoTest, setTable_nested_table_with_ref) {
	auto refTarget = create<Foo>("foo");
	auto refSource = create<ObjectWithTableProperty>("sourceObject");
	ValueHandle tableHandle{refSource, &ObjectWithTableProperty::t_};

	checkUndoRedoMultiStep<2>(
		{[this, tableHandle, refTarget]() {
			 Table innerTable;
			 innerTable.addProperty("ref", std::make_unique<Value<SEditorObject>>(refTarget));

			 Table outerTable;
			 outerTable.addProperty("inner", std::make_unique<Value<Table>>(innerTable));

			 context.set(tableHandle, outerTable);
			 this->undoStack.push("step 1");
		 },
			[this, tableHandle]() {
				Table emptyTable;
				context.set(tableHandle, emptyTable);
				this->undoStack.push("step 2");
			}},
		{[this, refTarget]() {
			 EXPECT_EQ(refTarget->referencesToThis().size(), 0);
		 },
			[this, refTarget, refSource]() {
				ASSERT_EQ(refTarget->referencesToThis().size(), 1);
				EXPECT_EQ(refTarget->referencesToThis().begin()->lock(), refSource);
			},
			[this, refTarget]() {
				EXPECT_EQ(refTarget->referencesToThis().size(), 0);
			}});
}

TEST_F(UndoTest, setArray_nested_table_with_ref) {
	auto refTarget = create<Foo>("foo");
	auto refSource = create<ObjectWithTableProperty>("sourceObject");
	ValueHandle tableHandle{refSource, &ObjectWithTableProperty::array_};

	checkUndoRedoMultiStep<2>(
		{[this, tableHandle, refTarget]() {
			 Table innerTable;
			 innerTable.addProperty("ref", std::make_unique<Value<SEditorObject>>(refTarget));

			 Table outerTable;
			 outerTable.addProperty("inner", std::make_unique<Value<Table>>(innerTable));

			 context.set(tableHandle, outerTable);
			 this->undoStack.push("step 1");
		 },
			[this, tableHandle]() {
				Table emptyTable;
				context.set(tableHandle, emptyTable);
				this->undoStack.push("step 2");
			}},
		{[this, refTarget]() {
			 EXPECT_EQ(refTarget->referencesToThis().size(), 0);
		 },
			[this, refTarget, refSource]() {
				ASSERT_EQ(refTarget->referencesToThis().size(), 1);
				EXPECT_EQ(refTarget->referencesToThis().begin()->lock(), refSource);
			},
			[this, refTarget]() {
				EXPECT_EQ(refTarget->referencesToThis().size(), 0);
			}});
}

TEST_F(UndoTest, setTable_nested_struct_with_ref) {
	auto refTarget = create<Foo>("foo");
	auto refSource = create<ObjectWithTableProperty>("sourceObject");
	ValueHandle tableHandle{refSource, &ObjectWithTableProperty::t_};

	checkUndoRedoMultiStep<2>(
		{[this, tableHandle, refTarget]() {
			 Value<StructWithRef>* value{new Value<StructWithRef>()};
			 (*value)->ref = refTarget;

			 Table newTable;
			 newTable.addProperty("struct", std::unique_ptr<Value<StructWithRef>>(value));

			 context.set(tableHandle, newTable);
			 this->undoStack.push("step 1");
		 },
			[this, tableHandle]() {
				Table emptyTable;
				context.set(tableHandle, emptyTable);
				this->undoStack.push("step 2");
			}},
		{[this, refTarget]() {
			 EXPECT_EQ(refTarget->referencesToThis().size(), 0);
		 },
			[this, refTarget, refSource]() {
				ASSERT_EQ(refTarget->referencesToThis().size(), 1);
				EXPECT_EQ(refTarget->referencesToThis().begin()->lock(), refSource);
			},
			[this, refTarget]() {
				EXPECT_EQ(refTarget->referencesToThis().size(), 0);
			}});
}

TEST_F(UndoTest, setArray_nested_struct_with_ref) {
	auto refTarget = create<Foo>("foo");
	auto refSource = create<ObjectWithTableProperty>("sourceObject");
	ValueHandle tableHandle{refSource, &ObjectWithTableProperty::array_};

	checkUndoRedoMultiStep<2>(
		{[this, tableHandle, refTarget]() {
			 Value<StructWithRef>* value{new Value<StructWithRef>()};
			 (*value)->ref = refTarget;

			 Table newTable;
			 newTable.addProperty("struct", std::unique_ptr<Value<StructWithRef>>(value));

			 context.set(tableHandle, newTable);
			 this->undoStack.push("step 1");
		 },
			[this, tableHandle]() {
				Table emptyTable;
				context.set(tableHandle, emptyTable);
				this->undoStack.push("step 2");
			}},
		{[this, refTarget]() {
			 EXPECT_EQ(refTarget->referencesToThis().size(), 0);
		 },
			[this, refTarget, refSource]() {
				ASSERT_EQ(refTarget->referencesToThis().size(), 1);
				EXPECT_EQ(refTarget->referencesToThis().begin()->lock(), refSource);
			},
			[this, refTarget]() {
				EXPECT_EQ(refTarget->referencesToThis().size(), 0);
			}});
}

TEST_F(UndoTest, referenced_object_moved_into_prefab) {
	auto prefab = create<Prefab>("prefab");
	auto camera = create<PerspectiveCamera>("camera");

	auto renderPass = create<RenderPass>("pass");
	auto refTargets = raco::core::Queries::findAllValidReferenceTargets(*commandInterface.project(), {renderPass, &raco::user_types::RenderPass::camera_});
	ASSERT_EQ(refTargets, std::vector<SEditorObject>{camera});

	commandInterface.set({renderPass, &raco::user_types::RenderPass::camera_}, camera);
	ASSERT_EQ(raco::core::ValueHandle(renderPass, &raco::user_types::RenderPass::camera_).asRef(), camera);

	commandInterface.moveScenegraphChildren({camera}, prefab);
	ASSERT_EQ(raco::core::ValueHandle(renderPass, &raco::user_types::RenderPass::camera_).asRef(), SEditorObject());

	commandInterface.undoStack().undo();
	ASSERT_EQ(raco::core::ValueHandle(renderPass, &raco::user_types::RenderPass::camera_).asRef(), camera);

	commandInterface.undoStack().redo();
	ASSERT_EQ(raco::core::ValueHandle(renderPass, &raco::user_types::RenderPass::camera_).asRef(), SEditorObject());
}