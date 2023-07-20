/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "application/RaCoApplication.h"
#include "application/RaCoProject.h"
#include "core/Context.h"
#include "core/ExternalReferenceAnnotation.h"
#include "core/Handles.h"
#include "core/MeshCacheInterface.h"
#include "core/Project.h"
#include "core/Queries.h"
#include "ramses_adaptor/SceneBackend.h"
#include "ramses_base/HeadlessEngineBackend.h"
#include "testing/RacoBaseTest.h"
#include "testing/TestUtil.h"
#include "user_types/UserObjectFactory.h"

#include "user_types/Animation.h"
#include "user_types/AnimationChannel.h"
#include "user_types/LuaScriptModule.h"
#include "user_types/Mesh.h"
#include "user_types/MeshNode.h"
#include "user_types/Node.h"
#include "user_types/OrthographicCamera.h"
#include "user_types/Prefab.h"
#include "user_types/PrefabInstance.h"
#include "user_types/RenderPass.h"
#include "user_types/Texture.h"
#include "user_types/Timer.h"

#include "gtest/gtest.h"

#include <filesystem>

#include <algorithm>

using namespace raco::core;
using namespace raco::user_types;

using raco::application::RaCoApplication;
using raco::application::RaCoApplicationLaunchSettings;

class ExtrefTest : public RacoBaseTest<> {
public:
	void checkLinks(const std::vector<raco::core::Link> &refLinks) {
		RacoBaseTest::checkLinks(*project, refLinks);
	}

	template <class C>
	std::shared_ptr<C> create(std::string name, SEditorObject parent = nullptr) {
		auto obj = std::dynamic_pointer_cast<C>(cmd->createObject(C::typeDescription.typeName, name));
		if (parent) {
			cmd->moveScenegraphChildren({obj}, parent);
		}
		return obj;
	}

	SAnimationChannel create_animationchannel(const std::string &name, const std::string &relpath) {
		auto channel = create<AnimationChannel>(name);
		cmd->set({channel, {"uri"}}, (test_path() / relpath).string());
		return channel;
	}

	SMesh create_mesh(const std::string &name, const std::string &relpath) {
		auto mesh = create<Mesh>(name);
		cmd->set({mesh, {"uri"}}, (test_path() / relpath).string());
		return mesh;
	}

	SMaterial create_material(const std::string &name, const std::string &relpathVertex, const std::string &relpathFragment) {
		auto material = create<Material>(name);
		cmd->set({material, {"uriVertex"}}, (test_path() / relpathVertex).string());
		cmd->set({material, {"uriFragment"}}, (test_path() / relpathFragment).string());
		return material;
	}

	SMeshNode create_meshnode(const std::string &name, SMesh mesh, SMaterial material, SEditorObject parent = nullptr) {
		auto meshnode = create<MeshNode>(name, parent);
		cmd->set({meshnode, {"mesh"}}, mesh);
		cmd->set({meshnode, {"materials", "material", "material"}}, material);
		return meshnode;
	}

	SLuaScript create_lua(const std::string &name, const std::string &relpath, raco::core::SEditorObject parent = nullptr) {
		auto lua = create<LuaScript>(name, parent);
		cmd->set({lua, {"uri"}}, (test_path() / relpath).string());
		return lua;
	}

	SLuaScript create_lua(const std::string &name, const TextFile &file, raco::core::SEditorObject parent = nullptr) {
		auto lua = create<LuaScript>(name, parent);
		cmd->set({lua, {"uri"}}, static_cast<std::string>(file));
		return lua;
	}

	SPrefabInstance create_prefabInstance(const std::string &name, raco::user_types::SPrefab prefab, raco::user_types::SEditorObject parent = nullptr) {
		auto inst = create<raco::user_types::PrefabInstance>(name, parent);
		cmd->set({inst, &PrefabInstance::template_}, prefab);
		return inst;
	}

	void change_uri(SEditorObject obj, const std::string &newvalue) {
		cmd->set({obj, {"uri"}}, (test_path() / newvalue).string());
	}

	void rename_project(const std::string &newProjectName) {
		if (!newProjectName.empty()) {
			cmd->set({app->activeRaCoProject().project()->settings(), {"objectName"}}, newProjectName);
		}
	}

	SEditorObject find(const std::string &name) {
		EXPECT_EQ(1,
			std::count_if(project->instances().begin(), project->instances().end(), [name](SEditorObject obj) {
				return obj->objectName() == name;
			}));
		auto obj = Queries::findByName(project->instances(), name);
		EXPECT_TRUE(obj != nullptr);
		return obj;
	}

	void dontFind(const std::string &name) {
		auto obj = Queries::findByName(project->instances(), name);
		EXPECT_TRUE(obj == nullptr);
	}

	SEditorObject findLocal(const std::string &name) {
		EXPECT_EQ(1,
			std::count_if(project->instances().begin(), project->instances().end(), [name](SEditorObject obj) {
				return obj->objectName() == name && !obj->query<raco::core::ExternalReferenceAnnotation>();
			}));

		auto it = std::find_if(project->instances().begin(), project->instances().end(), [name](SEditorObject obj) {
			return obj->objectName() == name && !obj->query<raco::core::ExternalReferenceAnnotation>();
		});
		return *it;
	}

	template <typename T = EditorObject>
	std::shared_ptr<T> findExt(const std::string &name, const std::string &projectID = std::string()) {
		SEditorObject editorObj = nullptr;
		for (const auto obj : project->instances()) {
			if (obj->objectName() == name && obj->query<raco::core::ExternalReferenceAnnotation>()) {
				editorObj = obj;
			}
		}
		EXPECT_TRUE(editorObj != nullptr);
		auto anno = editorObj->query<raco::core::ExternalReferenceAnnotation>();
		EXPECT_TRUE(anno != nullptr);
		if (!projectID.empty()) {
			EXPECT_EQ(*anno->projectID_, projectID);
		}
		auto objAsT = editorObj->as<T>();
		EXPECT_TRUE(objAsT != nullptr);
		return objAsT;
	}

	template <typename T = EditorObject>
	std::shared_ptr<T> findChild(SEditorObject object) {
		for (auto child : object->children_->asVector<SEditorObject>()) {
			if (auto childAsT = child->as<T>()) {
				return childAsT;
			}
		}
		return {};
	}

	template <typename T>
	size_t countInstances() {
		return std::count_if(project->instances().begin(), project->instances().end(), [](auto obj) {
			return std::dynamic_pointer_cast<T>(obj) != nullptr;
		});
	}

	raco::ramses_base::HeadlessEngineBackend backend{raco::ramses_base::BaseEngineBackend::maxFeatureLevel};

	RaCoApplication *app;
	Project *project;
	CommandInterface *cmd;

	std::string setupBase(const std::string &basePathName, std::function<void()> func, const std::string &baseProjectName = std::string("base"), int featureLevel = -1) {
		RaCoApplicationLaunchSettings settings;
		settings.createDefaultScene = false;
		settings.initialLoadFeatureLevel = featureLevel;
		RaCoApplication base{backend, settings};
		app = &base;
		project = base.activeRaCoProject().project();
		cmd = base.activeRaCoProject().commandInterface();

		rename_project(baseProjectName);

		func();

		std::string msg;
		base.activeRaCoProject().saveAs(basePathName.c_str(), msg);
		return project->projectID();
	}

	std::string setupGeneric(std::function<void()> func) {
		RaCoApplicationLaunchSettings settings;
		settings.createDefaultScene = false;
		RaCoApplication app_{backend, settings};
		app = &app_;
		project = app_.activeRaCoProject().project();
		cmd = app_.activeRaCoProject().commandInterface();

		func();
		return project->projectID();
	}

	std::string copyExternalObjects(const std::string& basePathName, const std::vector<std::string>& externalObjectNames) {
		RaCoApplicationLaunchSettings settings;
		settings.createDefaultScene = false;
		RaCoApplication app{backend, settings};
		auto project = app.activeRaCoProject().project();

		LoadContext loadContext;
		loadContext.pathStack.emplace_back(project->currentPath());
		bool status = app.externalProjects()->addExternalProject(basePathName.c_str(), loadContext);
		if (!status) {
			return {};
		}
		auto originProject = app.externalProjects()->getExternalProject(basePathName);
		auto originCmd = app.externalProjects()->getExternalProjectCommandInterface(basePathName);

		std::vector<SEditorObject> origin;
		for (auto name : externalObjectNames) {
			if (auto obj = Queries::findByName(originProject->instances(), name)) {
				origin.emplace_back(obj);
			}
		}

		return originCmd->copyObjects(origin);
	}

	bool pasteFromExt(const std::string &basePathName, const std::vector<std::string> &externalObjectNames, bool asExtref, std::vector<SEditorObject> *outPasted = nullptr) {
		bool success = true;
		LoadContext loadContext;
		loadContext.pathStack.emplace_back(project->currentPath());
		bool status = app->externalProjects()->addExternalProject(basePathName.c_str(), loadContext);
		if (!status) {
			if (outPasted) {
				outPasted->clear();
			}
			return false;
		}
		auto originProject = app->externalProjects()->getExternalProject(basePathName);
		auto originCmd = app->externalProjects()->getExternalProjectCommandInterface(basePathName);

		std::vector<SEditorObject> origin;
		for (auto name : externalObjectNames) {
			if (auto obj = Queries::findByName(originProject->instances(), name)) {
				origin.emplace_back(obj);
			}
		}

		size_t numInstances = project->instances().size();
		try {
			auto pasted = cmd->pasteObjects(originCmd->copyObjects(origin), nullptr, asExtref);
			if (outPasted) {
				*outPasted = pasted;
			}
		} catch (std::exception &error) {
			EXPECT_EQ(project->instances().size(), numInstances);
			success = false;
		}
		return success;
	}

	std::string setupComposite(const std::string &basePathName, const std::string &compositePathName, const std::vector<std::string> &externalObjectNames,
		std::function<void()> func, const std::string &projectName = std::string(), int featureLevel = -1) {
		RaCoApplicationLaunchSettings settings;
		settings.createDefaultScene = false;
		settings.initialLoadFeatureLevel = featureLevel;
		RaCoApplication app_{backend, settings};
		app = &app_;
		project = app->activeRaCoProject().project();
		cmd = app->activeRaCoProject().commandInterface();

		rename_project(projectName);

		pasteFromExt(basePathName, externalObjectNames, true);

		func();

		if (!compositePathName.empty()) {
			std::string msg;
			app->activeRaCoProject().saveAs(compositePathName.c_str(), msg);
		}
		return project->projectID();
	}

	void updateBase(const std::string &basePathName, std::function<void()> func, int featureLevel = -1) {
		RaCoApplicationLaunchSettings settings;
		settings.initialProject = basePathName.c_str();
		settings.initialLoadFeatureLevel = featureLevel;

		RaCoApplication base{backend, settings};
		app = &base;
		project = base.activeRaCoProject().project();
		cmd = base.activeRaCoProject().commandInterface();

		func();

		std::string msg;
		ASSERT_TRUE(base.activeRaCoProject().save(msg));
	}

	void updateComposite(const std::string &pathName, std::function<void()> func, int featureLevel = -1) {
		RaCoApplicationLaunchSettings settings;
		settings.initialProject = pathName.c_str();
		settings.initialLoadFeatureLevel = featureLevel;

		RaCoApplication app_{backend, settings};
		app = &app_;
		project = app->activeRaCoProject().project();
		cmd = app->activeRaCoProject().commandInterface();

		func();
	}
};

TEST_F(ExtrefTest, normal_paste) {
	auto basePathName{(test_path() / "base.rca").string()};

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
	auto basePathName{(test_path() / "base.rca").string()};

	setupBase(basePathName, [this]() {
		auto prefab = create<Prefab>("Prefab");
	});

	setupGeneric([this, basePathName]() {
		LoadContext loadContext;
		loadContext.pathStack.emplace_back(project->currentPath());
		app->externalProjects()->addExternalProject(basePathName.c_str(), loadContext);
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
	auto basePathName{(test_path() / "base.rca").string()};

	auto base_id = setupBase(
		basePathName, [this]() {
			auto prefab = create<Prefab>("Prefab");
		},
		std::string());

	setupGeneric([this, basePathName, base_id]() {
		ASSERT_TRUE(pasteFromExt(basePathName, {"Prefab"}, true));

		ASSERT_TRUE(project->hasExternalProjectMapping(base_id));
	});
}

TEST_F(ExtrefTest, extref_paste_fail_renderpass) {
	auto basePathName{(test_path() / "base.rca").string()};

	setupBase(basePathName, [this]() {
		auto prefab = create<RenderPass>("renderpass");
	});

	setupGeneric([this, basePathName]() {
		std::vector<SEditorObject> pasted;
		ASSERT_TRUE(pasteFromExt(basePathName, {"renderpass"}, true, &pasted));
		ASSERT_EQ(pasted.size(), 0);
	});
}

TEST_F(ExtrefTest, extref_paste_fail_existing_object_from_same_project) {
	auto basePathName{(test_path() / "base.rca").string()};

	setupBase(basePathName, [this]() {
		auto prefab = create<Prefab>("Prefab");
	});

	updateComposite(basePathName, [this, basePathName]() {
		ASSERT_FALSE(pasteFromExt(basePathName, {"Prefab"}, true));
		ASSERT_EQ(project->instances().size(), 2);  // ProjectSettings and Prefab
	});
}

TEST_F(ExtrefTest, extref_paste_fail_deleted_object_from_same_project) {
	auto basePathName{(test_path() / "base.rca").string()};

	setupBase(basePathName, [this]() {
		auto prefab = create<Prefab>("Prefab");
	});

	updateComposite(basePathName, [this, basePathName]() {
		auto prefab = find("Prefab");
		cmd->deleteObjects({prefab});
		dontFind("Prefab");

		ASSERT_FALSE(pasteFromExt(basePathName, {"Prefab"}, true));
		ASSERT_EQ(project->instances().size(), 1);  // ProjecSettings
	});
}

TEST_F(ExtrefTest, extref_paste_fail_existing_object_from_same_project_path) {
	auto basePathName{(test_path() / "base.rca").string()};

	setupBase(basePathName, [this]() {
		auto prefab = create<Prefab>("Prefab");
	});

	updateComposite(basePathName, [this, basePathName]() {
		rename_project("not_base_anymore");
		ASSERT_FALSE(pasteFromExt(basePathName, {"Prefab"}, true));
		ASSERT_EQ(project->instances().size(), 2);  // ProjectSettings and Prefab
	});
}

TEST_F(ExtrefTest, extref_paste_fail_deleted_object_from_same_project_path) {
	auto basePathName{(test_path() / "base.rca").string()};

	setupBase(basePathName, [this]() {
		auto prefab = create<Prefab>("Prefab");
	});

	updateComposite(basePathName, [this, basePathName]() {
		rename_project("not_base_anymore");

		auto prefab = find("Prefab");
		cmd->deleteObjects({prefab});
		dontFind("Prefab");

		ASSERT_FALSE(pasteFromExt(basePathName, {"Prefab"}, true));
		ASSERT_EQ(project->instances().size(), 1);  // ProjectSettings
	});
}

TEST_F(ExtrefTest, extref_paste_fail_from_filecopy) {
	auto basePathName1{(test_path() / "base1.rca").string()};
	auto basePathName2{(test_path() / "base2.rca").string()};

	setupBase(
		basePathName1, [this]() {
			auto mesh = create<Mesh>("mesh");
		},
		std::string("base"));

	std::filesystem::copy(basePathName1, basePathName2);
	updateBase(basePathName2, [this]() {
		auto mesh = find("mesh");
		auto prefab = create<Prefab>("prefab");
		auto meshnode = create<MeshNode>("meshnode", prefab);
		cmd->set({meshnode, {"mesh"}}, mesh);
		auto material = create<Material>("material");
	});

	updateComposite(basePathName1, [this, basePathName2]() {
		ASSERT_FALSE(pasteFromExt(basePathName2, {"mesh"}, true));
		ASSERT_EQ(project->instances().size(), 2);	// ProjectSettings and mesh
		ASSERT_FALSE(pasteFromExt(basePathName2, {"material"}, true));
		ASSERT_EQ(project->instances().size(), 2);	// ProjectSettings and mesh
		ASSERT_FALSE(pasteFromExt(basePathName2, {"prefab"}, true));
		ASSERT_EQ(project->instances().size(), 2);	// ProjectSettings and mesh
	});

	setupGeneric([this, basePathName2]() {
		ASSERT_TRUE(pasteFromExt(basePathName2, {"mesh"}, true));
		ASSERT_TRUE(pasteFromExt(basePathName2, {"material"}, true));
		ASSERT_TRUE(pasteFromExt(basePathName2, {"prefab"}, true));
	});
}

TEST_F(ExtrefTest, extref_paste) {
	auto basePathName{(test_path() / "base.rca").string()};

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
	auto basePathName1{(test_path() / "base1.rca").string()};
	auto basePathName2{(test_path() / "base2.rca").string()};

	auto base1_id = setupBase(
		basePathName1, [this]() {
			auto prefab = create<Prefab>("Prefab");
		},
		std::string("base"));

	auto base2_id = setupBase(
		basePathName2, [this]() {
			auto mesh = create<Mesh>("mesh");
		},
		std::string("base"));

	setupGeneric([this, basePathName1, basePathName2, base1_id, base2_id]() {
		ASSERT_TRUE(pasteFromExt(basePathName1, {"Prefab"}, true));
		ASSERT_EQ(project->lookupExternalProjectPath(base1_id), basePathName1);
		ASSERT_TRUE(pasteFromExt(basePathName2, {"mesh"}, true));
		ASSERT_EQ(project->lookupExternalProjectPath(base2_id), basePathName2);
	});
}

TEST_F(ExtrefTest, filecopy_paste_fail_same_object) {
	auto basePathName1{(test_path() / "base1.rca").string()};
	auto basePathName2{(test_path() / "base2.rca").string()};

	setupBase(
		basePathName1, [this]() {
			auto mesh = create<Mesh>("mesh");
		},
		std::string("base"));

	std::filesystem::copy(basePathName1, basePathName2);
	updateBase(basePathName2, [this]() {
		rename_project("copy");
	});

	setupGeneric([this, basePathName1, basePathName2]() {
		ASSERT_TRUE(pasteFromExt(basePathName1, {"mesh"}, true));
		ASSERT_FALSE(pasteFromExt(basePathName2, {"mesh"}, true));
	});
}

TEST_F(ExtrefTest, filecopy_paste_fail_different_object) {
	auto basePathName1{(test_path() / "base1.rca").string()};
	auto basePathName2{(test_path() / "base2.rca").string()};
	auto compositePathName{(test_path() / "composite.rca").string()};

	setupBase(
		basePathName1, [this]() {
			auto mesh = create<Mesh>("mesh");
			auto prefab = create<Prefab>("prefab");
			auto meshnode = create<MeshNode>("meshnode", prefab);
		},
		std::string("base"));

	std::filesystem::copy(basePathName1, basePathName2);
	updateBase(basePathName2, [this]() {
		rename_project("copy");
	});

	setupGeneric([this, basePathName1, basePathName2]() {
		ASSERT_TRUE(pasteFromExt(basePathName1, {"prefab"}, true));
		ASSERT_EQ(project->instances().size(), 3);	// ProjectSettings, prefab, and meshnode
		ASSERT_FALSE(pasteFromExt(basePathName2, {"mesh"}, true));
		ASSERT_EQ(project->instances().size(), 3);	// ProjectSettings, prefab, and meshnode
	});
}

TEST_F(ExtrefTest, filecopy_paste_fail_new_object) {
	auto basePathName1{(test_path() / "base1.rca").string()};
	auto basePathName2{(test_path() / "base2.rca").string()};

	setupBase(
		basePathName1, [this]() {
			auto mesh = create<Mesh>("mesh");
		},
		std::string("base"));

	std::filesystem::copy(basePathName1, basePathName2);
	updateBase(basePathName2, [this]() {
		auto material = create<Material>("material");
		rename_project("copy");
	});

	setupGeneric([this, basePathName1, basePathName2]() {
		ASSERT_TRUE(pasteFromExt(basePathName1, {"mesh"}, true));
		ASSERT_EQ(project->instances().size(), 2);	// ProjectSettings and mesh
		ASSERT_FALSE(pasteFromExt(basePathName2, {"material"}, true));
		ASSERT_EQ(project->instances().size(), 2);	// ProjectSettings and mesh
	});
}

TEST_F(ExtrefTest, extref_paste_same_project_name_after_delete_with_undo) {
	auto basePathName1{(test_path() / "base1.rca").string()};
	auto basePathName2{(test_path() / "base2.rca").string()};

	auto base_id1 = setupBase(
		basePathName1, [this]() {
			auto prefab = create<Prefab>("Prefab");
		},
		std::string("base"));

	auto base_id2 = setupBase(
		basePathName2, [this]() {
			auto mesh = create<Mesh>("mesh");
		},
		std::string("base"));

	setupGeneric([this, basePathName1, basePathName2, base_id1, base_id2]() {
		std::vector<SEditorObject> pasted;

		checkUndoRedoMultiStep<3>(*cmd,
			{[this, basePathName1, &pasted]() {
				 ASSERT_TRUE(pasteFromExt(basePathName1, {"Prefab"}, true, &pasted));
			 },
				[this, basePathName1, base_id1, &pasted]() {
					cmd->deleteObjects(pasted);
					ASSERT_FALSE(project->hasExternalProjectMapping(base_id1));
				},
				[this, basePathName2, &pasted]() {
					ASSERT_TRUE(pasteFromExt(basePathName2, {"mesh"}, true));
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
	auto basePathName{(test_path() / "base.rca").string()};

	setupBase(basePathName, [this]() {
		auto prefab = create<Prefab>("Prefab");
	});

	setupGeneric([this, basePathName]() {
		LoadContext loadContext;
		loadContext.pathStack.emplace_back(project->currentPath());
		app->externalProjects()->addExternalProject(basePathName.c_str(), loadContext);
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
	auto basePathName{(test_path() / "base.rca").string()};

	setupBase(basePathName, [this]() {
		auto mesh = create<Mesh>("Mesh");
		auto prefab = create<Prefab>("Prefab");
		auto meshnode = create<MeshNode>("prefab_child", prefab);
		cmd->set({meshnode, {"mesh"}}, mesh);
	});

	setupGeneric([this, basePathName]() {
		LoadContext loadContext;
		loadContext.pathStack.emplace_back(project->currentPath());
		app->externalProjects()->addExternalProject(basePathName.c_str(), loadContext);
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
	auto basePathName{(test_path() / "base.rca").string()};
	auto compositePathName{(test_path() / "composite.rca").string()};

	setupBase(basePathName, [this]() {
		auto prefab = create<Prefab>("prefab");
	});

	setupComposite(basePathName, compositePathName, {"prefab"}, [this]() {
		auto prefab = findExt<Prefab>("prefab");
		auto inst = create_prefabInstance("inst", prefab);
	});

	updateBase(basePathName, [this]() {
		rename_project("Foo");
	});

	updateComposite(compositePathName, [this]() {
		auto prefab = findExt("prefab");
		auto anno = prefab->query<ExternalReferenceAnnotation>();
		LoadContext loadContext;
		loadContext.pathStack.emplace_back(project->currentPath());
		auto extProject = app->externalProjects()->addExternalProject(project->lookupExternalProjectPath(*anno->projectID_), loadContext);
		ASSERT_EQ(extProject->projectName(), std::string("Foo"));
	});
}

TEST_F(ExtrefTest, extref_projname_change_paste_more) {
	auto basePathName{(test_path() / "base.rca").string()};
	auto compositePathName{(test_path() / "composite.rca").string()};

	setupBase(basePathName, [this]() {
		auto prefab = create<Prefab>("prefab");
		auto mesh = create<Mesh>("mesh");
	});

	setupComposite(basePathName, compositePathName, {"prefab"}, [this]() {
		auto prefab = findExt<Prefab>("prefab");
		auto inst = create_prefabInstance("inst", prefab);
	});

	updateBase(basePathName, [this]() {
		rename_project("Foo");
	});

	updateComposite(compositePathName, [this, basePathName]() {
		ASSERT_TRUE(pasteFromExt(basePathName, {"mesh"}, true));

		LoadContext loadContext;
		loadContext.pathStack.emplace_back(project->currentPath());
		for (auto obj : project->instances()) {
			if (auto anno = obj->query<ExternalReferenceAnnotation>()) {
				auto extProject = app->externalProjects()->addExternalProject(project->lookupExternalProjectPath(*anno->projectID_), loadContext);
				ASSERT_EQ(extProject->projectName(), std::string("Foo"));
			}
		}
	});
}

TEST_F(ExtrefTest, extref_can_delete_only_unused) {
	auto basePathName{(test_path() / "base.rca").string()};
	auto compositePathName{(test_path() / "composite.rca").string()};

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
		auto meshnode = findExt("prefab_meshnode");

		EXPECT_FALSE(Queries::filterForDeleteableObjects(*project, {material}).empty());
		EXPECT_TRUE(Queries::filterForDeleteableObjects(*project, {mesh}).empty());
		EXPECT_FALSE(Queries::filterForDeleteableObjects(*project, {prefab}).empty());

		EXPECT_EQ(Queries::filterForDeleteableObjects(*project, {material, mesh}).size(), 1);
		EXPECT_EQ(Queries::filterForDeleteableObjects(*project, {prefab, mesh}).size(), 3);
		EXPECT_EQ(Queries::filterForDeleteableObjects(*project, {prefab, meshnode}).size(), 2);
		EXPECT_EQ(Queries::filterForDeleteableObjects(*project, {prefab, mesh, meshnode}).size(), 3);
		EXPECT_EQ(Queries::filterForDeleteableObjects(*project, {prefab, meshnode, mesh, material}).size(), 4);
		EXPECT_EQ(Queries::filterForDeleteableObjects(*project, {prefab, material}).size(), 3);

		cmd->deleteObjects({prefab});

		EXPECT_FALSE(Queries::filterForDeleteableObjects(*project, {material}).empty());
		EXPECT_FALSE(Queries::filterForDeleteableObjects(*project, {mesh}).empty());
	});
}

TEST_F(ExtrefTest, extref_can_delete_only_unused_with_links) {
	auto basePathName{(test_path() / "base.rca").string()};
	auto compositePathName{(test_path() / "composite.rca").string()};

	setupBase(basePathName, [this]() {
		TextFile scriptFile = makeFile("script.lua", R"(
function interface(IN,OUT)
	IN.v = Type:Vec3f()
	OUT.v = Type:Vec3f()
end
function run(IN,OUT)
end
)");
		auto luaSource = create_lua("luaSource", scriptFile);
		auto luaSink = create_lua("luaSink", scriptFile);
		cmd->addLink({luaSource, {"outputs", "v"}}, {luaSink, {"inputs", "v"}});
	});

	setupComposite(basePathName, compositePathName, {"luaSink"}, [this]() {
		auto luaSource = findExt("luaSource");
		auto luaSink = findExt("luaSink");

		EXPECT_FALSE(Queries::filterForDeleteableObjects(*project, {luaSink}).empty());
		EXPECT_EQ(Queries::filterForDeleteableObjects(*project, {luaSink, luaSource}).size(), 2);
		EXPECT_TRUE(Queries::filterForDeleteableObjects(*project, {luaSource}).empty());

		cmd->deleteObjects({luaSink});

		EXPECT_FALSE(Queries::filterForDeleteableObjects(*project, {luaSource}).empty());
	});
}

TEST_F(ExtrefTest, extref_cant_move) {
	auto basePathName{(test_path() / "base.rca").string()};
	auto compositePathName{(test_path() / "composite.rca").string()};

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

		EXPECT_TRUE(Queries::filterForMoveableScenegraphChildren(*project, {meshnode}, node).empty());
		EXPECT_TRUE(Queries::filterForMoveableScenegraphChildren(*project, {meshnode}, nullptr).empty());
		EXPECT_TRUE(Queries::filterForMoveableScenegraphChildren(*project, {node}, meshnode).empty());
	});
}

TEST_F(ExtrefTest, extref_cant_paste_into) {
	auto basePathName{(test_path() / "base.rca").string()};
	auto compositePathName{(test_path() / "composite.rca").string()};

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
	auto basePathName{(test_path() / "base.rca").string()};
	auto compositePathName{(test_path() / "composite.rca").string()};

	setupBase(basePathName, [this]() {
		auto prefab = create<Prefab>("prefab");
		auto node = create<Node>("prefab_child", prefab);
	});

	setupComposite(basePathName, compositePathName, {"prefab"}, [this]() {
		auto prefab = findExt<Prefab>("prefab");
		auto node = findExt("prefab_child");
		ASSERT_EQ(prefab->children_->asVector<SEditorObject>(), std::vector<SEditorObject>({node}));
		auto inst = create_prefabInstance("inst", prefab);
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
	auto basePathName{(test_path() / "base.rca").string()};
	auto compositePathName{(test_path() / "composite.rca").string()};

	setupBase(basePathName, [this]() {
		auto prefab = create<Prefab>("prefab");
		auto node = create<Node>("prefab_child", prefab);
	});

	setupComposite(basePathName, compositePathName, {"prefab"}, [this]() {
		auto prefab = findExt<Prefab>("prefab");
		auto node = findExt("prefab_child");
		ASSERT_EQ(prefab->children_->asVector<SEditorObject>(), std::vector<SEditorObject>({node}));
		auto inst = create_prefabInstance("inst", prefab);
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
	auto basePathName{(test_path() / "base.rca").string()};
	auto compositePathName{(test_path() / "composite.rca").string()};

	setupBase(basePathName, [this]() {
		auto left = create<Prefab>("prefab_left");
		auto right = create<Prefab>("prefab_right");
		auto node = create<Node>("node", left);
	});

	setupComposite(basePathName, compositePathName, {"prefab_left"}, [this]() {
		auto left = findExt<Prefab>("prefab_left");
		auto node = findExt("node");
		ASSERT_EQ(left->children_->asVector<SEditorObject>(), std::vector<SEditorObject>({node}));
		auto inst = create_prefabInstance("inst", left);
	});

	updateBase(basePathName, [this]() {
		auto right = find("prefab_right");
		auto node = find("node");
		cmd->moveScenegraphChildren({node}, right);
	});

	updateComposite(compositePathName, [this]() {
		auto left = findExt("prefab_left");
		ASSERT_EQ(left->children_->size(), 0);
		dontFind("node");
	});
}
TEST_F(ExtrefTest, prefab_update_inter_prefab_scenegraph_move) {
	auto basePathName{(test_path() / "base.rca").string()};
	auto compositePathName{(test_path() / "composite.rca").string()};

	setupBase(basePathName, [this]() {
		auto left = create<Prefab>("prefab_left");
		auto right = create<Prefab>("prefab_right");
		auto node = create<Node>("node", left);
	});

	setupComposite(basePathName, compositePathName, {"prefab_left", "prefab_right"}, [this]() {
		auto left = findExt<Prefab>("prefab_left");
		auto right = findExt<Prefab>("prefab_right");
		auto node = findExt("node");
		ASSERT_EQ(left->children_->asVector<SEditorObject>(), std::vector<SEditorObject>({node}));
		ASSERT_EQ(right->children_->size(), 0);

		auto inst_left = create_prefabInstance("inst_left", left);
		auto inst_right = create_prefabInstance("inst_right", right);
	});

	updateBase(basePathName, [this]() {
		auto right = find("prefab_right");
		auto node = find("node");
		cmd->moveScenegraphChildren({node}, right);
	});

	updateComposite(compositePathName, [this]() {
		auto left = findExt("prefab_left");
		auto right = findExt("prefab_right");
		auto node = findExt("node");

		ASSERT_EQ(left->children_->size(), 0);
		ASSERT_EQ(right->children_->asVector<SEditorObject>(), std::vector<SEditorObject>({node}));
	});
}

TEST_F(ExtrefTest, prefab_update_move_and_delete_parent) {
	auto basePathName{(test_path() / "base.rca").string()};
	auto compositePathName{(test_path() / "composite.rca").string()};

	setupBase(basePathName, [this]() {
		auto prefab = create<Prefab>("prefab");
		auto node = create<Node>("prefab_node", prefab);
		auto child = create<Node>("prefab_child", node);
	});

	setupComposite(basePathName, compositePathName, {"prefab"}, [this]() {
		auto prefab = findExt<Prefab>("prefab");
		auto node = find("prefab_node");
		auto child = find("prefab_child");
		EXPECT_EQ(child->getParent(), node);
		auto inst = create_prefabInstance("inst", prefab);
	});

	updateBase(basePathName, [this]() {
		auto prefab = find("prefab");
		auto node = find("prefab_node");
		auto child = find("prefab_child");
		cmd->moveScenegraphChildren({child}, prefab);
		cmd->deleteObjects({node});
	});

	updateComposite(compositePathName, [this]() {
		auto prefab = findExt("prefab");
		dontFind("prefab_node");
		auto child = findExt("prefab_child");
		EXPECT_EQ(child->getParent(), prefab);
	});
}

TEST_F(ExtrefTest, prefab_update_move_and_delete_parent_linked) {
	auto basePathName{(test_path() / "base.rca").string()};
	auto compositePathName{(test_path() / "composite.rca").string()};

	setupBase(basePathName, [this]() {
		auto prefab = create<Prefab>("prefab");
		auto node = create<Node>("prefab_node", prefab);
		auto child = create<Node>("prefab_child", node);

		TextFile scriptFile = makeFile("script.lua", R"(
function interface(IN,OUT)
	IN.v = Type:Vec3f()
	OUT.v = Type:Vec3f()
end
function run(IN,OUT)
end
)");
		auto lua = create_lua("prefab_lua", scriptFile, prefab);
		cmd->addLink({lua, {"outputs", "v"}}, {child, {"translation"}});
	});

	setupComposite(basePathName, compositePathName, {"prefab"}, [this]() {
		auto prefab = findExt<Prefab>("prefab");
		auto node = find("prefab_node");
		auto child = find("prefab_child");
		auto lua = find("prefab_lua");
		EXPECT_EQ(child->getParent(), node);
		checkLinks({{{lua, {"outputs", "v"}}, {child, {"translation"}}}});
		auto inst = create_prefabInstance("inst", prefab);
	});

	updateBase(basePathName, [this]() {
		auto prefab = find("prefab");
		auto node = find("prefab_node");
		auto child = find("prefab_child");
		auto lua = find("prefab_lua");
		cmd->moveScenegraphChildren({child}, prefab);
		cmd->deleteObjects({node});
	});

	updateComposite(compositePathName, [this]() {
		auto prefab = findExt("prefab");
		dontFind("prefab_node");
		auto child = findExt("prefab_child");
		auto lua = findExt("prefab_lua");
		auto inst_lua = findLocal("prefab_lua");
		auto inst_child = findLocal("prefab_child");
		EXPECT_EQ(child->getParent(), prefab);
		checkLinks({{{lua, {"outputs", "v"}}, {child, {"translation"}}},
			{{inst_lua, {"outputs", "v"}}, {inst_child, {"translation"}}}});
	});
}

TEST_F(ExtrefTest, update_losing_uniforms) {
	auto basePathName{(test_path() / "base.rca").string()};
	auto compositePathName{(test_path() / "composite.rca").string()};

	setupBase(basePathName, [this]() {
		auto mesh = create_mesh("mesh", "meshes/Duck.glb");
		auto material = create_material("material", "shaders/basic.vert", "shaders/basic.frag");
		auto prefab = create<Prefab>("prefab");
		auto meshnode = create_meshnode("prefab_meshnode", mesh, material, prefab);
		cmd->set(meshnode->getMaterialPrivateHandle(0), true);

		ValueHandle matUniforms{material, {"uniforms"}};
		EXPECT_TRUE(matUniforms.hasProperty("u_color"));

		ValueHandle meshnodeUniforms{meshnode, {"materials", "material", "uniforms"}};
		EXPECT_TRUE(meshnodeUniforms.hasProperty("u_color"));
	});

	setupComposite(basePathName, compositePathName, {"prefab"}, [this]() {
		auto prefab = findExt<Prefab>("prefab");
		auto inst = create_prefabInstance("inst", prefab);
		auto mesh = findExt<Mesh>("mesh");
		auto material = findExt<Material>("material");
		auto meshnode = findExt("prefab_meshnode");

		auto local_meshnode = create_meshnode("local_meshnode", mesh, material);
		cmd->set(local_meshnode->getMaterialPrivateHandle(0), true);

		auto ext_mn_uniforms = ValueHandle(meshnode, {"materials"})[0].get("uniforms");
		ASSERT_TRUE(ext_mn_uniforms);
		ASSERT_TRUE(ext_mn_uniforms.hasProperty("u_color"));

		auto local_mn_uniforms = ValueHandle(local_meshnode, {"materials"})[0].get("uniforms");
		ASSERT_TRUE(local_mn_uniforms);
		ASSERT_TRUE(local_mn_uniforms.hasProperty("u_color"));
	});

	updateBase(basePathName, [this]() {
		auto material = find("material");
		cmd->set({material, {"uriVertex"}}, (test_path() / "shaders/nosuchfile.vert").string());
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
	auto basePathName{(test_path() / "base.rca").string()};
	auto compositePathName{(test_path() / "composite.rca").string()};

	setupBase(basePathName, [this]() {
		auto prefab = create<Prefab>("prefab");
		auto node = create<Node>("prefab_child", prefab);
	});

	setupComposite(basePathName, compositePathName, {"prefab"}, [this]() {
		auto prefab = findExt<Prefab>("prefab");
		auto node = findExt("prefab_child");
		ASSERT_EQ(prefab->children_->asVector<SEditorObject>(), std::vector<SEditorObject>({node}));

		auto inst = create_prefabInstance("inst", prefab);

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

TEST_F(ExtrefTest, prefab_instance_update_camera_property) {
	auto basePathName{(test_path() / "base.rca").string()};
	auto compositePathName{(test_path() / "composite.rca").string()};

	setupBase(basePathName, [this]() {
		auto prefab = create<Prefab>("prefab");
		auto camera = create<OrthographicCamera>("prefab_camera", prefab);
		cmd->set({camera, {"translation", "x"}}, 4.0);
	});

	setupComposite(basePathName, compositePathName, {"prefab"}, [this]() {
		auto prefab = findExt<Prefab>("prefab");
		auto prefab_camera = prefab->children_->get(0)->asRef()->as<OrthographicCamera>();
		EXPECT_EQ(*prefab_camera->translation_->x, 4.0);

		auto inst = create_prefabInstance("inst", prefab);
		auto inst_camera = inst->children_->get(0)->asRef()->as<OrthographicCamera>();
		EXPECT_EQ(*inst_camera->translation_->x, 4.0);
	});

	updateBase(basePathName, [this]() {
		auto camera = find("prefab_camera");
		cmd->set({camera, {"translation", "x"}}, 0.0);
	});

	updateComposite(compositePathName, [this]() {
		auto prefab = findExt<Prefab>("prefab");
		auto prefab_camera = prefab->children_->get(0)->asRef()->as<OrthographicCamera>();
		EXPECT_EQ(*prefab_camera->translation_->x, 0.0);

		auto inst = find("inst");
		auto inst_camera = inst->children_->get(0)->asRef()->as<OrthographicCamera>();
		EXPECT_EQ(*inst_camera->translation_->x, 0.0);
	});
}

TEST_F(ExtrefTest, prefab_instance_lua_update_link) {
	auto basePathName{(test_path() / "base.rca").string()};
	auto compositePathName{(test_path() / "composite.rca").string()};

	setupBase(basePathName, [this]() {
		auto prefab = create<Prefab>("prefab");
		auto node = create<Node>("prefab_child", prefab);

		TextFile scriptFile = makeFile("script.lua", R"(
function interface(IN,OUT)
	IN.v = Type:Vec3f()
	OUT.v = Type:Vec3f()
end
function run(IN,OUT)
end
)");
		auto lua = create_lua("prefab_lua", scriptFile, prefab);
	});

	setupComposite(basePathName, compositePathName, {"prefab"}, [this]() {
		auto prefab = findExt<Prefab>("prefab");
		auto node = find("prefab_child");
		auto lua = find("prefab_lua");
		ASSERT_EQ(prefab->children_->asVector<SEditorObject>(), std::vector<SEditorObject>({node, lua}));
		auto inst = create_prefabInstance("inst", prefab);
	});

	updateBase(basePathName, [this]() {
		auto node = find("prefab_child");
		auto lua = find("prefab_lua");
		cmd->addLink({lua, {"outputs", "v"}}, {node, {"translation"}});
	});

	updateComposite(compositePathName, [this]() {
		auto prefab = findExt("prefab");
		auto node = findExt("prefab_child");
		auto lua = findExt("prefab_lua");
		ASSERT_EQ(prefab->children_->asVector<SEditorObject>(), std::vector<SEditorObject>({node, lua}));

		auto link = Queries::getLink(*project, {node, {"translation"}});
		EXPECT_TRUE(link && link->startProp() == PropertyDescriptor(lua, {"outputs", "v"}));
	});
}

TEST_F(ExtrefTest, duplicate_link_paste_prefab) {
	auto basePathName{(test_path() / "base.rca").string()};
	auto compositePathName{(test_path() / "composite.rca").string()};

	setupBase(basePathName, [this]() {
		auto prefab = create<Prefab>("prefab");
		auto node = create<Node>("prefab_child", prefab);

		TextFile scriptFile = makeFile("script.lua", R"(
function interface(IN,OUT)
	IN.v = Type:Vec3f()
	OUT.v = Type:Vec3f()
end
function run(IN,OUT)
end
)");
		auto lua = create_lua("prefab_lua", scriptFile, prefab);
		cmd->addLink({lua, {"outputs", "v"}}, {node, {"translation"}});
	});

	setupGeneric([this, basePathName]() {
		ASSERT_TRUE(pasteFromExt(basePathName, {"prefab"}, true));
		ASSERT_EQ(project->links().size(), 1);
		ASSERT_TRUE(pasteFromExt(basePathName, {"prefab"}, true));
		ASSERT_EQ(project->links().size(), 1);
	});
}

TEST_F(ExtrefTest, duplicate_link_paste_lua) {
	auto basePathName{(test_path() / "base.rca").string()};

	setupBase(basePathName, [this]() {
		TextFile scriptFile = makeFile("script.lua", R"(
function interface(IN,OUT)
	IN.v = Type:Vec3f()
	OUT.v = Type:Vec3f()
end
function run(IN,OUT)
end
)");
		auto start = create_lua("start", scriptFile);
		auto end = create_lua("end", scriptFile);
		cmd->addLink({start, {"outputs", "v"}}, {end, {"inputs", "v"}});
	});

	setupGeneric([this, basePathName]() {
		ASSERT_TRUE(pasteFromExt(basePathName, {"end"}, true));
		auto start = findExt("start");
		auto end = findExt("end");
		checkLinks({{{start, {"outputs", "v"}}, {end, {"inputs", "v"}}}});
		ASSERT_TRUE(pasteFromExt(basePathName, {"end"}, true));
		checkLinks({{{start, {"outputs", "v"}}, {end, {"inputs", "v"}}}});
	});
}

TEST_F(ExtrefTest, duplicate_link_paste_chained_lua) {
	auto basePathName{(test_path() / "base.rca").string()};

	setupBase(basePathName, [this]() {
		TextFile scriptFile = makeFile("script.lua", R"(
function interface(IN,OUT)
	IN.v = Type:Vec3f()
	OUT.v = Type:Vec3f()
end
function run(IN,OUT)
end
)");
		auto start = create_lua("start", scriptFile);
		auto mid = create_lua("mid", scriptFile);
		auto end = create_lua("end", scriptFile);
		cmd->addLink({start, {"outputs", "v"}}, {mid, {"inputs", "v"}});
		cmd->addLink({mid, {"outputs", "v"}}, {end, {"inputs", "v"}});
	});

	setupGeneric([this, basePathName]() {
		ASSERT_TRUE(pasteFromExt(basePathName, {"mid"}, true));
		auto start = findExt("start");
		auto mid = findExt("mid");
		checkLinks({{{start, {"outputs", "v"}}, {mid, {"inputs", "v"}}}});
		ASSERT_TRUE(pasteFromExt(basePathName, {"mid", "end"}, true));
		auto end = findExt("end");
		checkLinks({{{start, {"outputs", "v"}}, {mid, {"inputs", "v"}}},
			{{mid, {"outputs", "v"}}, {end, {"inputs", "v"}}}});
	});
}

TEST_F(ExtrefTest, nesting_create) {
	auto basePathName{(test_path() / "base.rca").string()};
	auto midPathName((test_path() / "mid.rca").string());
	auto compositePathName{(test_path() / "composite.rca").string()};

	std::string base_id;
	std::string mid_id;

	setupBase(basePathName, [this, &base_id]() {
		// auto mesh = create_mesh("mesh", "meshes/Duck.glb");
		auto mesh = create<Mesh>("mesh");
		base_id = project->projectID();
	});

	setupComposite(
		basePathName, midPathName, {"mesh"}, [this, &mid_id]() {
			auto prefab = create<Prefab>("prefab");
			auto meshnode = create<MeshNode>("prefab_child", prefab);
			auto mesh = findExt("mesh");
			cmd->set({meshnode, {"mesh"}}, mesh);
			mid_id = project->projectID();
		},
		"mid");

	setupComposite(midPathName, compositePathName, {"prefab"}, [this, base_id, mid_id]() {
		auto prefab = findExt("prefab", mid_id);
		auto meshnode = findExt<MeshNode>("prefab_child", mid_id);
		auto mesh = findExt("mesh", base_id);
		ASSERT_EQ(prefab->children_->asVector<SEditorObject>(), std::vector<SEditorObject>({meshnode}));
		ASSERT_EQ(*meshnode->mesh_, mesh);
	});
}

TEST_F(ExtrefTest, filecopy_update_fail_nested_same_object) {
	auto basePathName1{(test_path() / "base1.rca").string()};
	auto basePathName2{(test_path() / "base2.rca").string()};
	auto midPathName((test_path() / "mid.rca").string());
	auto compositePathName{(test_path() / "composite.rca").string()};

	setupBase(
		basePathName1, [this]() {
			auto mesh = create<Mesh>("mesh");
		},
		std::string("base"));

	std::filesystem::copy(basePathName1, basePathName2);
	updateBase(basePathName2, [this]() {
		rename_project("copy");
	});

	setupBase(
		midPathName, [this]() {
			auto prefab = create<Prefab>("prefab");
			auto meshnode = create<MeshNode>("prefab_child", prefab);
		},
		"mid");

	setupBase(
		compositePathName, [this, basePathName1, midPathName]() {
			ASSERT_TRUE(pasteFromExt(basePathName1, {"mesh"}, true));
			ASSERT_TRUE(pasteFromExt(midPathName, {"prefab"}, true));
			auto mesh = findExt<Mesh>("mesh");
			auto prefab = findExt<Prefab>("prefab");

			auto meshnode = create<MeshNode>("meshnode");
			cmd->set({meshnode, {"mesh"}}, mesh);
			auto inst = create_prefabInstance("inst", prefab);
		},
		"composite");

	updateBase(midPathName, [this, basePathName2]() {
		ASSERT_TRUE(pasteFromExt(basePathName2, {"mesh"}, true));
		auto mesh = findExt<Mesh>("mesh");

		auto meshnode = find("prefab_child");
		cmd->set({meshnode, {"mesh"}}, mesh);
	});

	EXPECT_THROW(updateComposite(compositePathName, [this]() {}), raco::core::ExtrefError);
}

TEST_F(ExtrefTest, filecopy_update_fail_nested_different_object) {
	auto basePathName1{(test_path() / "base1.rca").string()};
	auto basePathName2{(test_path() / "base2.rca").string()};
	auto midPathName((test_path() / "mid.rca").string());
	auto compositePathName{(test_path() / "composite.rca").string()};

	setupBase(
		basePathName1, [this]() {
			auto mesh = create<Mesh>("mesh");
			auto material = create<Material>("material");
		},
		std::string("base"));

	std::filesystem::copy(basePathName1, basePathName2);
	updateBase(basePathName2, [this]() {
		rename_project("copy");
	});

	setupBase(
		midPathName, [this]() {
			auto prefab = create<Prefab>("prefab");
			auto meshnode = create<MeshNode>("prefab_child", prefab);
		},
		"mid");

	setupBase(
		compositePathName, [this, basePathName1, midPathName]() {
			ASSERT_TRUE(pasteFromExt(basePathName1, {"material"}, true));
			ASSERT_TRUE(pasteFromExt(midPathName, {"prefab"}, true));
			auto material = findExt<Material>("material");
			auto prefab = findExt<Prefab>("prefab");
			auto mesh = create_mesh("mesh_comp", "meshes/Duck.glb");
			auto meshnode = create_meshnode("meshnode", mesh, material);
			auto inst = create_prefabInstance("inst", prefab);
		},
		"composite");

	updateBase(midPathName, [this, basePathName2]() {
		ASSERT_TRUE(pasteFromExt(basePathName2, {"mesh"}, true));

		auto meshnode = find("prefab_child");
		auto mesh = findExt("mesh");
		cmd->set({meshnode, {"mesh"}}, mesh);
	});

	EXPECT_THROW(updateComposite(compositePathName, [this]() {}), raco::core::ExtrefError);
}

TEST_F(ExtrefTest, nesting_create_loop_fail) {
	auto basePathName{(test_path() / "base.rca").string()};
	auto compositePathName{(test_path() / "composite.rca").string()};

	setupBase(basePathName, [this]() {
		auto prefab = create<Prefab>("prefab");
	});

	setupComposite(
		basePathName, compositePathName, {"prefab"}, [this]() {
			auto prefab_base = findExt<Prefab>("prefab");
			auto prefab_comp = create<Prefab>("prefab_comp");
			auto inst_base = create_prefabInstance("inst", prefab_base);
		},
		"composite");

	updateComposite(basePathName, [this, compositePathName]() {
		ASSERT_FALSE(pasteFromExt(compositePathName, {"prefab_comp"}, true));
		ASSERT_EQ(project->instances().size(), 2);	// ProjectSettings and prefab
	});
}

TEST_F(ExtrefTest, paste_fail_precheckExternalReferenceUpdate_project_loop_nested) {
	auto basePathName{(test_path() / "base.rca").string()};
	auto compositePathName{(test_path() / "composite.rca").string()};

	setupBase(basePathName, [this]() {
		auto prefab = create<Prefab>("prefab");
	});

	setupComposite(
		basePathName, compositePathName, {"prefab"}, [this]() {
			auto prefab_base = findExt<Prefab>("prefab");
			auto prefab_comp = create<Prefab>("prefab_comp");
			auto inst_base = create_prefabInstance("inst", prefab_base);
		},
		"composite");

	auto serialized = copyExternalObjects(compositePathName, {"prefab_comp"});

	updateComposite(basePathName, [this, serialized]() {
		EXPECT_EQ(project->instances().size(), 2); // ProjectSettings, prefab
		EXPECT_THROW(cmd->pasteObjects(serialized, {}, true), std::runtime_error);
		EXPECT_EQ(project->instances().size(), 2);	// ProjectSettings, prefab
		EXPECT_TRUE(project->externalProjectsMap().empty());
	});
}

TEST_F(ExtrefTest, nested_shared_material_update) {
	auto basePathName{(test_path() / "base.rca").string()};
	auto duckPathName((test_path() / "duck.rca").string());
	auto compositePathName{(test_path() / "composite.rca").string()};

	setupBase(basePathName, [this]() {
		auto material = create_material("material", "shaders/basic.vert", "shaders/basic.frag");
	});

	setupComposite(
		basePathName, duckPathName, {"material"}, [this]() {
			auto prefab = create<Prefab>("prefab_duck");
			auto mesh = create_mesh("mesh", "meshes/Duck.glb");
			auto material = findExt<Material>("material");
			auto meshnode = create_meshnode("meshnode", mesh, material, prefab);
		},
		"duck");

	setupComposite(duckPathName, compositePathName, {"prefab_duck"}, [this]() {
		auto prefab = findExt<Prefab>("prefab_duck");
		auto inst = create_prefabInstance("inst", prefab);
	});

	updateBase(basePathName, [this]() {
		auto material = find("material");
		cmd->set({material, {"uniforms", "u_color", "x"}}, 0.5);
	});

	updateComposite(compositePathName, [this]() {
		auto material = findExt<Material>("material");
		ASSERT_EQ(ValueHandle(material, {"uniforms", "u_color", "x"}).asDouble(), 0.5);
	});
}

TEST_F(ExtrefTest, meshnode_uniform_refs_private_material) {
	auto basePathName{(test_path() / "base.rca").string()};
	auto compositePathName{(test_path() / "composite.rca").string()};

	setupBase(basePathName, [this]() {
		auto mesh = create_mesh("mesh", "meshes/Duck.glb");
		auto material = create_material("material", "shaders/simple_texture.vert", "shaders/simple_texture.frag");
		auto texture = create<Texture>("texture");
		auto prefab = create<Prefab>("prefab_duck");
		auto meshnode = create_meshnode("meshnode", mesh, material, prefab);
		cmd->set(meshnode->getMaterialPrivateHandle(0), true);
		cmd->set({meshnode, {"materials", "material", "uniforms", "u_Tex"}}, texture);
	});

	setupComposite(basePathName, compositePathName, {"prefab_duck"}, [this]() {
		auto texture = findExt<Texture>("texture");
		auto material = findExt<Material>("material");
		auto mesh = findExt<Mesh>("mesh");
	});
}

TEST_F(ExtrefTest, meshnode_uniform_refs_shared_material) {
	auto basePathName{(test_path() / "base.rca").string()};
	auto compositePathName{(test_path() / "composite.rca").string()};

	setupBase(basePathName, [this]() {
		auto mesh = create_mesh("mesh", "meshes/Duck.glb");
		auto material = create_material("material", "shaders/simple_texture.vert", "shaders/simple_texture.frag");
		auto texture = create<Texture>("texture");
		auto prefab = create<Prefab>("prefab_duck");
		auto meshnode = create_meshnode("meshnode", mesh, material, prefab);
		ASSERT_FALSE(meshnode->materialPrivate(0));
		ASSERT_EQ(meshnode->getUniformContainer(0)->size(), 0);

		cmd->set(meshnode->getMaterialPrivateHandle(0), true);
		ASSERT_EQ(meshnode->getUniformContainer(0)->size(), 1);

		cmd->set({meshnode, {"materials", "material", "uniforms", "u_Tex"}}, texture);
		cmd->set(meshnode->getMaterialPrivateHandle(0), false);
		ASSERT_EQ(meshnode->getUniformContainer(0)->size(), 0);
	});

	setupComposite(basePathName, compositePathName, {"prefab_duck"}, [this]() {
		dontFind("texture");
		auto material = findExt<Material>("material");
		auto mesh = findExt<Mesh>("mesh");
	});
}

TEST_F(ExtrefTest, nested_shared_material_linked_update) {
	auto basePathName{(test_path() / "base.rca").string()};
	auto duckPathName((test_path() / "duck.rca").string());
	auto compositePathName{(test_path() / "composite.rca").string()};

	setupBase(basePathName, [this]() {
		auto material = create_material("material", "shaders/basic.vert", "shaders/basic.frag");
		TextFile scriptFile = makeFile("script.lua", R"(
function interface(IN,OUT)
	IN.scalar = Type:Float()
	OUT.color = Type:Vec3f()
end
function run(IN,OUT)
	OUT.color = {IN.scalar, IN.scalar, 0.0}
end
)");
		auto lua = create_lua("global_control", scriptFile);
		cmd->addLink({lua, {"outputs", "color"}}, {material, {"uniforms", "u_color"}});
	});

	setupComposite(
		basePathName, duckPathName, {"material"}, [this]() {
			auto prefab = create<Prefab>("prefab_duck");
			auto mesh = create_mesh("mesh", "meshes/Duck.glb");
			auto material = findExt<Material>("material");
			auto meshnode = create_meshnode("meshnode", mesh, material, prefab);
		},
		"duck");

	setupComposite(duckPathName, compositePathName, {"prefab_duck"}, [this]() {
		auto prefab = findExt<Prefab>("prefab_duck");
		auto inst = create_prefabInstance("inst", prefab);
		auto material = findExt<Material>("material");
		ASSERT_EQ(ValueHandle(material, {"uniforms", "u_color", "x"}).asDouble(), 0.0);
	});

	updateBase(basePathName, [this]() {
		auto lua = find("global_control");
		cmd->set({lua, {"inputs", "scalar"}}, 0.5);
	});

	updateComposite(compositePathName, [this]() {
		app->doOneLoop();
		auto material = findExt<Material>("material");
		auto lua = findExt<LuaScript>("global_control");
		ASSERT_EQ(ValueHandle(material, {"uniforms", "u_color", "x"}).asDouble(), 0.5);
	});
}

TEST_F(ExtrefTest, shared_material_stacked_lua_linked_update) {
	auto basePathName{(test_path() / "base.rca").string()};
	auto compositePathName{(test_path() / "composite.rca").string()};

	setupBase(basePathName, [this]() {
		auto material = create_material("material", "shaders/basic.vert", "shaders/basic.frag");
		TextFile scriptFile = makeFile("script.lua", R"(
function interface(IN,OUT)
	IN.scalar = Type:Float()
	OUT.color = Type:Vec3f()
end
function run(IN,OUT)
	OUT.color = {IN.scalar, IN.scalar, 0.0}
end
)");
		auto lua = create_lua("global_control", scriptFile);
		cmd->addLink({lua, {"outputs", "color"}}, {material, {"uniforms", "u_color"}});

		TextFile masterScriptFile = makeFile("master.lua", R"(
function interface(IN,OUT)
	IN.scalar = Type:Float()
	OUT.mat = Type:Float()
end
function run(IN,OUT)
	OUT.mat = 3 * IN.scalar;
end
)");
		auto master = create_lua("master_control", masterScriptFile);
		cmd->addLink({master, {"outputs", "mat"}}, {lua, {"inputs", "scalar"}});
	});

	setupComposite(basePathName, compositePathName, {"material"}, [this]() {
		auto mesh = create_mesh("mesh", "meshes/Duck.glb");
		auto material = findExt<Material>("material");
		auto meshnode = create_meshnode("meshnode", mesh, material);
		ASSERT_EQ(ValueHandle(material, {"uniforms", "u_color", "x"}).asDouble(), 0.0);
	});

	updateBase(basePathName, [this]() {
		auto lua = find("master_control");
		cmd->set({lua, {"inputs", "scalar"}}, 0.5);
	});

	updateComposite(compositePathName, [this]() {
		app->doOneLoop();
		auto material = findExt<Material>("material");
		ASSERT_EQ(ValueHandle(material, {"uniforms", "u_color", "x"}).asDouble(), 1.5);
	});
}

TEST_F(ExtrefTest, diamond_shared_material_linked_update) {
	auto basePathName{(test_path() / "base.rca").string()};
	auto duckPathName((test_path() / "duck.rca").string());
	auto quadPathName((test_path() / "quad.rca").string());
	auto compositePathName{(test_path() / "composite.rca").string()};

	setupBase(basePathName, [this]() {
		auto material = create_material("material", "shaders/basic.vert", "shaders/basic.frag");
		TextFile scriptFile = makeFile("script.lua", R"(
function interface(IN,OUT)
	IN.scalar = Type:Float()
	OUT.color = Type:Vec3f()
end
function run(IN,OUT)
	OUT.color = {IN.scalar, IN.scalar, 0.0}
end
)");
		auto lua = create_lua("global_control", scriptFile);
		cmd->addLink({lua, {"outputs", "color"}}, {material, {"uniforms", "u_color"}});
	});

	setupComposite(
		basePathName, duckPathName, {"material"}, [this]() {
			auto prefab = create<Prefab>("prefab_duck");
			auto mesh = create_mesh("mesh", "meshes/Duck.glb");
			auto material = findExt<Material>("material");
			auto meshnode = create_meshnode("meshnode", mesh, material, prefab);

			ASSERT_EQ(countInstances<Material>(), 1);
		},
		"duck");

	setupComposite(
		basePathName, quadPathName, {"material"}, [this]() {
			auto prefab = create<Prefab>("prefab_quad");
			auto mesh = create_mesh("mesh", "meshes/defaultQuad.gltf");
			auto material = findExt<Material>("material");
			auto meshnode = create_meshnode("meshnode", mesh, material, prefab);

			ASSERT_EQ(countInstances<Material>(), 1);
		},
		"quad");

	setupBase(compositePathName, [this, duckPathName, quadPathName]() {
		pasteFromExt(duckPathName, {"prefab_duck"}, true);
		pasteFromExt(quadPathName, {"prefab_quad"}, true);
		auto prefab_duck = findExt<Prefab>("prefab_duck");
		auto prefab_quad = findExt<Prefab>("prefab_quad");

		ASSERT_EQ(countInstances<LuaScript>(), 1);
		ASSERT_EQ(countInstances<Material>(), 1);

		auto material = findExt<Material>("material");
		ASSERT_EQ(ValueHandle(material, {"uniforms", "u_color", "x"}).asDouble(), 0.0);

		auto inst_duck = create_prefabInstance("inst_duck", prefab_duck);
		auto inst_quad = create_prefabInstance("inst_quad", prefab_quad);
	});

	updateBase(basePathName, [this]() {
		auto lua = find("global_control");
		cmd->set({lua, {"inputs", "scalar"}}, 0.5);
	});

	updateComposite(compositePathName, [this]() {
		app->doOneLoop();
		auto material = findExt<Material>("material");
		ASSERT_EQ(ValueHandle(material, {"uniforms", "u_color", "x"}).asDouble(), 0.5);
	});
}

TEST_F(ExtrefTest, diamond_shared_material_linked_move_lua) {
	auto basePathName{(test_path() / "base.rca").string()};
	auto duckPathName((test_path() / "duck.rca").string());
	auto quadPathName((test_path() / "quad.rca").string());
	auto compositePathName{(test_path() / "composite.rca").string()};

	setupBase(basePathName, [this]() {
		auto material = create_material("material", "shaders/basic.vert", "shaders/basic.frag");

		TextFile scriptFile = makeFile("script.lua", R"(
function interface(IN,OUT)
	IN.scalar = Type:Float()
	OUT.color = Type:Vec3f()
end
function run(IN,OUT)
	OUT.color = {IN.scalar, IN.scalar, 0.0}
end
)");
		auto lua = create_lua("global_control", scriptFile);
		cmd->addLink({lua, {"outputs", "color"}}, {material, {"uniforms", "u_color"}});
	});

	setupComposite(
		basePathName, duckPathName, {"material"}, [this]() {
			auto prefab = create<Prefab>("prefab_duck");
			auto mesh = create_mesh("mesh", "meshes/Duck.glb");
			auto material = findExt<Material>("material");
			auto meshnode = create_meshnode("meshnode", mesh, material, prefab);

			ASSERT_EQ(countInstances<Material>(), 1);
		},
		"duck");

	setupComposite(
		basePathName, quadPathName, {"material"}, [this]() {
			auto prefab = create<Prefab>("prefab_quad");
			auto mesh = create_mesh("mesh", "meshes/defaultQuad.gltf");
			auto material = findExt<Material>("material");
			auto meshnode = create_meshnode("meshnode", mesh, material, prefab);

			ASSERT_EQ(countInstances<Material>(), 1);
		},
		"quad");

	setupBase(compositePathName, [this, duckPathName, quadPathName]() {
		pasteFromExt(duckPathName, {"prefab_duck"}, true);
		pasteFromExt(quadPathName, {"prefab_quad"}, true);
		auto prefab_duck = findExt<Prefab>("prefab_duck");
		auto prefab_quad = findExt<Prefab>("prefab_quad");

		ASSERT_EQ(countInstances<LuaScript>(), 1);
		ASSERT_EQ(countInstances<Material>(), 1);

		auto material = findExt<Material>("material");
		ASSERT_EQ(ValueHandle(material, {"uniforms", "u_color", "x"}).asDouble(), 0.0);

		auto inst_duck = create_prefabInstance("inst_duck", prefab_duck);
		auto inst_quad = create_prefabInstance("inst_quad", prefab_quad);
	});

	updateBase(basePathName, [this]() {
		auto node = create<Node>("dummyNode");
		auto lua = find("global_control");
		cmd->moveScenegraphChildren({lua}, node);
		cmd->set({lua, {"inputs", "scalar"}}, 0.5);
	});

	updateComposite(compositePathName, [this]() {
		app->doOneLoop();
		ASSERT_EQ(countInstances<LuaScript>(), 0);
		ASSERT_EQ(countInstances<Material>(), 1);
		auto material = findExt<Material>("material");
		ASSERT_EQ(ValueHandle(material, {"uniforms", "u_color", "x"}).asDouble(), 0.0);
	});
}

TEST_F(ExtrefTest, saveas_reroot_uri_lua) {
	auto basePathName{(test_path() / "base.rca").string()};

	auto base_id = setupBase(basePathName, [this, basePathName]() {
		project->setCurrentPath(basePathName);

		auto prefab = create<Prefab>("prefab");
		auto lua = create<LuaScript>("lua", prefab);
		cmd->set({lua, {"uri"}}, std::string("relativeURI"));
	});

	std::filesystem::create_directory((test_path() / "subdir"));
	setupGeneric([this, basePathName, base_id]() {
		project->setCurrentPath((test_path() / "project.file").string());

		pasteFromExt(basePathName, {"prefab"}, true);
		auto prefab = findExt("prefab");
		auto lua_prefab = findExt<LuaScript>("lua");

		auto inst = create<PrefabInstance>("inst");
		cmd->set({inst, {"template"}}, prefab);

		auto inst_children = inst->children_->asVector<SEditorObject>();
		ASSERT_EQ(inst_children.size(), 1);
		auto lua_inst = inst_children[0]->as<LuaScript>();
		ASSERT_TRUE(lua_inst != nullptr);

		auto lua_local = create<LuaScript>("lua_local");
		cmd->set({lua_local, {"uri"}}, std::string("relativeURI"));

		EXPECT_EQ(*lua_prefab->uri_, std::string("relativeURI"));
		EXPECT_EQ(*lua_inst->uri_, std::string("relativeURI"));
		EXPECT_EQ(*lua_local->uri_, std::string("relativeURI"));

		ASSERT_EQ(project->externalProjectsMap().at(base_id).path, "base.rca");

		std::string msg;
		ASSERT_TRUE(app->activeRaCoProject().saveAs((test_path() / "subdir" / "project.file").string().c_str(), msg));

		EXPECT_EQ(*lua_prefab->uri_, std::string("relativeURI"));
		EXPECT_EQ(*lua_inst->uri_, std::string("relativeURI"));
		EXPECT_EQ(*lua_local->uri_, std::string("../relativeURI"));

		ASSERT_EQ(project->externalProjectsMap().at(base_id).path, "../base.rca");
	});
}

TEST_F(ExtrefTest, paste_reroot_lua_uri_dir_up) {
	auto basePathName{(test_path() / "base.rca").string()};

	auto base_id = setupBase(basePathName, [this, basePathName]() {
		project->setCurrentPath(basePathName);

		auto prefab = create<Prefab>("prefab");
		auto lua = create<LuaScript>("lua", prefab);
		cmd->set({lua, {"uri"}}, std::string("relativeURI"));
	});

	setupGeneric([this, basePathName, base_id]() {
		project->setCurrentPath((test_path() / "subdir" / "project.file").string());

		pasteFromExt(basePathName, {"prefab"}, true);
		auto prefab = findExt("prefab");
		auto lua_prefab = findExt<LuaScript>("lua");

		EXPECT_EQ(*lua_prefab->uri_, std::string("relativeURI"));
		ASSERT_EQ(project->externalProjectsMap().at(base_id).path, "../base.rca");

		auto pasted = cmd->pasteObjects(cmd->copyObjects({lua_prefab}));
		EXPECT_EQ(*pasted[0]->as<LuaScript>()->uri_, std::string("../relativeURI"));

		auto inst = create<PrefabInstance>("inst");
		cmd->set({inst, {"template"}}, prefab);
		auto lua_inst = inst->children_->asVector<SEditorObject>()[0]->as<LuaScript>();
		EXPECT_EQ(*lua_inst->uri_, std::string("relativeURI"));

		auto pasted_lua_inst = cmd->pasteObjects(cmd->copyObjects({lua_inst}));
		EXPECT_EQ(*pasted_lua_inst[0]->as<LuaScript>()->uri_, std::string("../relativeURI"));

		auto pasted_inst = cmd->pasteObjects(cmd->copyObjects({inst}));
		EXPECT_EQ(pasted_inst.size(), 1);
		auto pasted_inst_child_lua = pasted_inst[0]->children_->asVector<SEditorObject>()[0]->as<LuaScript>();
		EXPECT_EQ(*pasted_inst_child_lua->uri_, std::string("relativeURI"));
	});
}

TEST_F(ExtrefTest, paste_reroot_lua_uri_dir_down) {
	std::filesystem::create_directory((test_path() / "subdir"));
	auto basePathName{(test_path() / "subdir" / "base.rca").string()};

	auto base_id = setupBase(basePathName, [this, basePathName]() {
		project->setCurrentPath(basePathName);

		auto prefab = create<Prefab>("prefab");
		auto lua = create<LuaScript>("lua", prefab);
		cmd->set({lua, {"uri"}}, std::string("relativeURI"));
	});

	setupGeneric([this, basePathName, base_id]() {
		project->setCurrentPath((test_path() / "project.file").string());

		pasteFromExt(basePathName, {"prefab"}, true);
		auto prefab = findExt("prefab");
		auto lua_prefab = findExt<LuaScript>("lua");

		EXPECT_EQ(*lua_prefab->uri_, std::string("relativeURI"));
		ASSERT_EQ(project->externalProjectsMap().at(base_id).path, "subdir/base.rca");

		auto pasted = cmd->pasteObjects(cmd->copyObjects({lua_prefab}));
		EXPECT_EQ(*pasted[0]->as<LuaScript>()->uri_, std::string("subdir/relativeURI"));

		auto inst = create<PrefabInstance>("inst");
		cmd->set({inst, {"template"}}, prefab);
		auto lua_inst = inst->children_->asVector<SEditorObject>()[0]->as<LuaScript>();
		EXPECT_EQ(*lua_inst->uri_, std::string("relativeURI"));

		auto pasted_lua_inst = cmd->pasteObjects(cmd->copyObjects({lua_inst}));
		EXPECT_EQ(*pasted_lua_inst[0]->as<LuaScript>()->uri_, std::string("subdir/relativeURI"));

		auto pasted_inst = cmd->pasteObjects(cmd->copyObjects({inst}));
		EXPECT_EQ(pasted_inst.size(), 1);
		auto pasted_inst_child_lua = pasted_inst[0]->children_->asVector<SEditorObject>()[0]->as<LuaScript>();
		EXPECT_EQ(*pasted_inst_child_lua->uri_, std::string("relativeURI"));
	});
}

TEST_F(ExtrefTest, paste_reroot_lua_uri_with_link_down) {
	std::filesystem::create_directory((test_path() / "subdir"));
	auto basePathName{(test_path() / "subdir" / "base.rca").string()};
	auto compositePathName{(test_path() / "composite.rca").string()};

	setupBase(basePathName, [this, basePathName]() {
		project->setCurrentPath(basePathName);

		auto prefab = create<Prefab>("prefab");
		auto node = create<Node>("prefab_node", prefab);
		auto lua = create<LuaScript>("prefab_lua", prefab);

		raco::utils::file::write((test_path() / "subdir/script.lua").string(), R"(
function interface(IN,OUT)
	IN.v = Type:Vec3f()
	OUT.v = Type:Vec3f()
end
function run(IN,OUT)
end
)");

		cmd->set({lua, {"uri"}}, std::string("script.lua"));
		cmd->addLink({lua, {"outputs", "v"}}, {node, {"translation"}});
	});

	setupGeneric([this, basePathName]() {
		project->setCurrentPath((test_path() / "project.file").string());

		pasteFromExt(basePathName, {"prefab"}, true);
		auto prefab = findExt("prefab");
		auto prefab_lua = findExt<LuaScript>("prefab_lua");
		auto prefab_node = findExt<Node>("prefab_node");

		EXPECT_EQ(*prefab_lua->uri_, std::string("script.lua"));
		checkLinks({{{prefab_lua, {"outputs", "v"}}, {prefab_node, {"translation"}}}});

		auto inst = create<PrefabInstance>("inst");
		cmd->set({inst, {"template"}}, prefab);
		auto inst_lua = findChild<LuaScript>(inst);
		auto inst_node = findChild<Node>(inst);
		EXPECT_EQ(*inst_lua->uri_, std::string("script.lua"));
		checkLinks({{{prefab_lua, {"outputs", "v"}}, {prefab_node, {"translation"}}},
			{{inst_lua, {"outputs", "v"}}, {inst_node, {"translation"}}}});

		auto pasted_inst = cmd->pasteObjects(cmd->copyObjects({inst}));
		EXPECT_EQ(pasted_inst.size(), 1);
		auto pasted_inst_lua = findChild<LuaScript>(pasted_inst[0]);
		auto pasted_inst_node = findChild<Node>(pasted_inst[0]);
		EXPECT_EQ(*pasted_inst_lua->uri_, std::string("script.lua"));
		checkLinks({{{prefab_lua, {"outputs", "v"}}, {prefab_node, {"translation"}}},
			{{inst_lua, {"outputs", "v"}}, {inst_node, {"translation"}}},
			{{pasted_inst_lua, {"outputs", "v"}}, {pasted_inst_node, {"translation"}}}});
	});
}

TEST_F(ExtrefTest, prefab_link_quaternion_in_prefab) {
	auto basePathName1{(test_path() / "base.rca").string()};
	auto basePathName2{(test_path() / "base2.rca").string()};

	setupBase(basePathName1, [this, basePathName1]() {
		project->setCurrentPath(basePathName1);

		raco::utils::file::write((test_path() / "script.lua").string(), R"(
function interface(IN,OUT)
	IN.v = Type:Vec4f()
	OUT.v = Type:Vec4f()
end
function run(IN,OUT)
OUT.v = IN.v
end
)");

		auto prefab = create<Prefab>("prefab");
		auto node = create<Node>("prefab_node", prefab);
		auto lua = create_lua("prefab_lua", "script.lua", prefab);

		cmd->moveScenegraphChildren({node}, prefab);
		cmd->moveScenegraphChildren({lua}, prefab);
		cmd->addLink({lua, {"outputs", "v"}}, {node, {"rotation"}});
	});

	setupBase(basePathName2, [this, basePathName1, basePathName2]() {
		project->setCurrentPath(basePathName2);
		pasteFromExt(basePathName1, {"prefab"}, true);
		auto prefab = findExt("prefab");
		auto prefab_lua = findExt<LuaScript>("prefab_lua");
		auto prefab_node = findExt<Node>("prefab_node");

		auto inst = create<PrefabInstance>("inst");
		cmd->set({inst, {"template"}}, prefab);

		auto inst_children = inst->children_->asVector<SEditorObject>();
		auto inst_node = inst_children[0]->as<Node>();
		auto inst_lua = inst_children[1]->as<LuaScript>();

		checkLinks({{{prefab_lua, {"outputs", "v"}}, {prefab_node, {"rotation"}}},
			{{inst_lua, {"outputs", "v"}}, {inst_node, {"rotation"}}}});
	});
}

TEST_F(ExtrefTest, animation_channel_data_gets_propagated) {
	auto basePathName1{(test_path() / "base.rca").string()};
	auto basePathName2{(test_path() / "base2.rca").string()};
	auto compositePathName{(test_path() / "composite.rca").string()};

	setupBase(basePathName1, [this]() {
		auto animChannel = create_animationchannel("animCh", "meshes/InterpolationTest/InterpolationTest.gltf");
	});

	setupBase(basePathName2, [this, basePathName1]() {
		ASSERT_TRUE(pasteFromExt(basePathName1, {"animCh"}, true));

		auto anim = create<Animation>("anim");
		auto animChannel = findExt<AnimationChannel>("animCh");
		cmd->set({anim, {"animationChannels", "Channel 0"}}, animChannel);

		ASSERT_EQ(anim->get("animationChannels")->asTable()[0]->asRef(), animChannel);
	});
}

TEST_F(ExtrefTest, animation_in_extref_prefab_gets_propagated) {
	auto basePathName1{(test_path() / "base.rca").string()};
	auto basePathName2{(test_path() / "base2.rca").string()};

	setupBase(basePathName1, [this]() {
		auto anim = create<Animation>("anim");
		auto animNode = create<Node>("node");
		cmd->moveScenegraphChildren({anim}, animNode);

		auto prefab = create<Prefab>("prefab");
		cmd->moveScenegraphChildren({animNode}, prefab);

		auto animChannel = create_animationchannel("animCh", "meshes/InterpolationTest/InterpolationTest.gltf");
		cmd->set({anim, {"animationChannels", "Channel 0"}}, animChannel);
	});

	setupBase(basePathName2, [this, basePathName1]() {
		ASSERT_TRUE(pasteFromExt(basePathName1, {"prefab"}, true));

		auto anim = findExt<Animation>("anim");
		auto animChannel = findExt<AnimationChannel>("animCh");

		ASSERT_EQ(anim->get("animationChannels")->asTable()[0]->asRef(), animChannel);
	});
}

TEST_F(ExtrefTest, prefab_cut_deep_linked_does_not_delete_shared) {
	auto basePathName{(test_path() / "base.rca").string()};
	auto compositePathName{(test_path() / "composite.rca").string()};

	setupBase(basePathName, [this]() {
		auto mesh1 = create_mesh("mesh1", "meshes/Duck.glb");
		auto mesh2 = create_mesh("mesh2", "meshes/Duck.glb");
		auto sharedMaterial = create_material("sharedMaterial", "shaders/basic.vert", "shaders/basic.frag");

		auto prefab1 = create<Prefab>("prefab1");
		auto prefab2 = create<Prefab>("prefab2");

		auto meshNode1 = create_meshnode("prefab1_meshNode", mesh1, sharedMaterial, prefab1);
		auto meshNode2 = create_meshnode("prefab2_meshNode", mesh2, sharedMaterial, prefab2);
	});

	setupComposite(basePathName, compositePathName, {"prefab1", "prefab2"}, [this]() {
		auto prefab1 = find("prefab1");
		auto prefab2 = find("prefab2");

		find("sharedMaterial");

		cmd->cutObjects({prefab2}, true);

		find("sharedMaterial");

		cmd->cutObjects({prefab1}, true);

		dontFind("sharedMaterial");
	});
}

TEST_F(ExtrefTest, module_gets_propagated) {
	auto basePathName1{(test_path() / "base.rca").string()};
	auto basePathName2{(test_path() / "base2.rca").string()};

	setupBase(basePathName1, [this]() {
		auto module = create<LuaScriptModule>("module");
		cmd->set({module, &LuaScriptModule::uri_}, (test_path() / "scripts" / "moduleDefinition.lua").string());
	});

	setupBase(basePathName2, [this, basePathName1]() {
		ASSERT_TRUE(pasteFromExt(basePathName1, {"module"}, true));
		auto lua = create_lua("script", "scripts/moduleDependency.lua");

		auto module = findExt<LuaScriptModule>("module");
		cmd->set({lua, {"luaModules", "coalas"}}, module);

		ASSERT_FALSE(cmd->errors().hasError({lua}));
	});
}

TEST_F(ExtrefTest, prefab_instance_with_lua_and_module) {
	auto basePathName1{(test_path() / "base.rca").string()};
	auto basePathName2{(test_path() / "base2.rca").string()};

	setupBase(basePathName1, [this]() {
		auto prefab = create<Prefab>("prefab");

		auto lua = create_lua("lua", "scripts/moduleDependency.lua", prefab);

		auto luaModule = create<LuaScriptModule>("luaModule");
		cmd->set({luaModule, &LuaScriptModule::uri_}, (test_path() / "scripts/moduleDefinition.lua").string());

		cmd->set({lua, {"luaModules", "coalas"}}, luaModule);
	});

	setupBase(basePathName2, [this, basePathName1]() {
		ASSERT_TRUE(pasteFromExt(basePathName1, {"prefab"}, true));
		auto lua = findExt<LuaScript>("lua");
		auto module = findExt<LuaScriptModule>("luaModule");

		ASSERT_FALSE(cmd->errors().hasError({lua}));
	});
}
TEST_F(ExtrefTest, prefab_instance_update_lua_and_module_remove_module_ref) {
	auto basePathName1{(test_path() / "base.rca").string()};
	auto basePathName2{(test_path() / "base2.rca").string()};

	setupBase(basePathName1, [this]() {
		auto prefab = create<Prefab>("prefab");

		auto lua = create_lua("lua", "scripts/moduleDependency.lua", prefab);

		auto luaModule = create<LuaScriptModule>("luaModule");
		cmd->set({luaModule, &LuaScriptModule::uri_}, (test_path() / "scripts/moduleDefinition.lua").string());

		cmd->set({lua, {"luaModules", "coalas"}}, luaModule);
	});

	setupBase(basePathName2, [this, basePathName1]() {
		ASSERT_TRUE(pasteFromExt(basePathName1, {"prefab"}, true));
		auto prefab = findExt<Prefab>("prefab");
		auto lua = findExt<LuaScript>("lua");
		auto module = findExt<LuaScriptModule>("luaModule");

		ASSERT_FALSE(cmd->errors().hasError(ValueHandle{lua, {"luaModules", "coalas"}}));

		auto inst = create_prefabInstance("inst", prefab);
	});

	updateBase(basePathName1, [this]() {
		auto lua = find("lua");
		cmd->set({lua, {"luaModules", "coalas"}}, SEditorObject{});
	});

	updateBase(basePathName2, [this]() {
		auto lua = findExt<LuaScript>("lua");
		auto module = findExt<LuaScriptModule>("luaModule");

		ASSERT_TRUE(cmd->errors().hasError(ValueHandle{lua, {"luaModules", "coalas"}}));
	});
}

TEST_F(ExtrefTest, prefab_instance_update_lua_and_module_change_lua_uri) {
	auto basePathName1{(test_path() / "base.rca").string()};
	auto basePathName2{(test_path() / "base2.rca").string()};

	setupBase(basePathName1, [this]() {
		auto prefab = create<Prefab>("prefab");

		auto lua = create_lua("lua", "scripts/moduleDependency.lua", prefab);

		auto luaModule = create<LuaScriptModule>("luaModule");
		cmd->set({luaModule, &LuaScriptModule::uri_}, (test_path() / "scripts/moduleDefinition.lua").string());

		cmd->set({lua, {"luaModules", "coalas"}}, luaModule);
	});

	setupBase(basePathName2, [this, basePathName1]() {
		ASSERT_TRUE(pasteFromExt(basePathName1, {"prefab"}, true));
		auto prefab = findExt<Prefab>("prefab");
		auto lua = findExt<LuaScript>("lua");
		auto module = findExt<LuaScriptModule>("luaModule");

		ASSERT_FALSE(cmd->errors().hasError({lua}));

		auto inst = create_prefabInstance("inst", prefab);
	});

	updateBase(basePathName1, [this]() {
		auto lua = find("lua");
		cmd->set({lua, &LuaScript::uri_}, (test_path() / "scripts/types-scalar.lua").string());
	});

	updateBase(basePathName2, [this]() {
		auto lua = findExt<LuaScript>("lua");
		auto module = findExt<LuaScriptModule>("luaModule");

		ASSERT_FALSE(cmd->errors().hasError({lua}));
	});
}

TEST_F(ExtrefTest, link_extref_to_local_reload) {
	auto basePathName{(test_path() / "base.rca").string()};
	auto compositePathName{(test_path() / "composite.rca").string()};

	TextFile scriptFile = makeFile("script.lua", R"(
function interface(IN,OUT)
	IN.v = Type:Vec3f()
	OUT.v = Type:Vec3f()
end
function run(IN,OUT)
end
)");

	setupBase(basePathName, [this, &scriptFile]() {
		auto luaSource = create_lua("luaSource", scriptFile);
	});

	setupComposite(basePathName, compositePathName, {"luaSource"}, [this, &scriptFile]() {
		auto luaSource = findExt("luaSource");

		auto luaSink = create_lua("luaSink", scriptFile);
		cmd->addLink({luaSource, {"outputs", "v"}}, {luaSink, {"inputs", "v"}});
		checkLinks({{{luaSource, {"outputs", "v"}}, {luaSink, {"inputs", "v"}}}});
	});

	updateBase(compositePathName, [this]() {
		auto luaSource = findExt("luaSource");
		auto luaSink = findLocal("luaSink");
		checkLinks({{{luaSource, {"outputs", "v"}}, {luaSink, {"inputs", "v"}}}});
		cmd->removeLink({luaSink, {"inputs", "v"}});
		EXPECT_EQ(project->links().size(), 0);
	});

	updateComposite(compositePathName, [this]() {
		dontFind("luaSource");
	});
}

TEST_F(ExtrefTest, link_extref_to_local_update_remove_source) {
	auto basePathName{(test_path() / "base.rca").string()};
	auto compositePathName{(test_path() / "composite.rca").string()};

	TextFile scriptFile = makeFile("script.lua", R"(
function interface(IN,OUT)
	IN.v = Type:Vec3f()
	OUT.v = Type:Vec3f()
end
function run(IN,OUT)
end
)");

	setupBase(basePathName, [this, &scriptFile]() {
		auto luaSource = create_lua("luaSource", scriptFile);
	});

	setupComposite(basePathName, compositePathName, {"luaSource"}, [this, &scriptFile]() {
		auto luaSource = findExt("luaSource");

		auto luaSink = create_lua("luaSink", scriptFile);
		cmd->addLink({luaSource, {"outputs", "v"}}, {luaSink, {"inputs", "v"}});

		EXPECT_EQ(project->links().size(), 1);
	});

	updateBase(basePathName, [this]() {
		auto luaSource = find("luaSource");
		cmd->deleteObjects({luaSource});
	});

	updateComposite(compositePathName, [this]() {
		EXPECT_EQ(project->links().size(), 0);
	});
}

TEST_F(ExtrefTest, link_extref_to_local_update_invalidate) {
	auto basePathName{(test_path() / "base.rca").string()};
	auto compositePathName{(test_path() / "composite.rca").string()};

	TextFile scriptFile = makeFile("script.lua", R"(
function interface(IN,OUT)
	IN.v = Type:Vec3f()
	OUT.v = Type:Vec3f()
end
function run(IN,OUT)
end
)");

	TextFile scriptFile_alt = makeFile("script-alt.lua", R"(
function interface(IN,OUT)
	IN.v = Type:float()
	OUT.v = Type:float()
end
function run(IN,OUT)
end
)");

	setupBase(basePathName, [this, &scriptFile]() {
		auto luaSource = create_lua("luaSource", scriptFile);
	});

	setupComposite(basePathName, compositePathName, {"luaSource"}, [this, &scriptFile]() {
		auto luaSource = findExt("luaSource");
		auto luaSink = create_lua("luaSink", scriptFile);
		cmd->addLink({luaSource, {"outputs", "v"}}, {luaSink, {"inputs", "v"}});
		checkLinks({{{luaSource, {"outputs", "v"}}, {luaSink, {"inputs", "v"}}, true, false}});
	});

	updateBase(basePathName, [this, &scriptFile_alt]() {
		auto luaSource = find("luaSource");
		cmd->set({luaSource, {"uri"}}, (test_path() / std::string(scriptFile_alt)).string());
	});

	updateComposite(compositePathName, [this]() {
		auto luaSource = findExt("luaSource");
		auto luaSink = findLocal("luaSink");
		checkLinks({{{luaSource, {"outputs", "v"}}, {luaSink, {"inputs", "v"}}, false, false}});
	});
}

TEST_F(ExtrefTest, timer_in_extref) {
	auto basePathName1{(test_path() / "base.rca").string()};
	auto basePathName2{(test_path() / "base2.rca").string()};

	setupBase(basePathName1, [this]() {
		auto timer = create<Timer>("timer");
		cmd->set({timer, {"inputs", "ticker_us"}}, int64_t{300});
	});

	setupBase(basePathName2, [this, basePathName1]() {
		ASSERT_TRUE(pasteFromExt(basePathName1, {"timer"}, true));
		app->doOneLoop();

		auto timer = findExt<Timer>("timer");
		ASSERT_EQ((ValueHandle{timer, {"inputs", "ticker_us"}}.asInt64()), int64_t{300});
		ASSERT_EQ((ValueHandle{timer, {"outputs", "ticker_us"}}.asInt64()), int64_t{300});
	});
}

TEST_F(ExtrefTest, extref_timer_with_local_prefabinstance) {
	auto basePathName1{(test_path() / "base.rca").string()};
	auto basePathName2{(test_path() / "base2.rca").string()};

	setupBase(basePathName1, [this]() {
		auto timer = create<Timer>("timer");
	});

	setupBase(basePathName2, [this, basePathName1]() {
		pasteFromExt(basePathName1, {"timer"}, true);
		auto timer = findExt<Timer>("timer");

		TextFile scriptFile = makeFile("script.lua", R"(
function interface(INOUT)
	INOUT.v = Type:Int64()
end
)");
		auto script1 = create<LuaInterface>("script1");
		cmd->set({script1, {"uri"}}, scriptFile);

		auto prefab = create<Prefab>("prefab");
		cmd->moveScenegraphChildren({script1}, prefab);

		auto prefabInst = create<PrefabInstance>("prefabInst");
		cmd->set({prefabInst, &PrefabInstance::template_}, prefab);
		app->doOneLoop();

		auto prefabInnerInstanceScript = prefabInst->children_->asVector<SEditorObject>().front();
		cmd->addLink({timer, {"outputs", "ticker_us"}}, {prefabInnerInstanceScript, {"inputs", "v"}});
		app->doOneLoop();

		ASSERT_NE((ValueHandle{prefabInnerInstanceScript, {"inputs", "v"}}.asInt64()), int64_t{0});
	});
}

TEST_F(ExtrefTest, extref_timer_with_extref_prefab_local_instance) {
	auto basePathName1{(test_path() / "base.rca").string()};
	auto basePathName2{(test_path() / "base2.rca").string()};
	auto basePathName3{(test_path() / "base3.rca").string()};

	setupBase(basePathName1, [this]() {
		auto timer = create<Timer>("timer");
	});

	setupBase(basePathName2, [this, basePathName1]() {
		TextFile scriptFile = makeFile("script.lua", R"(
function interface(INOUT)
	INOUT.v = Type:Int64()
end
)");
		auto script1 = create<LuaInterface>("script1");
		cmd->set({script1, {"uri"}}, scriptFile);

		auto prefab = create<Prefab>("prefab");
		cmd->moveScenegraphChildren({script1}, prefab);
	});

	setupBase(basePathName3, [this, basePathName1, basePathName2]() {
		pasteFromExt(basePathName1, {"timer"}, true);
		pasteFromExt(basePathName2, {"prefab"}, true);
		auto timer = findExt<Timer>("timer");
		auto prefabExt = findExt<Prefab>("prefab");

		auto prefabInst = create<PrefabInstance>("prefabInst");
		cmd->set({prefabInst, &PrefabInstance::template_}, prefabExt);
		app->doOneLoop();

		auto instanceScript = prefabInst->children_->asVector<SEditorObject>().front();
		cmd->addLink({timer, {"outputs", "ticker_us"}}, {instanceScript, {"inputs", "v"}});
		app->doOneLoop();

		ASSERT_NE((ValueHandle{instanceScript, {"inputs", "v"}}.asInt64()), int64_t{0});
	});
}

TEST_F(ExtrefTest, link_strong_to_weak_transition) {
	auto basePathName{(test_path() / "base.rca").string()};
	auto compositePathName{(test_path() / "composite.rca").string()};

	setupBase(basePathName, [this]() {
		auto start = create_lua("start", "scripts/types-scalar.lua");
		auto end = create_lua("end", "scripts/types-scalar.lua");
		cmd->addLink(ValueHandle{start, {"outputs", "ofloat"}}, ValueHandle{end, {"inputs", "float"}});
	});

	setupComposite(basePathName, compositePathName, {"start", "end"}, [this]() {
		auto start = findExt("start");
		auto end = findExt("end");
		checkLinks({{{start, {"outputs", "ofloat"}}, {end, {"inputs", "float"}}, true, false}});
		EXPECT_TRUE(project->createsLoop({end, {"outputs", "ofloat"}}, {start, {"inputs", "float"}}));

		auto local_start = create_lua("local_start", "scripts/types-scalar.lua");
		auto local_end = create_lua("local_end", "scripts/types-scalar.lua");
		cmd->addLink(ValueHandle{start, {"outputs", "ofloat"}}, ValueHandle{local_start, {"inputs", "float"}});
		cmd->addLink(ValueHandle{end, {"outputs", "ofloat"}}, ValueHandle{local_end, {"inputs", "float"}});
	});

	updateBase(basePathName, [this]() {
		auto start = find("start");
		auto end = find("end");
		cmd->removeLink({end, {"inputs", "float"}});
		cmd->addLink(ValueHandle{start, {"outputs", "ofloat"}}, ValueHandle{end, {"inputs", "float"}}, true);
	});

	updateComposite(compositePathName, [this]() {
		auto start = findExt("start");
		auto end = findExt("end");

		auto local_start = find("local_start");
		auto local_end = find("local_end");

		checkLinks({{{start, {"outputs", "ofloat"}}, {end, {"inputs", "float"}}, true, true},
			{{start, {"outputs", "ofloat"}}, {local_start, {"inputs", "float"}}, true, false},
			{{end, {"outputs", "ofloat"}}, {local_end, {"inputs", "float"}}, true, false}});
		EXPECT_FALSE(project->createsLoop({end, {"outputs", "ofloat"}}, {start, {"inputs", "float"}}));
	});
}

TEST_F(ExtrefTest, link_strong_valid_to_weak_invalid_transition) {
	auto basePathName{(test_path() / "base.rca").string()};
	auto compositePathName{(test_path() / "composite.rca").string()};

	setupBase(basePathName, [this]() {
		auto start = create_lua("start", "scripts/types-scalar.lua");
		auto end = create_lua("end", "scripts/types-scalar.lua");
		cmd->addLink(ValueHandle{start, {"outputs", "ofloat"}}, ValueHandle{end, {"inputs", "float"}});
	});

	setupComposite(basePathName, compositePathName, {"start", "end"}, [this]() {
		auto start = findExt("start");
		auto end = findExt("end");
		checkLinks({{{start, {"outputs", "ofloat"}}, {end, {"inputs", "float"}}, true, false}});
		EXPECT_TRUE(project->createsLoop({end, {"outputs", "ofloat"}}, {start, {"inputs", "float"}}));

		auto local_start = create_lua("local_start", "scripts/types-scalar.lua");
		auto local_end = create_lua("local_end", "scripts/types-scalar.lua");
		cmd->addLink(ValueHandle{start, {"outputs", "ofloat"}}, ValueHandle{local_start, {"inputs", "float"}});
		cmd->addLink(ValueHandle{end, {"outputs", "ofloat"}}, ValueHandle{local_end, {"inputs", "float"}});
	});

	updateBase(basePathName, [this]() {
		auto start = find("start");
		auto end = find("end");
		cmd->removeLink({end, {"inputs", "float"}});
		cmd->addLink(ValueHandle{start, {"outputs", "ofloat"}}, ValueHandle{end, {"inputs", "float"}}, true);
		cmd->set(ValueHandle{end, {"uri"}}, (test_path() / "scripts/SimpleScript.lua").string());
	});

	updateComposite(compositePathName, [this]() {
		auto start = findExt("start");
		auto end = findExt("end");

		auto local_start = find("local_start");
		auto local_end = find("local_end");

		checkLinks({{{start, {"outputs", "ofloat"}}, {end, {"inputs", "float"}}, false, true},
			{{start, {"outputs", "ofloat"}}, {local_start, {"inputs", "float"}}, true, false},
			{{end, {"outputs", "ofloat"}}, {local_end, {"inputs", "float"}}, false, false}});
		EXPECT_FALSE(project->createsLoop({end, {"outputs", "ofloat"}}, {start, {"inputs", "float"}}));
	});
}

TEST_F(ExtrefTest, link_weak_cycle) {
	auto basePathName{(test_path() / "base.rca").string()};
	auto compositePathName{(test_path() / "composite.rca").string()};

	setupBase(basePathName, [this]() {
		auto start = create_lua("start", "scripts/types-scalar.lua");
		auto end = create_lua("end", "scripts/types-scalar.lua");
		cmd->addLink(ValueHandle{start, {"outputs", "ofloat"}}, ValueHandle{end, {"inputs", "float"}});
		cmd->addLink(ValueHandle{end, {"outputs", "ofloat"}}, ValueHandle{start, {"inputs", "float"}}, true);

		checkLinks({{{start, {"outputs", "ofloat"}}, {end, {"inputs", "float"}}, true, false},
			{{end, {"outputs", "ofloat"}}, {start, {"inputs", "float"}}, true, true}});
	});

	setupComposite(basePathName, compositePathName, {"start", "end"}, [this]() {
		auto start = findExt("start");
		auto end = findExt("end");
		checkLinks({{{start, {"outputs", "ofloat"}}, {end, {"inputs", "float"}}, true, false},
			{{end, {"outputs", "ofloat"}}, {start, {"inputs", "float"}}, true, true}});
	});
}

TEST_F(ExtrefTest, feature_level_upgrade_composite) {
	auto basePathName{(test_path() / "base.rca").string()};
	auto compositePathName{(test_path() / "composite.rca").string()};

	setupBase(
		basePathName, [this]() {
			auto prefab = create<Prefab>("prefab");
		},
		"base", 1);

	setupComposite(
		basePathName, compositePathName, {"prefab"}, [this]() {
			auto prefab = findExt<Prefab>("prefab");
			auto inst = create_prefabInstance("inst", prefab);
		},
		"composite", 1);

	updateBase(basePathName, [this]() {
		EXPECT_EQ(project->featureLevel(), 1);
		auto prefab = find("prefab");
		auto node = create<Node>("node", prefab);
	});

	updateComposite(
		compositePathName, [this]() {
			EXPECT_EQ(project->featureLevel(), 2);
		},
		2);
}

TEST_F(ExtrefTest, feature_level_upgrade_base) {
	auto basePathName{(test_path() / "base.rca").string()};
	auto compositePathName{(test_path() / "composite.rca").string()};

	setupBase(
		basePathName, [this]() {
			auto prefab = create<Prefab>("prefab");
		},
		"base", 1);

	setupComposite(
		basePathName, compositePathName, {"prefab"}, [this]() {
			auto prefab = findExt<Prefab>("prefab");
			auto inst = create_prefabInstance("inst", prefab);
		},
		"composite", 1);

	updateBase(
		basePathName, [this]() {}, 2);

	EXPECT_THROW(updateComposite(
					 compositePathName, [this]() {}, 1),
		raco::core::ExtrefError);

	updateComposite(
		compositePathName, [this]() {
			EXPECT_EQ(project->featureLevel(), 2);
		},
		2);
}

TEST_F(ExtrefTest, extref_paste_from_orig_and_save_as_with_new_id_copy) {
	auto basePathName1{(test_path() / "base1.rca").string()};
	auto basePathName2{(test_path() / "base2.rca").string()};

	setupBase(
		basePathName1, [this]() {
			auto mesh = create<Mesh>("mesh");
		},
		std::string("base"));

	updateComposite(basePathName1, [this, basePathName2]() {
		std::string error;
		app->saveAsWithNewIDs(QString::fromStdString(basePathName2), error);
	});

	setupGeneric([this, basePathName1, basePathName2]() {
		ASSERT_TRUE(pasteFromExt(basePathName1, {"mesh"}, true));
		ASSERT_TRUE(pasteFromExt(basePathName2, {"mesh"}, true));
	});
}

TEST_F(ExtrefTest, save_as_with_new_id_preserves_extref_id) {
	auto basePathName{(test_path() / "base.rca").string()};
	auto compositePathName{(test_path() / "composite.rca").string()};
	auto compositePathName2{(test_path() / "composite2.rca").string()};

	setupBase(basePathName, [this]() {
		auto prefab = create<Prefab>("prefab");
	});

	std::string prefabID;
	std::string instID;
	setupComposite(basePathName, compositePathName, {"prefab"}, [this, &prefabID, &instID]() {
		auto prefab = findExt<Prefab>("prefab");
		auto inst = create_prefabInstance("inst", prefab);
		prefabID = prefab->objectID();
		instID = prefab->objectID();
	});

	updateComposite(compositePathName, [this, compositePathName2, prefabID, instID]() {
		std::string error;
		app->saveAsWithNewIDs(QString::fromStdString(compositePathName2), error);
		project = app->activeRaCoProject().project();
		cmd = app->activeRaCoProject().commandInterface();

		auto prefab = findExt<Prefab>("prefab");
		auto inst = find("inst");
		EXPECT_EQ(prefabID, prefab->objectID());
		EXPECT_NE(instID, inst->objectID());
	});
}

TEST_F(ExtrefTest, save_as_with_new_id_preserves_prefabinst_local_properties) {
	auto basePathName{(test_path() / "base.rca").string()};
	auto compositePathName{(test_path() / "composite.rca").string()};
	auto compositePathName2{(test_path() / "composite2.rca").string()};

	setupBase(basePathName, [this]() {
		auto prefab = create<Prefab>("prefab");
		TextFile scriptFile = makeFile("interface.lua", R"(
function interface(INOUT)
	INOUT.u = Type:Float()
	INOUT.v = Type:Float()
end
)");
		auto interface = create<LuaInterface>("interface", prefab);
		cmd->set({interface, {"uri"}}, scriptFile);
		cmd->set({interface, {"inputs", "u"}}, 1.0);
	});

	setupComposite(basePathName, compositePathName, {"prefab"}, [this]() {
		auto prefab = findExt<Prefab>("prefab");
		auto inst = create_prefabInstance("inst", prefab);

		auto prefab_intf  = prefab->children_->asVector<SEditorObject>()[0]->as<LuaInterface>();
		auto inst_intf = inst->children_->asVector<SEditorObject>()[0]->as<LuaInterface>();
		cmd->set({inst_intf, {"inputs", "u"}}, 0.0);
		cmd->set({inst_intf, {"inputs", "v"}}, 2.0);

		EXPECT_EQ(prefab_intf->inputs_->get("u")->asDouble(), 1.0);
		EXPECT_EQ(prefab_intf->inputs_->get("v")->asDouble(), 0.0);
		EXPECT_EQ(inst_intf->inputs_->get("u")->asDouble(), 0.0);
		EXPECT_EQ(inst_intf->inputs_->get("v")->asDouble(), 2.0);
	});

	updateComposite(compositePathName, [this, compositePathName2]() {
		std::string error;
		app->saveAsWithNewIDs(QString::fromStdString(compositePathName2), error);
		project = app->activeRaCoProject().project();
		cmd = app->activeRaCoProject().commandInterface();

		auto prefab = findExt<Prefab>("prefab");
		auto inst = find("inst");

		auto prefab_intf = prefab->children_->asVector<SEditorObject>()[0]->as<LuaInterface>();
		auto inst_intf = inst->children_->asVector<SEditorObject>()[0]->as<LuaInterface>();
		EXPECT_EQ(prefab_intf->inputs_->get("u")->asDouble(), 1.0);
		EXPECT_EQ(prefab_intf->inputs_->get("v")->asDouble(), 0.0);
		EXPECT_EQ(inst_intf->inputs_->get("u")->asDouble(), 0.0);
		EXPECT_EQ(inst_intf->inputs_->get("v")->asDouble(), 2.0);
	});
}

TEST_F(ExtrefTest, save_as_with_new_id_preserves_prefabinst_local_links) {
	auto basePathName{(test_path() / "base.rca").string()};
	auto compositePathName{(test_path() / "composite.rca").string()};
	auto compositePathName2{(test_path() / "composite2.rca").string()};

	TextFile scriptFile = makeFile("interface.lua", R"(
function interface(INOUT)
	INOUT.u = Type:Float()
end
)");

	setupBase(basePathName, [this, &scriptFile]() {
		auto prefab = create<Prefab>("prefab");
		auto interface = create<LuaInterface>("interface", prefab);
		cmd->set({interface, {"uri"}}, scriptFile);
	});

	setupComposite(basePathName, compositePathName, {"prefab"}, [this, &scriptFile]() {
		auto prefab = findExt<Prefab>("prefab");
		auto inst = create_prefabInstance("inst", prefab);
		auto global_interface = create<LuaInterface>("global_interface", prefab);
		cmd->set({global_interface, {"uri"}}, scriptFile);

		auto inst_intf = inst->children_->asVector<SEditorObject>()[0]->as<LuaInterface>();
		cmd->addLink({global_interface, {"inputs", "u"}}, {inst_intf, {"inputs", "u"}});

		checkLinks({{{global_interface, {"inputs", "u"}}, {inst_intf, {"inputs", "u"}}}});
	});

	updateComposite(compositePathName, [this, compositePathName2]() {
		std::string error;
		app->saveAsWithNewIDs(QString::fromStdString(compositePathName2), error);
		project = app->activeRaCoProject().project();
		cmd = app->activeRaCoProject().commandInterface();

		auto inst = find("inst");
		auto global_interface = find("global_interface");
		auto inst_intf = inst->children_->asVector<SEditorObject>()[0]->as<LuaInterface>();

		checkLinks({{{global_interface, {"inputs", "u"}}, {inst_intf, {"inputs", "u"}}}});
	});
}
