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
#include "core/ExternalReferenceAnnotation.h"
#include "core/Handles.h"
#include "core/MeshCacheInterface.h"
#include "core/Project.h"
#include "core/Queries.h"
#include "ramses_base/HeadlessEngineBackend.h"
#include "testing/RacoBaseTest.h"
#include "testing/TestEnvironmentCore.h"
#include "testing/TestUtil.h"
#include "user_types/UserObjectFactory.h"
#include "application/RaCoProject.h"
#include "application/RaCoApplication.h"
#include "ramses_adaptor/SceneBackend.h"

#include "user_types/Mesh.h"
#include "user_types/MeshNode.h"
#include "user_types/Node.h"
#include "user_types/Prefab.h"
#include "user_types/PrefabInstance.h"

#include "gtest/gtest.h"

#include "utils/stdfilesystem.h"

using namespace raco::core;
using namespace raco::user_types;

using raco::application::RaCoApplication;

class ExtrefTest : public RacoBaseTest<> {
public:
	template <class C>
	std::shared_ptr<C> create(std::string name, SEditorObject parent = nullptr) {
		auto obj = std::dynamic_pointer_cast<C>(cmd->createObject(C::typeDescription.typeName, name));
		if (parent) {
			cmd->moveScenegraphChild(obj, parent);
		}
		return obj;
	}

	SMesh create_mesh(const std::string &name, const std::string &relpath) {
		auto mesh = create<Mesh>(name);
		cmd->set({mesh, {"uri"}}, (cwd_path() / relpath).string());
		return mesh;
	}

	SMaterial create_material(const std::string &name, const std::string &relpathVertex, const std::string &relpathFragment) {
		auto material = create<Material>(name);
		cmd->set({material, {"uriVertex"}}, (cwd_path() / relpathVertex).string());
		cmd->set({material, {"uriFragment"}}, (cwd_path() / relpathFragment).string());
		return material;
	}

	SMeshNode create_meshnode(const std::string &name, SMesh mesh, SMaterial material, SEditorObject parent = nullptr) {
		auto meshnode = create<MeshNode>(name, parent);
		cmd->set({meshnode, {"mesh"}}, mesh);
		cmd->set({meshnode, {"materials", "material", "material"}}, material);
		return meshnode;
	}

	void change_uri(SEditorObject obj, const std::string &newvalue) {
		cmd->set({obj, {"uri"}}, (cwd_path() / newvalue).string());
	}

	void rename_project(const std::string &newProjectName) {
		if (!newProjectName.empty()) {
			cmd->set({app->activeRaCoProject().project()->settings(), {"objectName"}}, newProjectName);
		}
	}

	SEditorObject find(const std::string& name) {
		auto obj = Queries::findByName(project->instances(), name);
		EXPECT_TRUE(obj != nullptr);
		return obj;
	}

	void dontFind(const std::string &name) {
		auto obj = Queries::findByName(project->instances(), name);
		EXPECT_TRUE(obj == nullptr);
	}

	template<typename T = EditorObject>
	std::shared_ptr<T> findExt(const std::string& name, const std::string& projectID = std::string()) {
		SEditorObject editorObj = Queries::findByName(project->instances(), name);
		auto anno = editorObj->query<raco::core::ExternalReferenceAnnotation>();
		EXPECT_TRUE(anno != nullptr);
		if (!projectID.empty()) {
			EXPECT_EQ(*anno->projectID_, projectID);
		}
		auto objAsT = editorObj->as<T>();
		EXPECT_TRUE(objAsT != nullptr);
		return objAsT;
	}

	raco::ramses_base::HeadlessEngineBackend backend{};

	RaCoApplication *app;
	Project *project;
	CommandInterface *cmd;

	std::string setupBase(const std::string &basePathName, std::function<void()> func, const std::string& baseProjectName = std::string("base")) {
		RaCoApplication base{backend};
		app = &base;
		project = base.activeRaCoProject().project();
		cmd = base.activeRaCoProject().commandInterface();

		rename_project(baseProjectName);

		func();

		base.activeRaCoProject().saveAs(basePathName.c_str());
		return project->projectID();
	}

	std::string setupGeneric(std::function<void()> func) {
		RaCoApplication app_{backend};
		app = &app_;
		project = app_.activeRaCoProject().project();
		cmd = app_.activeRaCoProject().commandInterface();

		func();
		return project->projectID();
	}

	std::vector<SEditorObject> pasteFromExt(const std::string &basePathName, const std::vector<std::string> &externalObjectNames, bool asExtref,  bool* outSuccess = nullptr) {
		std::vector<std::string> stack;
		stack.emplace_back(project->currentPath());
		bool status = app->externalProjects()->addExternalProject(basePathName.c_str(), stack);
		if (!status) {
			if (outSuccess) {
				*outSuccess = false;
			}
			return std::vector<SEditorObject>();
		}
		auto originProject = app->externalProjects()->getExternalProject(basePathName);
		auto originCmd = app->externalProjects()->getExternalProjectCommandInterface(basePathName);

		std::vector<SEditorObject> origin;
		for (auto name : externalObjectNames) {
			if (auto obj = Queries::findByName(originProject->instances(), name)) {
				origin.emplace_back(obj);
			}
		}

		return cmd->pasteObjects(originCmd->copyObjects(origin), nullptr, asExtref, outSuccess);
	}

	std::string setupComposite(const std::string &basePathName, const std::string &compositePathName, const std::vector<std::string> &externalObjectNames,
		std::function<void()> func, const std::string& projectName = std::string()) {
		RaCoApplication app_{backend};
		app = &app_;
		project = app->activeRaCoProject().project();
		cmd = app->activeRaCoProject().commandInterface();

		rename_project(projectName);

		pasteFromExt(basePathName, externalObjectNames, true);

		func();

		if (!compositePathName.empty()) {
			app->activeRaCoProject().saveAs(compositePathName.c_str());
		}
		return project->projectID();
	}

	void updateBase(const std::string &basePathName, std::function<void()> func) {
		RaCoApplication base{backend, basePathName.c_str()};
		app = &base;
		project = base.activeRaCoProject().project();
		cmd = base.activeRaCoProject().commandInterface();

		func();

		base.activeRaCoProject().save();
	}

	void updateComposite(const std::string &pathName, std::function<void()> func) {
		RaCoApplication app_{backend, pathName.c_str()};
		app = &app_;
		project = app->activeRaCoProject().project();
		cmd = app->activeRaCoProject().commandInterface();

		func();
	}
};

TEST_F(ExtrefTest, normal_paste) {
	auto basePathName{(cwd_path() / "base.rcp").string()};

	setupBase(basePathName, [this]() {
		auto prefab = create<Prefab>("Prefab");
		auto node = create<Node>("prefab_child", prefab);
	});

	setupGeneric([this, basePathName]() {
		pasteFromExt(basePathName, {"Prefab"}, false);

		auto prefab = find("Prefab");
		auto node = find("prefab_child");
		ASSERT_EQ(prefab->children_->asVector<SEditorObject>(), std::vector<SEditorObject>({node}));
	});
}

TEST_F(ExtrefTest, duplicate_normal_paste) {
	auto basePathName{(cwd_path() / "base.rcp").string()};

	setupBase(basePathName, [this]() {
		auto prefab = create<Prefab>("Prefab");
	});

	setupGeneric([this, basePathName]() {
		std::vector<std::string> stack;
		stack.emplace_back(project->currentPath());
		app->externalProjects()->addExternalProject(basePathName.c_str(), stack);
		auto originProject = app->externalProjects()->getExternalProject(basePathName);
		auto originCmd = app->externalProjects()->getExternalProjectCommandInterface(basePathName);
		auto originPrefab = Queries::findByName(originProject->instances(), "Prefab");

		auto pasted1 = cmd->pasteObjects(originCmd->copyObjects({originPrefab}));
		auto pasted2 = cmd->pasteObjects(originCmd->copyObjects({originPrefab}));

		auto prefab1 = find("Prefab");
		auto prefab2 = find("Prefab (1)");
	});
}


TEST_F(ExtrefTest, extref_paste_empty_projectname) {
	auto basePathName{(cwd_path() / "base.rcp").string()};

	auto base_id = setupBase(basePathName, [this]() {
		auto prefab = create<Prefab>("Prefab");
	}, std::string());
	
	setupGeneric([this, basePathName, base_id]() {
		bool success = true;
		pasteFromExt(basePathName, {"Prefab"}, true, &success);
		ASSERT_TRUE(success);

		ASSERT_TRUE(project->hasExternalProjectMapping(base_id));
	});
}

TEST_F(ExtrefTest, extref_paste_fail_existing_object_from_same_project) {
	auto basePathName{(cwd_path() / "base.rcp").string()};

	setupBase(basePathName, [this]() {
		auto prefab = create<Prefab>("Prefab");
	});

	updateComposite(basePathName, [this, basePathName]() {
		bool success = true;
		pasteFromExt(basePathName, {"Prefab"}, true, &success);
		ASSERT_FALSE(success);
	});
}

TEST_F(ExtrefTest, extref_paste_fail_deleted_object_from_same_project) {
	auto basePathName{(cwd_path() / "base.rcp").string()};

	setupBase(basePathName, [this]() {
		auto prefab = create<Prefab>("Prefab");
	});

	updateComposite(basePathName, [this, basePathName]() {
		auto prefab = find("Prefab");
		cmd->deleteObjects({prefab});
		dontFind("Prefab");

		bool success = true;
		pasteFromExt(basePathName, {"Prefab"}, true, &success);
		ASSERT_FALSE(success);
	});
}

TEST_F(ExtrefTest, extref_paste_fail_existing_object_from_same_project_path) {
	auto basePathName{(cwd_path() / "base.rcp").string()};

	setupBase(basePathName, [this]() {
		auto prefab = create<Prefab>("Prefab");
	});

	updateComposite(basePathName, [this, basePathName]() {
		rename_project("not_base_anymore");
		bool success = true;
		pasteFromExt(basePathName, {"Prefab"}, true, &success);
		ASSERT_FALSE(success);
	});
}

TEST_F(ExtrefTest, extref_paste_fail_deleted_object_from_same_project_path) {
	auto basePathName{(cwd_path() / "base.rcp").string()};

	setupBase(basePathName, [this]() {
		auto prefab = create<Prefab>("Prefab");
	});

	updateComposite(basePathName, [this, basePathName]() {
		rename_project("not_base_anymore");

		auto prefab = find("Prefab");
		cmd->deleteObjects({prefab});
		dontFind("Prefab");

		bool success = true;
		pasteFromExt(basePathName, {"Prefab"}, true, &success);
		ASSERT_FALSE(success);
	});
}

TEST_F(ExtrefTest, extref_paste) {
	auto basePathName{(cwd_path() / "base.rcp").generic_string()};

	auto base_id = setupBase(basePathName, [this]() {
		auto prefab = create<Prefab>("prefab");
		auto node = create<Node>("prefab_child", prefab);
	});

	setupComposite(basePathName, "", {"prefab"}, [this, basePathName, base_id]() {
		auto prefab = findExt("prefab");
		auto node = findExt("prefab_child");
		ASSERT_EQ(prefab->children_->asVector<SEditorObject>(), std::vector<SEditorObject>({node}));

		ASSERT_EQ(project->lookupExternalProjectPath(base_id), basePathName);
	});
}

TEST_F(ExtrefTest, extref_paste_duplicate_projname) {
	auto basePathName1{(cwd_path() / "base1.rcp").generic_string()};
	auto basePathName2{(cwd_path() / "base2.rcp").generic_string()};

	auto base1_id = setupBase(basePathName1, [this]() {
		auto prefab = create<Prefab>("Prefab");
	}, std::string("base"));

	auto base2_id = setupBase(basePathName2, [this]() {
		auto mesh = create<Mesh>("mesh");
	}, std::string("base"));

	setupGeneric([this, basePathName1, basePathName2, base1_id, base2_id]() {
		bool success = true;
		pasteFromExt(basePathName1, {"Prefab"}, true, &success);
		ASSERT_TRUE(success);
		ASSERT_EQ(project->lookupExternalProjectPath(base1_id), basePathName1);
		pasteFromExt(basePathName2, {"mesh"}, true, &success);
		ASSERT_TRUE(success);
		ASSERT_EQ(project->lookupExternalProjectPath(base2_id), basePathName2);
	});
}

TEST_F(ExtrefTest, filecopy_paste_fail_same_object) {
	auto basePathName1{(cwd_path() / "base1.rcp").generic_string()};
	auto basePathName2{(cwd_path() / "base2.rcp").generic_string()};

	setupBase(basePathName1, [this]() {
		auto mesh = create<Mesh>("mesh");
	}, std::string("base"));

	std::filesystem::copy(basePathName1, basePathName2);
	updateBase(basePathName2, [this]() {
		rename_project("copy");
	});

	setupGeneric([this, basePathName1, basePathName2]() {
		bool success = true;
		pasteFromExt(basePathName1, {"mesh"}, true, &success);
		ASSERT_TRUE(success);

		pasteFromExt(basePathName2, {"mesh"}, true, &success);
		ASSERT_FALSE(success);
	});
}

TEST_F(ExtrefTest, filecopy_paste_fail_different_object) {
	auto basePathName1{(cwd_path() / "base1.rcp").generic_string()};
	auto basePathName2{(cwd_path() / "base2.rcp").generic_string()};
	auto compositePathName{(cwd_path() / "composite.rcp").generic_string()};

	setupBase(basePathName1, [this]() {
		auto mesh = create<Mesh>("mesh");
		auto prefab = create<Prefab>("prefab");
		auto meshnode = create<MeshNode>("meshnode", prefab);
	}, std::string("base"));

	std::filesystem::copy(basePathName1, basePathName2);
	updateBase(basePathName2, [this]() {
		rename_project("copy");
	});

	setupBase(compositePathName, [this, basePathName1, basePathName2]() {
		bool success = true;
		pasteFromExt(basePathName1, {"prefab"}, true, &success);
		ASSERT_TRUE(success);

		pasteFromExt(basePathName2, {"mesh"}, true, &success);
		ASSERT_FALSE(success);
	}, "composite");
}


TEST_F(ExtrefTest, extref_paste_same_project_name_after_delete_with_undo) {
	auto basePathName1{(cwd_path() / "base1.rcp").generic_string()};
	auto basePathName2{(cwd_path() / "base2.rcp").generic_string()};

	auto base_id1 = setupBase(basePathName1, [this]() {
		auto prefab = create<Prefab>("Prefab");
	}, std::string("base"));

	auto base_id2 = setupBase(basePathName2, [this]() {
		auto mesh = create<Mesh>("mesh");
	}, std::string("base"));

	setupGeneric([this, basePathName1, basePathName2, base_id1, base_id2]() {
		std::vector<SEditorObject> pasted;

		checkUndoRedoMultiStep<3>(*cmd,
			{[this, basePathName1, &pasted]() {
				 bool success = true;
				 pasted = pasteFromExt(basePathName1, {"Prefab"}, true, &success);
				 ASSERT_TRUE(success);
			 },
				[this, basePathName1, base_id1, &pasted]() {
					cmd->deleteObjects(pasted);
					ASSERT_FALSE(project->hasExternalProjectMapping(base_id1));
				},
				[this, basePathName2, &pasted]() {
					bool success = true;
					pasteFromExt(basePathName2, {"mesh"}, true, &success);
					ASSERT_TRUE(success);
				}},
			{[this, base_id1, base_id2]() {
				 ASSERT_FALSE(project->hasExternalProjectMapping(base_id1));
				 ASSERT_FALSE(project->hasExternalProjectMapping(base_id2));
			 },
				[this, basePathName1, basePathName2, base_id1, base_id2]() {
					ASSERT_EQ(project->lookupExternalProjectPath(base_id1), basePathName1);
					ASSERT_FALSE(project->hasExternalProjectMapping(base_id2));
				},
				[this, basePathName1, basePathName2, base_id1, base_id2]() {
					ASSERT_FALSE(project->hasExternalProjectMapping(base_id1));
					ASSERT_FALSE(project->hasExternalProjectMapping(base_id2));
				},
				[this, basePathName1, basePathName2, base_id1, base_id2]() {
					ASSERT_FALSE(project->hasExternalProjectMapping(base_id1));
					ASSERT_EQ(project->lookupExternalProjectPath(base_id2), basePathName2);
				}});
	});
}

TEST_F(ExtrefTest, duplicate_extref_paste_discard_all) {
	auto basePathName{(cwd_path() / "base.rcp").string()};

	setupBase(basePathName, [this]() {
		auto prefab = create<Prefab>("Prefab");
	});

	setupGeneric([this, basePathName]() {
		std::vector<std::string> stack;
		stack.emplace_back(project->currentPath());
		app->externalProjects()->addExternalProject(basePathName.c_str(), stack);
		auto originProject = app->externalProjects()->getExternalProject(basePathName);
		auto originCmd = app->externalProjects()->getExternalProjectCommandInterface(basePathName);
		auto originPrefab = Queries::findByName(originProject->instances(), "Prefab");

		auto pasted1 = cmd->pasteObjects(originCmd->copyObjects({originPrefab}), nullptr, true);
		auto pasted2 = cmd->pasteObjects(originCmd->copyObjects({originPrefab}), nullptr, true);

		int num = std::count_if(project->instances().begin(), project->instances().end(), [](SEditorObject obj) {
			return obj->objectName() == "Prefab";
		});
		ASSERT_EQ(num, 1);
	});
}

TEST_F(ExtrefTest, duplicate_extref_paste_discard_some) {
	auto basePathName{(cwd_path() / "base.rcp").string()};

	setupBase(basePathName, [this]() {
		auto mesh = create<Mesh>("Mesh");
		auto prefab = create<Prefab>("Prefab");
		auto meshnode = create<MeshNode>("prefab_child", prefab);
		cmd->set({meshnode, {"mesh"}}, mesh);
	});

	setupGeneric([this, basePathName]() {
		std::vector<std::string> stack;
		stack.emplace_back(project->currentPath());
		app->externalProjects()->addExternalProject(basePathName.c_str(), stack);
		auto originProject = app->externalProjects()->getExternalProject(basePathName);
		auto originCmd = app->externalProjects()->getExternalProjectCommandInterface(basePathName);
		auto originMesh = Queries::findByName(originProject->instances(), "Mesh");
		auto originPrefab = Queries::findByName(originProject->instances(), "Prefab");

		auto pasted1 = cmd->pasteObjects(originCmd->copyObjects({originMesh}), nullptr, true);
		auto mesh = findExt("Mesh");

		auto pasted2 = cmd->pasteObjects(originCmd->copyObjects({originPrefab}), nullptr, true);

		int num = std::count_if(project->instances().begin(), project->instances().end(), [](SEditorObject obj) {
			return obj->objectName() == "Mesh";
		});
		ASSERT_EQ(num, 1);
		auto prefab = findExt("Prefab");
		auto meshnode = findExt<MeshNode>("prefab_child");
		ASSERT_TRUE(meshnode != nullptr);
		ASSERT_EQ(prefab->children_->asVector<SEditorObject>(), std::vector<SEditorObject>({meshnode}));
		ASSERT_EQ(*meshnode->mesh_, mesh);
	});
}

TEST_F(ExtrefTest, extref_projname_change_update) {
	auto basePathName{(cwd_path() / "base.rcp").string()};
	auto compositePathName{(cwd_path() / "composite.rcp").string()};

	setupBase(basePathName, [this]() {
		auto prefab = create<Prefab>("prefab");
	});

	setupComposite(basePathName, compositePathName, {"prefab"}, [this]() {
		auto prefab = findExt("prefab");
	});

	updateBase(basePathName, [this]() {
		rename_project("Foo");
	});

	updateComposite(compositePathName, [this]() {
		auto prefab = findExt("prefab");
		auto anno = prefab->query<ExternalReferenceAnnotation>();
		std::vector<std::string> stack;
		stack.emplace_back(project->currentPath());
		auto extProject = app->externalProjects()->addExternalProject(project->lookupExternalProjectPath(*anno->projectID_), stack);
		ASSERT_EQ(extProject->projectName(), std::string("Foo"));
	});
}

TEST_F(ExtrefTest, extref_projname_change_paste_more) {
	auto basePathName{(cwd_path() / "base.rcp").string()};
	auto compositePathName{(cwd_path() / "composite.rcp").string()};

	setupBase(basePathName, [this]() {
		auto prefab = create<Prefab>("prefab");
		auto mesh = create<Mesh>("mesh");
	});

	setupComposite(basePathName, compositePathName, {"prefab"}, [this]() {
		auto prefab = findExt("prefab");
	});

	updateBase(basePathName, [this]() {
		rename_project("Foo");
	});

	updateComposite(compositePathName, [this, basePathName]() {
		bool success = true;
		pasteFromExt(basePathName, {"mesh"}, true, &success);
		ASSERT_TRUE(success);

		std::vector<std::string> stack;
		stack.emplace_back(project->currentPath());
		for (auto obj : project->instances()) {
			if (auto anno = obj->query<ExternalReferenceAnnotation>()) {
				auto extProject = app->externalProjects()->addExternalProject(project->lookupExternalProjectPath(*anno->projectID_), stack);
				ASSERT_EQ(extProject->projectName(), std::string("Foo"));
			}
		}
	});
}

TEST_F(ExtrefTest, extref_can_delete_only_unused) {
	auto basePathName{(cwd_path() / "base.rcp").string()};
	auto compositePathName{(cwd_path() / "composite.rcp").string()};

	setupBase(basePathName, [this]() {
		auto mesh = create<Mesh>("mesh");
		auto material = create<Material>("material");
		auto prefab = create<Prefab>("prefab");
		auto meshnode = create<MeshNode>("prefab_meshnode", prefab);
		cmd->set({meshnode, {"mesh"}}, mesh);
	});

	setupComposite(basePathName, compositePathName, {"prefab", "material"}, [this]() {
		auto prefab = findExt("prefab");
		auto material = findExt("material");
		auto mesh = findExt("mesh");

		EXPECT_TRUE(Queries::canDeleteObjects(*project, {material}));
		EXPECT_FALSE(Queries::canDeleteObjects(*project, {mesh}));
		EXPECT_TRUE(Queries::canDeleteObjects(*project, {prefab}));

		cmd->deleteObjects({prefab});

		EXPECT_TRUE(Queries::canDeleteObjects(*project, {material}));
		EXPECT_TRUE(Queries::canDeleteObjects(*project, {mesh}));
	});
}

TEST_F(ExtrefTest, extref_cant_move) {
	auto basePathName{(cwd_path() / "base.rcp").string()};
	auto compositePathName{(cwd_path() / "composite.rcp").string()};

	setupBase(basePathName, [this]() {
		auto mesh = create<Mesh>("mesh");
		auto material = create<Material>("material");
		auto prefab = create<Prefab>("prefab");
		auto meshnode = create<MeshNode>("meshnode", prefab);
		cmd->set({meshnode, {"mesh"}}, mesh);
	});

	setupComposite(basePathName, compositePathName, {"prefab", "material"}, [this]() {
		auto node = create<Node>("local_node");

		auto prefab = findExt("prefab");
		auto material = findExt("material");
		auto mesh = findExt("mesh");
		auto meshnode = findExt("meshnode");

		EXPECT_FALSE(Queries::canMoveScenegraphChild(*project, meshnode, node));
		EXPECT_FALSE(Queries::canMoveScenegraphChild(*project, meshnode, nullptr));
		EXPECT_FALSE(Queries::canMoveScenegraphChild(*project, node, meshnode));
	});
}

TEST_F(ExtrefTest, extref_cant_paste_into) {
	auto basePathName{(cwd_path() / "base.rcp").string()};
	auto compositePathName{(cwd_path() / "composite.rcp").string()};

	setupBase(basePathName, [this]() {
		auto material = create<Material>("material");
		auto prefab = create<Prefab>("prefab");
		auto meshnode = create<MeshNode>("meshnode", prefab);
	});

	setupComposite(basePathName, compositePathName, {"prefab", "material"}, [this]() {
		auto node = create<Node>("local_node");

		auto prefab = findExt("prefab");
		auto material = findExt("material");
		auto meshnode = findExt("meshnode");

		EXPECT_FALSE(Queries::canPasteIntoObject(*project, prefab));
		EXPECT_FALSE(Queries::canPasteIntoObject(*project, meshnode));
		EXPECT_FALSE(Queries::canPasteIntoObject(*project, material));
		EXPECT_TRUE(Queries::canPasteIntoObject(*project, node));
	});
}

TEST_F(ExtrefTest, prefab_update_create_child) {
	auto basePathName{(cwd_path() / "base.rcp").string()};
	auto compositePathName{(cwd_path() / "composite.rcp").string()};

	setupBase(basePathName, [this](){
		auto prefab = create<Prefab>("prefab");
		auto node = create<Node>("prefab_child", prefab);
	});

	setupComposite(basePathName, compositePathName, {"prefab"}, [this](){
		auto prefab = findExt("prefab");
		auto node = findExt("prefab_child");
		ASSERT_EQ(prefab->children_->asVector<SEditorObject>(), std::vector<SEditorObject>({node}));
	});
	
	updateBase(basePathName, [this]() {
		auto prefab = find("prefab");
		auto meshnode = create<MeshNode>("prefab_child_meshnode", prefab);
	});

	updateComposite(compositePathName, [this]() {
		auto prefab = findExt("prefab");
		auto node = findExt("prefab_child");
		auto meshnode = findExt("prefab_child_meshnode");
		ASSERT_EQ(prefab->children_->asVector<SEditorObject>(), std::vector<SEditorObject>({node, meshnode}));
	});
}

TEST_F(ExtrefTest, prefab_update_delete_child) {
	auto basePathName{(cwd_path() / "base.rcp").string()};
	auto compositePathName{(cwd_path() / "composite.rcp").string()};

	setupBase(basePathName, [this]() {
		auto prefab = create<Prefab>("prefab");
		auto node = create<Node>("prefab_child", prefab);
	});

	setupComposite(basePathName, compositePathName, {"prefab"}, [this]() {
		auto prefab = findExt("prefab");
		auto node = findExt("prefab_child");
		ASSERT_EQ(prefab->children_->asVector<SEditorObject>(), std::vector<SEditorObject>({node}));
	});

	updateBase(basePathName, [this]() {
		auto node = find("prefab_child");
		cmd->deleteObjects({node});
	});

	updateComposite(compositePathName, [this]() {
		auto prefab = findExt("prefab");
		dontFind("prefab_child");
	});
}


TEST_F(ExtrefTest, prefab_update_stop_using_child) {
	auto basePathName{(cwd_path() / "base.rcp").string()};
	auto compositePathName{(cwd_path() / "composite.rcp").string()};

	setupBase(basePathName, [this]() {
		auto left = create<Prefab>("prefab_left");
		auto right = create<Prefab>("prefab_right");
		auto node = create<Node>("node", left);
	});

	setupComposite(basePathName, compositePathName, {"prefab_left"}, [this]() {
		auto left = findExt("prefab_left");
		auto node = findExt("node");
		ASSERT_EQ(left->children_->asVector<SEditorObject>(), std::vector<SEditorObject>({node}));
	});

	updateBase(basePathName, [this]() {
		auto right = find("prefab_right");
		auto node = find("node");
		cmd->moveScenegraphChild(node, right);
	});

	updateComposite(compositePathName, [this]() {
		auto left = findExt("prefab_left");
		ASSERT_EQ(left->children_->size(), 0);
		dontFind("node");
	});
}
TEST_F(ExtrefTest, prefab_update_inter_prefab_scenegraph_move) {
	auto basePathName{(cwd_path() / "base.rcp").string()};
	auto compositePathName{(cwd_path() / "composite.rcp").string()};

	setupBase(basePathName, [this]() {
		auto left = create<Prefab>("prefab_left");
		auto right = create<Prefab>("prefab_right");
		auto node = create<Node>("node", left);
	});

	setupComposite(basePathName, compositePathName, {"prefab_left", "prefab_right"}, [this]() {
		auto left = findExt("prefab_left");
		auto right = findExt("prefab_right");
		auto node = findExt("node");
		ASSERT_EQ(left->children_->asVector<SEditorObject>(), std::vector<SEditorObject>({node}));
		ASSERT_EQ(right->children_->size(), 0);
	});

	updateBase(basePathName, [this]() {
		auto right = find("prefab_right");
		auto node = find("node");
		cmd->moveScenegraphChild(node, right);
	});

	updateComposite(compositePathName, [this]() {
		auto left = findExt("prefab_left");
		auto right = findExt("prefab_right");
		auto node = findExt("node");

		ASSERT_EQ(left->children_->size(), 0);
		ASSERT_EQ(right->children_->asVector<SEditorObject>(), std::vector<SEditorObject>({node}));
	});
}

TEST_F(ExtrefTest, update_losing_uniforms) {
	auto basePathName{(cwd_path() / "base.rcp").string()};
	auto compositePathName{(cwd_path() / "composite.rcp").string()};

	setupBase(basePathName, [this]() {
		auto mesh = create_mesh("mesh", "meshes/Duck.glb");
		auto material = create_material("material", "shaders/basic.vert", "shaders/basic.frag");
		auto prefab = create<Prefab>("prefab");
		auto meshnode = create_meshnode("prefab_meshnode", mesh, material, prefab);

		ValueHandle matUniforms{material, {"uniforms"}};
		EXPECT_TRUE(matUniforms.hasProperty("u_color"));

		ValueHandle meshnodeUniforms{meshnode, {"materials", "material", "uniforms"}};
		EXPECT_TRUE(meshnodeUniforms.hasProperty("u_color"));
	});

	setupComposite(basePathName, compositePathName, {"prefab"}, [this]() {
		auto mesh = findExt<Mesh>("mesh");
		auto material = findExt<Material>("material");
		auto meshnode = findExt("prefab_meshnode");

		auto local_meshnode = create_meshnode("local_meshnode", mesh, material);

		auto ext_mn_uniforms = ValueHandle(meshnode, {"materials"})[0].get("uniforms");
		ASSERT_TRUE(ext_mn_uniforms);
		ASSERT_TRUE(ext_mn_uniforms.hasProperty("u_color"));

		auto local_mn_uniforms = ValueHandle(local_meshnode, {"materials"})[0].get("uniforms");
		ASSERT_TRUE(local_mn_uniforms);
		ASSERT_TRUE(local_mn_uniforms.hasProperty("u_color"));
	});

	updateBase(basePathName, [this]() {
		auto material = find("material");
		cmd->set({material, {"uriVertex"}}, (cwd_path() / "shaders/nosuchfile.vert").string());
	});

	updateComposite(compositePathName, [this]() {
		auto mesh = findExt<Mesh>("mesh");
		auto material = findExt<Material>("material");
		auto meshnode = findExt("prefab_meshnode");

		auto local_meshnode = find("local_meshnode");

		auto ext_mn_uniforms = ValueHandle(meshnode, {"materials"})[0].get("uniforms");
		ASSERT_TRUE(ext_mn_uniforms);
		ASSERT_FALSE(ext_mn_uniforms.hasProperty("u_color"));

		auto local_mn_uniforms = ValueHandle(local_meshnode, {"materials"})[0].get("uniforms");
		ASSERT_TRUE(local_mn_uniforms);
		ASSERT_FALSE(local_mn_uniforms.hasProperty("u_color"));
	});
}

TEST_F(ExtrefTest, prefab_instance_update) {
	auto basePathName{(cwd_path() / "base.rcp").string()};
	auto compositePathName{(cwd_path() / "composite.rcp").string()};

	setupBase(basePathName, [this]() {
		auto prefab = create<Prefab>("prefab");
		auto node = create<Node>("prefab_child", prefab);
	});

	setupComposite(basePathName, compositePathName, {"prefab"}, [this]() {
		auto prefab = findExt("prefab");
		auto node = findExt("prefab_child");
		ASSERT_EQ(prefab->children_->asVector<SEditorObject>(), std::vector<SEditorObject>({node}));

		auto inst = create<PrefabInstance>("inst");
		cmd->set({inst, {"template"}}, prefab);

		auto inst_children = inst->children_->asVector<SEditorObject>();
		ASSERT_EQ(inst_children.size(), 1);
		auto inst_node = inst_children[0]->as<Node>();
		ASSERT_TRUE(inst_node != nullptr);
		ASSERT_TRUE(inst_node->query<ExternalReferenceAnnotation>() == nullptr);
	});

	updateBase(basePathName, [this]() {
		auto prefab = find("prefab");
		auto meshnode = create<MeshNode>("prefab_child_meshnode", prefab);
	});

	updateComposite(compositePathName, [this]() {
		auto prefab = findExt("prefab");
		auto node = findExt("prefab_child");
		auto inst = find("inst");

		auto meshnode = findExt("prefab_child_meshnode");
		ASSERT_TRUE(meshnode != nullptr);
		ASSERT_EQ(prefab->children_->asVector<SEditorObject>(), std::vector<SEditorObject>({node, meshnode}));

		auto inst_children = inst->children_->asVector<SEditorObject>();
		ASSERT_EQ(inst_children.size(), 2);
		for (auto child : inst_children) {
			ASSERT_TRUE(child->query<ExternalReferenceAnnotation>() == nullptr);
		}
	});
}

TEST_F(ExtrefTest, prefab_instance_lua_update_link) {
	auto basePathName{(cwd_path() / "base.rcp").string()};
	auto compositePathName{(cwd_path() / "composite.rcp").string()};

	setupBase(basePathName, [this]() {
		auto prefab = create<Prefab>("prefab");
		auto node = create<Node>("prefab_child", prefab);
		auto lua = create<LuaScript>("prefab_lua", prefab);

		TextFile scriptFile = makeFile("script.lua", R"(
function interface()
	IN.v = VEC3F
	OUT.v = VEC3F
end
function run()
end
)");
		cmd->set({lua, {"uri"}}, scriptFile);
	});

	setupComposite(basePathName, compositePathName, {"prefab"}, [this]() {
		auto prefab = find("prefab");
		auto node = find("prefab_child");
		auto lua = find("prefab_lua");
		ASSERT_EQ(prefab->children_->asVector<SEditorObject>(), std::vector<SEditorObject>({node, lua}));
	});

	updateBase(basePathName, [this]() {
		auto node = find("prefab_child");
		auto lua = find("prefab_lua");
		cmd->addLink({lua, {"luaOutputs", "v"}}, {node, {"translation"}});
	});

	updateComposite(compositePathName, [this]() {
		auto prefab = find("prefab");
		auto node = find("prefab_child");
		auto lua = find("prefab_lua");
		ASSERT_EQ(prefab->children_->asVector<SEditorObject>(), std::vector<SEditorObject>({node, lua}));

		auto link = Queries::getLink(*project, {node, {"translation"}});
		EXPECT_TRUE(link && link->startProp() == PropertyDescriptor(lua, {"luaOutputs", "v"}));
	});
}

TEST_F(ExtrefTest, nesting_create) {
	auto basePathName{(cwd_path() / "base.rcp").string()};
	auto midPathName((cwd_path() / "mid.rcp").string());
	auto compositePathName{(cwd_path() / "composite.rcp").string()};

	std::string base_id;
	std::string mid_id;

	setupBase(basePathName, [this, &base_id]() {
		//auto mesh = create_mesh("mesh", "meshes/Duck.glb");
		auto mesh = create<Mesh>("mesh");
		base_id = project->projectID();
	});

	setupComposite(basePathName, midPathName, {"mesh"}, [this, &mid_id]() {
		auto prefab = create<Prefab>("prefab");
		auto meshnode = create<MeshNode>("prefab_child", prefab);
		auto mesh = findExt("mesh");
		cmd->set({meshnode, {"mesh"}}, mesh);
		mid_id = project->projectID();
	}, "mid");

	setupComposite(midPathName, compositePathName, {"prefab"}, [this, base_id, mid_id]() {
		auto prefab = findExt("prefab", mid_id);
		auto meshnode = findExt<MeshNode>("prefab_child", mid_id);
		auto mesh = findExt("mesh", base_id);
		ASSERT_EQ(prefab->children_->asVector<SEditorObject>(), std::vector<SEditorObject>({meshnode}));
		ASSERT_EQ(*meshnode->mesh_, mesh);
	});
}

TEST_F(ExtrefTest, filecopy_update_fail_nested_same_object) {
	auto basePathName1{(cwd_path() / "base1.rcp").generic_string()};
	auto basePathName2{(cwd_path() / "base2.rcp").generic_string()};
	auto midPathName((cwd_path() / "mid.rcp").string());
	auto compositePathName{(cwd_path() / "composite.rcp").generic_string()};

	setupBase(basePathName1, [this]() {
		auto mesh = create<Mesh>("mesh");
	}, std::string("base"));

	std::filesystem::copy(basePathName1, basePathName2);
	updateBase(basePathName2, [this]() {
		rename_project("copy");
	});

	setupBase(midPathName, [this]() {
		auto prefab = create<Prefab>("prefab");
		auto meshnode = create<MeshNode>("prefab_child", prefab);
	}, "mid");

	setupBase(
		compositePathName, [this, basePathName1, midPathName]() {
			bool success = true;
			pasteFromExt(basePathName1, {"mesh"}, true, &success);
			ASSERT_TRUE(success);

			pasteFromExt(midPathName, {"prefab"}, true, &success);
			ASSERT_TRUE(success);
		},
		"composite");

	updateBase(midPathName, [this, basePathName2]() {
		bool success;
		pasteFromExt(basePathName2, {"mesh"}, true, &success);
		ASSERT_TRUE(success);

		auto meshnode = find("prefab_child");
		auto mesh = findExt("mesh");
		cmd->set({meshnode, {"mesh"}}, mesh);
	});

	EXPECT_THROW(updateComposite(compositePathName, [this]() {}), raco::core::ExtrefError);
}

TEST_F(ExtrefTest, filecopy_update_fail_nested_different_object) {
	auto basePathName1{(cwd_path() / "base1.rcp").generic_string()};
	auto basePathName2{(cwd_path() / "base2.rcp").generic_string()};
	auto midPathName((cwd_path() / "mid.rcp").string());
	auto compositePathName{(cwd_path() / "composite.rcp").generic_string()};

	setupBase(basePathName1, [this]() {
		auto mesh = create<Mesh>("mesh");
		auto material = create<Material>("material");
	}, std::string("base"));

	std::filesystem::copy(basePathName1, basePathName2);
	updateBase(basePathName2, [this]() {
		rename_project("copy");
	});

	setupBase(midPathName, [this]() {
		auto prefab = create<Prefab>("prefab");
		auto meshnode = create<MeshNode>("prefab_child", prefab);
	}, "mid");

	setupBase(
		compositePathName, [this, basePathName1, midPathName]() {
			bool success = true;
			pasteFromExt(basePathName1, {"material"}, true, &success);
			ASSERT_TRUE(success);

			pasteFromExt(midPathName, {"prefab"}, true, &success);
			ASSERT_TRUE(success);
		},
		"composite");

	updateBase(midPathName, [this, basePathName2]() {
		bool success;
		pasteFromExt(basePathName2, {"mesh"}, true, &success);
		ASSERT_TRUE(success);

		auto meshnode = find("prefab_child");
		auto mesh = findExt("mesh");
		cmd->set({meshnode, {"mesh"}}, mesh);
	});

	EXPECT_THROW(updateComposite(compositePathName, [this]() {}), raco::core::ExtrefError);
}


TEST_F(ExtrefTest, nesting_create_loop_fail) {
	auto basePathName{(cwd_path() / "base.rcp").string()};
	auto compositePathName{(cwd_path() / "composite.rcp").string()};

	setupBase(basePathName, [this]() {
		auto prefab = create<Prefab>("prefab");
	});

	setupComposite(basePathName, compositePathName, {"prefab"}, [this]() {
		auto prefab_base= findExt("prefab");
		auto prefab_comp = create<Prefab>("prefab_comp");
	}, "composite");

	updateComposite(basePathName, [this, compositePathName]() {
		bool success = true;
		pasteFromExt(compositePathName, {"prefab_comp"}, true, &success);
		ASSERT_FALSE(success);
	});
}