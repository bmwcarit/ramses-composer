/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "testing/RaCoApplicationTest.h"

#include "application/RaCoApplication.h"
#include "application/RaCoProject.h"
#include "components/RaCoNameConstants.h"
#include "components/RaCoPreferences.h"
#include "core/PathManager.h"
#include "core/Queries.h"
#include "ramses_adaptor/SceneBackend.h"
#include "ramses_base/BaseEngineBackend.h"
#include "spdlog/sinks/base_sink.h"
#include "testing/TestUtil.h"
#include "user_types/LuaScript.h"
#include "user_types/LuaScriptModule.h"
#include "user_types/Material.h"
#include "user_types/Mesh.h"
#include "user_types/MeshNode.h"
#include "user_types/Node.h"
#include "utils/u8path.h"

class RaCoProjectFixture : public RaCoApplicationTest {};

using namespace raco::core;
using namespace raco::user_types;

TEST_F(RaCoProjectFixture, saveLoadWithLink) {
	raco::createLinkedScene(commandInterface(), test_path());
	std::string msg;
	ASSERT_TRUE(application.activeRaCoProject().saveAs((test_path() / "project.rca").string().c_str(), msg));

	application.switchActiveRaCoProject(QString::fromStdString((test_path() / "project.rca").string()), {});
	ASSERT_EQ(1, application.activeRaCoProject().project()->links().size());
}

TEST_F(RaCoProjectFixture, saveLoadWithBrokenLink) {
	auto linkedScene = raco::createLinkedScene(commandInterface(), test_path());
	raco::utils::file::write((test_path() / "lua_script.lua").string(), R"(
function interface(IN,OUT)
	OUT.newTranslation = Type:Vec3f()
end
function run(IN,OUT)
end
	)");
	std::string msg;
	ASSERT_TRUE(application.activeRaCoProject().saveAs((test_path() / "project.rca").string().c_str(), msg));

	application.switchActiveRaCoProject(QString::fromStdString((test_path() / "project.rca").string()), {});

	ASSERT_EQ(1, application.activeRaCoProject().project()->links().size());
	ASSERT_FALSE((*application.activeRaCoProject().project()->links().begin())->isValid());
	auto node = raco::core::Queries::findByName(application.activeRaCoProject().project()->instances(), "node");
	ASSERT_TRUE(application.activeRaCoProject().errors()->hasError(node));
}

TEST_F(RaCoProjectFixture, saveLoadWithBrokenAndValidLink) {
	{
		const auto luaScript{application.activeRaCoProject().commandInterface()->createObject(raco::user_types::LuaScript::typeDescription.typeName, "lua_script")};
		const auto node{application.activeRaCoProject().commandInterface()->createObject(raco::user_types::Node::typeDescription.typeName, "node")};
		raco::utils::file::write((test_path() / "lua_script.lua").string(), R"(
function interface(IN,OUT)
	OUT.translation = Type:Vec3f()
	OUT.rotation = Type:Vec3f()
end
function run(IN,OUT)
end
	)");
		application.activeRaCoProject().commandInterface()->set({luaScript, {"uri"}}, (test_path() / "lua_script.lua").string());
		application.activeRaCoProject().commandInterface()->addLink({luaScript, {"outputs", "translation"}}, {node, {"translation"}});
		application.activeRaCoProject().commandInterface()->addLink({luaScript, {"outputs", "rotation"}}, {node, {"rotation"}});

		checkLinks(*application.activeRaCoProject().project(),
			{{{luaScript, {"outputs", "translation"}}, {node, {"translation"}}, true},
				{{luaScript, {"outputs", "rotation"}}, {node, {"rotation"}}, true}});

		std::string msg;
		ASSERT_TRUE(application.activeRaCoProject().saveAs((test_path() / "project.rca").string().c_str(), msg));
	}

	raco::utils::file::write((test_path() / "lua_script.lua").string(), R"(
function interface(IN,OUT)
	OUT.newTranslation = Type:Vec3f()
	OUT.rotation = Type:Vec3f()
end
function run(IN,OUT)
end
	)");

	{
		application.switchActiveRaCoProject(QString::fromStdString((test_path() / "project.rca").string()), {});
		auto node = raco::core::Queries::findByName(application.activeRaCoProject().project()->instances(), "node");
		auto luaScript = raco::core::Queries::findByName(application.activeRaCoProject().project()->instances(), "lua_script");

		checkLinks(*application.activeRaCoProject().project(),
			{{{luaScript, {"outputs", "translation"}}, {node, {"translation"}}, false},
				{{luaScript, {"outputs", "rotation"}}, {node, {"rotation"}}, true}});

		ASSERT_TRUE(application.activeRaCoProject().errors()->hasError(node));
	}

	raco::utils::file::write((test_path() / "lua_script.lua").string(), R"(
function interface(IN,OUT)
	OUT.translation = Type:Vec3f()
	OUT.rotation = Type:Vec3f()
end
function run(IN,OUT)
end
	)");

	{
		application.switchActiveRaCoProject(QString::fromStdString((test_path() / "project.rca").string()), {});
		auto node = raco::core::Queries::findByName(application.activeRaCoProject().project()->instances(), "node");
		auto luaScript = raco::core::Queries::findByName(application.activeRaCoProject().project()->instances(), "lua_script");

		checkLinks(*application.activeRaCoProject().project(),
			{{{luaScript, {"outputs", "translation"}}, {node, {"translation"}}, true},
				{{luaScript, {"outputs", "rotation"}}, {node, {"rotation"}}, true}});

		ASSERT_FALSE(application.activeRaCoProject().errors()->hasError(node));
	}
}

TEST_F(RaCoProjectFixture, saveWithValidLinkLoadWithBrokenLink) {
	{
		auto linkedScene = raco::createLinkedScene(*application.activeRaCoProject().commandInterface(), test_path());
		std::string msg;
		ASSERT_TRUE(application.activeRaCoProject().saveAs((test_path() / "project.rca").string().c_str(), msg));
	}
	raco::utils::file::write((test_path() / "lua_script.lua").string(), R"(
function interface(IN,OUT)
	OUT.newTranslation = Type:Vec3f()
end
function run(IN,OUT)
end
	)");

	{
		application.switchActiveRaCoProject(QString::fromStdString((test_path() / "project.rca").string()), {});
		ASSERT_EQ(1, application.activeRaCoProject().project()->links().size());
		ASSERT_FALSE((*application.activeRaCoProject().project()->links().begin())->isValid());
		auto node = raco::core::Queries::findByName(application.activeRaCoProject().project()->instances(), "node");
		ASSERT_TRUE(application.activeRaCoProject().errors()->hasError(node));
	}
}

TEST_F(RaCoProjectFixture, saveWithBrokenLinkLoadWithValidLink) {
	{
		auto linkedScene = raco::createLinkedScene(*application.activeRaCoProject().commandInterface(), test_path());
		raco::utils::file::write((test_path() / "lua_script.lua").string(), R"(
function interface(IN,OUT)
	OUT.newTranslation = Type:Vec3f()
end
function run(IN,OUT)
end
	)");
		std::string msg;
		ASSERT_TRUE(application.activeRaCoProject().saveAs((test_path() / "project.rca").string().c_str(), msg));
	}

	raco::utils::file::write((test_path() / "lua_script.lua").string(), R"(
function interface(IN,OUT)
	OUT.translation = Type:Vec3f()
end
function run(IN,OUT)
end
	)");

	{
		application.switchActiveRaCoProject(QString::fromStdString((test_path() / "project.rca").string()), {});
		ASSERT_EQ(1, application.activeRaCoProject().project()->links().size());
		ASSERT_TRUE((*application.activeRaCoProject().project()->links().begin())->isValid());
	}
}

TEST_F(RaCoProjectFixture, saveLoadWithLinkRemoveOutputPropertyBeforeLoading) {
	{
		raco::createLinkedScene(*application.activeRaCoProject().commandInterface(), test_path());
		std::string msg;
		ASSERT_TRUE(application.activeRaCoProject().saveAs((test_path() / "project.rca").string().c_str(), msg));
	}

	// replace OUT.translation with OUT.scale in external script file
	raco::utils::file::write((test_path() / "lua_script.lua").string(), R"(
function interface(IN,OUT)
	OUT.scale = Type:Vec3f()
end
function run(IN,OUT)
end
	)");

	{
		application.switchActiveRaCoProject(QString::fromStdString((test_path() / "project.rca").string()), {});
		application.doOneLoop();
		ASSERT_EQ(application.activeRaCoProject().project()->links().size(), 1);
		ASSERT_FALSE((*application.activeRaCoProject().project()->links().begin())->isValid());
	}
}

TEST_F(RaCoProjectFixture, saveLoadWithLuaScriptNewOutputPropertyGetsCalculated) {
	auto luaScriptPath = (test_path() / "lua_script.lua").string();

	raco::utils::file::write(luaScriptPath, R"(
function interface(IN,OUT)
	IN.integer = Type:Int32()
	OUT.integer = Type:Int32()
end


function run(IN,OUT)
	OUT.integer = IN.integer
end

	)");

	{
		const auto luaScript{application.activeRaCoProject().commandInterface()->createObject(raco::user_types::LuaScript::typeDescription.typeName, "lua_script")};
		application.activeRaCoProject().commandInterface()->set(raco::core::ValueHandle{luaScript, {"uri"}}, luaScriptPath);
		application.activeRaCoProject().commandInterface()->set(raco::core::ValueHandle{luaScript, {"inputs", "integer"}}, 5);
		std::string msg;
		ASSERT_TRUE(application.activeRaCoProject().saveAs((test_path() / "project.rca").string().c_str(), msg));
	}

	raco::utils::file::write((test_path() / "lua_script.lua").string(), R"(
function interface(IN,OUT)
	IN.integer = Type:Int32()
	OUT.integerTwo = Type:Int32()
end


function run(IN,OUT)
	OUT.integerTwo = IN.integer
end

	)");
	{
		application.switchActiveRaCoProject(QString::fromStdString((test_path() / "project.rca").string()), {});
		application.doOneLoop();
		auto luaScript = raco::core::Queries::findByName(application.activeRaCoProject().project()->instances(), "lua_script");
		auto newPropertyOutput = raco::core::ValueHandle{luaScript, {"outputs", "integerTwo"}}.asInt();
		ASSERT_EQ(newPropertyOutput, 5);
	}
}

TEST_F(RaCoProjectFixture, saveAsMeshRerootRelativeURIHierarchyDown) {
	application.activeRaCoProject().project()->setCurrentPath((test_path() / "project.rca").string());
	auto mesh = application.activeRaCoProject().commandInterface()->createObject(raco::user_types::Mesh::typeDescription.typeName, "mesh");

	std::string relativeUri{"Duck.glb"};
	application.activeRaCoProject().commandInterface()->set({mesh, {"uri"}}, relativeUri);

	QDir().mkdir((test_path() / "project").string().c_str());
	std::string msg;
	ASSERT_TRUE(application.activeRaCoProject().saveAs((test_path() / "project" / "project.rca").string().c_str(), msg));
	std::string newRelativeDuckPath{"../" + relativeUri};

	ASSERT_EQ(application.activeRaCoProject().project()->instances().back()->get("uri")->asString(), newRelativeDuckPath);
}

TEST_F(RaCoProjectFixture, saveAsThenLoadAnimationKeepsChannelAmount) {
	auto* commandInterface = application.activeRaCoProject().commandInterface();

	raco::core::MeshDescriptor desc;
	desc.absPath = test_path().append("meshes/CesiumMilkTruck/CesiumMilkTruck.gltf").string();
	desc.bakeAllSubmeshes = false;

	auto [scenegraph, dummyCacheEntry] = raco::getMeshSceneGraphWithHandler(commandInterface->meshCache(), desc);

	commandInterface->insertAssetScenegraph(scenegraph, desc.absPath, nullptr);
	commandInterface->createObject(raco::user_types::Animation::typeDescription.typeName, "userAnim");

	std::string msg;
	ASSERT_TRUE(application.activeRaCoProject().saveAs((test_path() / "anims.rca").string().c_str(), msg));
	application.switchActiveRaCoProject("", {});
	application.switchActiveRaCoProject(QString::fromStdString((test_path() / "anims.rca").string()), {});

	auto anims = raco::core::Queries::filterByTypeName(application.activeRaCoProject().project()->instances(), {raco::user_types::Animation::typeDescription.typeName});
	auto importedAnim = raco::core::Queries::findByName(anims, "Wheels");
	auto userAnim = raco::core::Queries::findByName(anims, "userAnim");

	ASSERT_EQ(userAnim->as<raco::user_types::Animation>()->animationChannels_.asTable().size(), raco::user_types::Animation::ANIMATION_CHANNEL_AMOUNT);
	ASSERT_EQ(importedAnim->as<raco::user_types::Animation>()->animationChannels_.asTable().size(), 2);
}

TEST_F(RaCoProjectFixture, saveAsMeshRerootRelativeURIHierarchyUp) {
	application.activeRaCoProject().project()->setCurrentPath((test_path() / "project" / "project.rca").string());
	auto mesh = application.activeRaCoProject().commandInterface()->createObject(raco::user_types::Mesh::typeDescription.typeName, "mesh");

	std::string relativeUri{"Duck.glb"};
	application.activeRaCoProject().commandInterface()->set({mesh, {"uri"}}, relativeUri);

	std::string msg;
	ASSERT_TRUE(application.activeRaCoProject().saveAs((test_path() / "project.rca").string().c_str(), msg));
	std::string newRelativeDuckPath{"project/" + relativeUri};

	ASSERT_EQ(application.activeRaCoProject().project()->instances().back()->get("uri")->asString(), newRelativeDuckPath);
}

TEST_F(RaCoProjectFixture, saveAsMaterialRerootRelativeURI) {
	application.activeRaCoProject().project()->setCurrentPath((test_path() / "project.rca").string());
	auto mat = application.activeRaCoProject().commandInterface()->createObject(raco::user_types::Material::typeDescription.typeName, "material");

	std::string relativeUri{"relativeURI"};
	application.activeRaCoProject().commandInterface()->set({mat, {"uriVertex"}}, relativeUri);
	application.activeRaCoProject().commandInterface()->set({mat, {"uriFragment"}}, relativeUri);

	QDir().mkdir((test_path() / "project").string().c_str());
	std::string msg;
	ASSERT_TRUE(application.activeRaCoProject().saveAs((test_path() / "project" / "project.rca").string().c_str(), msg));
	std::string newRelativePath{"../" + relativeUri};

	ASSERT_EQ(application.activeRaCoProject().project()->instances().back()->get("uriVertex")->asString(), newRelativePath);
	ASSERT_EQ(application.activeRaCoProject().project()->instances().back()->get("uriFragment")->asString(), newRelativePath);
}

TEST_F(RaCoProjectFixture, saveAsLuaScriptRerootRelativeURI) {
	application.activeRaCoProject().project()->setCurrentPath((test_path() / "project.rca").string());
	auto lua = application.activeRaCoProject().commandInterface()->createObject(raco::user_types::LuaScript::typeDescription.typeName, "lua");

	std::string relativeUri{"relativeURI"};
	application.activeRaCoProject().commandInterface()->set({lua, {"uri"}}, relativeUri);

	QDir().mkdir((test_path() / "project").string().c_str());
	std::string msg;
	ASSERT_TRUE(application.activeRaCoProject().saveAs((test_path() / "project" / "project.rca").string().c_str(), msg));
	std::string newRelativePath{"../" + relativeUri};

	ASSERT_EQ(application.activeRaCoProject().project()->instances().back()->get("uri")->asString(), newRelativePath);
}

TEST_F(RaCoProjectFixture, saveAsSimulateSavingFromNewProjectCorrectlyRerootedRelativeURI) {
	std::filesystem::create_directory(test_path() / "project");
	application.activeRaCoProject().project()->setCurrentPath((test_path() / "project").string());
	auto mesh = application.activeRaCoProject().commandInterface()->createObject(raco::user_types::Mesh::typeDescription.typeName, "lua");

	std::string relativeUri{"relativeURI.ctm"};
	application.activeRaCoProject().commandInterface()->set({mesh, {"uri"}}, relativeUri);

	QDir().mkdir((test_path() / "project").string().c_str());
	std::string msg;
	ASSERT_TRUE(application.activeRaCoProject().saveAs((test_path() / "project" / "project.rca").string().c_str(), msg));

	ASSERT_EQ(raco::core::ValueHandle(mesh, {"uri"}).asString(), relativeUri);
}

#if defined(_WIN32)
TEST_F(RaCoProjectFixture, saveAsToDifferentDriveSetsRelativeURIsToAbsolute) {
	std::filesystem::create_directory(test_path() / "project");
	application.activeRaCoProject().project()->setCurrentPath((test_path() / "project").string());
	auto mesh = application.activeRaCoProject().commandInterface()->createObject(raco::user_types::Mesh::typeDescription.typeName, "lua");

	std::string relativeUri{"relativeURI.ctm"};
	application.activeRaCoProject().commandInterface()->set({mesh, {"uri"}}, relativeUri);

	// Usually, an ASSERT_TRUE could be put around saving of RaCo projects,
	// here saving this will fail if the drive Z: does not exist.
	std::string msg;
	application.activeRaCoProject().saveAs("Z:/projectOnDifferentDrive.rca", msg);

	ASSERT_EQ(raco::core::ValueHandle(mesh, {"uri"}).asString(), (test_path() / "project" / relativeUri).string());
}
#endif

TEST_F(RaCoProjectFixture, saveAsWithNewID) {
	std::string msg;
	const auto objectInitID = application.activeRaCoProject().project()->settings()->objectID();
	ASSERT_TRUE(application.saveAsWithNewIDs((test_path() / "project.rca").string().c_str(), msg, false));
	const auto objectNewID = application.activeRaCoProject().project()->settings()->objectID();
	ASSERT_NE(objectInitID, objectNewID);

	application.switchActiveRaCoProject(QString::fromStdString((test_path() / "project.rca").string()), {});
	ASSERT_EQ(application.activeRaCoProject().project()->settings()->objectID(), objectNewID);
}

TEST_F(RaCoProjectFixture, saveAsWithNewIDSamePath) {
	std::string msg;
	const auto objectInitID = application.activeRaCoProject().project()->settings()->objectID();
	ASSERT_TRUE(application.saveAsWithNewIDs((test_path() / "project.rca").string().c_str(), msg, false));
	const auto objectNewID1 = application.activeRaCoProject().project()->settings()->objectID();
	ASSERT_NE(objectNewID1, objectInitID);
	ASSERT_TRUE(application.saveAsWithNewIDs((test_path() / "project.rca").string().c_str(), msg, false));
	const auto objectNewID2 = application.activeRaCoProject().project()->settings()->objectID();
	ASSERT_NE(objectNewID1, objectNewID2);
}

TEST_F(RaCoProjectFixture, save_as_with_new_id_preserves_prefab_id_structure_nested) {
	auto& cmd = *application.activeRaCoProject().commandInterface();

	//           prefab_2  inst_2
	//  prefab   inst_1    inst_3
	//  lua      lua_1     lua_2

	application.activeRaCoProject().project()->setCurrentPath((test_path() / "project.rca").string());

	auto prefab = cmd.createObject(raco::user_types::Prefab::typeDescription.typeName, "prefab");
	auto lua = cmd.createObject(raco::user_types::LuaScript::typeDescription.typeName, "lua");
	cmd.moveScenegraphChildren({lua}, prefab);

	auto prefab_2 = cmd.createObject(raco::user_types::Prefab::typeDescription.typeName, "prefab2");
	auto inst_1 = cmd.createObject(raco::user_types::PrefabInstance::typeDescription.typeName, "inst");
	cmd.moveScenegraphChildren({inst_1}, prefab_2);
	cmd.set({inst_1, &raco::user_types::PrefabInstance::template_}, prefab);

	EXPECT_EQ(inst_1->children_->size(), 1);
	auto lua_1 = inst_1->children_->asVector<SEditorObject>()[0];

	auto inst_2 = cmd.createObject(raco::user_types::PrefabInstance::typeDescription.typeName, "inst2");
	cmd.set({inst_2, &raco::user_types::PrefabInstance::template_}, prefab_2);
	EXPECT_EQ(inst_2->children_->size(), 1);
	auto inst_3 = inst_2->children_->asVector<SEditorObject>()[0];
	EXPECT_EQ(inst_3->children_->size(), 1);
	auto lua_2 = inst_3->children_->asVector<SEditorObject>()[0];

	EXPECT_EQ(lua_1->objectID(), EditorObject::XorObjectIDs(lua->objectID(), inst_1->objectID()));
	EXPECT_EQ(inst_3->objectID(), EditorObject::XorObjectIDs(inst_1->objectID(), inst_2->objectID()));
	EXPECT_EQ(lua_2->objectID(), EditorObject::XorObjectIDs(lua_1->objectID(), inst_2->objectID()));
	EXPECT_EQ(lua_2->objectID(), EditorObject::XorObjectIDs(lua->objectID(), inst_3->objectID()));

	std::string prefab_id = prefab->objectID();
	std::string prefab_2_id = prefab_2->objectID();
	std::string inst_1_id = inst_1->objectID();
	std::string inst_2_id = inst_2->objectID();
	std::string inst_3_id = inst_3->objectID();
	std::string lua_id = lua->objectID();
	std::string lua_1_id = lua_1->objectID();
	std::string lua_2_id = lua_2->objectID();

	std::string msg;
	ASSERT_TRUE(application.saveAsWithNewIDs((test_path() / "project.rca").string().c_str(), msg, false));
	auto& instances = application.activeRaCoProject().project()->instances();

	{
		auto prefab = raco::select<Prefab>(instances, "prefab");
		auto lua = prefab->children_->asVector<SEditorObject>()[0];

		auto prefab_2 = raco::select<Prefab>(instances, "prefab2");
		auto inst_1 = prefab_2->children_->asVector<SEditorObject>()[0];
		auto lua_1 = inst_1->children_->asVector<SEditorObject>()[0];

		auto inst_2 = raco::select<PrefabInstance>(instances, "inst2");
		auto inst_3 = inst_2->children_->asVector<SEditorObject>()[0];
		auto lua_2 = inst_3->children_->asVector<SEditorObject>()[0];

		EXPECT_NE(prefab_id, prefab->objectID());
		EXPECT_NE(prefab_2_id, prefab_2->objectID());

		EXPECT_NE(inst_1_id, inst_1->objectID());
		EXPECT_NE(inst_2_id, inst_2->objectID());
		EXPECT_NE(inst_3_id, inst_3->objectID());

		EXPECT_NE(lua_id, lua->objectID());
		EXPECT_NE(lua_1_id, lua_1->objectID());
		EXPECT_NE(lua_2_id, lua_2->objectID());

		EXPECT_EQ(lua_1->objectID(), EditorObject::XorObjectIDs(lua->objectID(), inst_1->objectID()));
		EXPECT_EQ(inst_3->objectID(), EditorObject::XorObjectIDs(inst_1->objectID(), inst_2->objectID()));
		EXPECT_EQ(lua_2->objectID(), EditorObject::XorObjectIDs(lua_1->objectID(), inst_2->objectID()));
		EXPECT_EQ(lua_2->objectID(), EditorObject::XorObjectIDs(lua->objectID(), inst_3->objectID()));
	}
}

TEST_F(RaCoProjectFixture, saveAsSetProjectName) {
	std::string msg;
	ASSERT_TRUE(application.activeRaCoProject().project()->settings()->objectName().empty());
	ASSERT_TRUE(application.activeRaCoProject().saveAs((test_path() / "project.rca").string().c_str(), msg, true));
	ASSERT_EQ("project", application.activeRaCoProject().project()->settings()->objectName());

	application.switchActiveRaCoProject(QString::fromStdString((test_path() / "project.rca").string()), {});
	ASSERT_EQ("project", application.activeRaCoProject().project()->settings()->objectName());
}

TEST_F(RaCoProjectFixture, idChange) {
	application.activeRaCoProject().project()->setCurrentPath((test_path() / "project.rca").string());
	application.doOneLoop();
	ASSERT_EQ(123u, application.sceneBackend()->currentSceneIdValue());

	application.activeRaCoProject().commandInterface()->set({application.activeRaCoProject().project()->settings(), {"sceneId"}}, 1024);
	application.doOneLoop();
	ASSERT_EQ(1024u, application.sceneBackend()->currentSceneIdValue());
}

TEST_F(RaCoProjectFixture, restoredLinkWorksInLogicEngine) {
	auto start{application.activeRaCoProject().commandInterface()->createObject(raco::user_types::LuaScript::typeDescription.typeName, "start")};
	application.activeRaCoProject().commandInterface()->set({start, {"uri"}}, test_path().append("scripts/SimpleScript.lua").string());
	application.doOneLoop();

	auto end{application.activeRaCoProject().commandInterface()->createObject(raco::user_types::LuaScript::typeDescription.typeName, "end")};
	application.activeRaCoProject().commandInterface()->set({end, {"uri"}}, test_path().append("scripts/SimpleScript.lua").string());
	application.doOneLoop();

	application.activeRaCoProject().commandInterface()->addLink({start, {"outputs", "out_float"}}, {end, {"inputs", "in_float"}});
	application.doOneLoop();

	application.activeRaCoProject().commandInterface()->set({end, {"uri"}}, std::string());
	application.doOneLoop();
	ASSERT_EQ(application.activeRaCoProject().project()->links().size(), 1);
	ASSERT_FALSE((*application.activeRaCoProject().project()->links().begin())->isValid());

	application.activeRaCoProject().commandInterface()->set({end, {"uri"}}, test_path().append("scripts/SimpleScript.lua").string());
	application.activeRaCoProject().commandInterface()->set({start, {"inputs", "in_float"}}, 3.0);
	application.doOneLoop();
	ASSERT_EQ(raco::core::ValueHandle(end, {"inputs", "in_float"}).asDouble(), 3.0);
}

TEST_F(RaCoProjectFixture, brokenLinkDoesNotResetProperties) {
	auto linkStart{application.activeRaCoProject().commandInterface()->createObject(raco::user_types::LuaScript::typeDescription.typeName, "start")};
	application.activeRaCoProject().commandInterface()->set({linkStart, {"uri"}}, test_path().append("scripts/types-scalar.lua").string());

	auto linkEnd{application.activeRaCoProject().commandInterface()->createObject(raco::user_types::LuaScript::typeDescription.typeName, "end")};
	application.activeRaCoProject().commandInterface()->set({linkEnd, {"uri"}}, test_path().append("scripts/types-scalar.lua").string());
	application.doOneLoop();

	application.activeRaCoProject().commandInterface()->addLink({linkStart, {"outputs", "ointeger"}}, {linkEnd, {"inputs", "integer"}});
	application.doOneLoop();

	application.activeRaCoProject().commandInterface()->set({linkStart, {"inputs", "integer"}}, 10);
	application.doOneLoop();

	application.activeRaCoProject().commandInterface()->set({linkStart, {"uri"}}, std::string());
	application.doOneLoop();

	application.activeRaCoProject().commandInterface()->set({linkEnd, {"inputs", "float"}}, 20.0);
	application.doOneLoop();

	auto output_int = raco::core::ValueHandle{linkEnd, {"outputs", "ointeger"}}.asInt();
	auto output_bool = raco::core::ValueHandle{linkEnd, {"outputs", "flag"}}.asBool();
	ASSERT_EQ(output_int, 40);
	ASSERT_EQ(output_bool, true);
}

TEST_F(RaCoProjectFixture, launchApplicationWithNoResourceSubFoldersCachedPathsAreSetToUserProjectsDirectoryAndSubFolders) {
	auto newProjectFolder = test_path() / "newProject";
	std::filesystem::create_directory(newProjectFolder);
	raco::components::RaCoPreferences::instance().userProjectsDirectory = QString::fromStdString(newProjectFolder.string());
	application.switchActiveRaCoProject({}, {});

	const auto& defaultResourceDirectories = application.activeRaCoProject().project()->settings()->defaultResourceDirectories_;
	auto imageSubdirectory = defaultResourceDirectories->imageSubdirectory_.asString();
	auto meshSubdirectory = defaultResourceDirectories->meshSubdirectory_.asString();
	auto scriptSubdirectory = defaultResourceDirectories->scriptSubdirectory_.asString();
	auto interfaceSubdirectory = defaultResourceDirectories->interfaceSubdirectory_.asString();
	auto shaderSubdirectory = defaultResourceDirectories->shaderSubdirectory_.asString();

	ASSERT_EQ(raco::core::PathManager::getCachedPath(raco::core::PathManager::FolderTypeKeys::Project), newProjectFolder);
	ASSERT_EQ(raco::core::PathManager::getCachedPath(raco::core::PathManager::FolderTypeKeys::Mesh), newProjectFolder / meshSubdirectory);
	ASSERT_EQ(raco::core::PathManager::getCachedPath(raco::core::PathManager::FolderTypeKeys::Image), newProjectFolder / imageSubdirectory);
	ASSERT_EQ(raco::core::PathManager::getCachedPath(raco::core::PathManager::FolderTypeKeys::Script), newProjectFolder / scriptSubdirectory);
	ASSERT_EQ(raco::core::PathManager::getCachedPath(raco::core::PathManager::FolderTypeKeys::Interface), newProjectFolder / interfaceSubdirectory);
	ASSERT_EQ(raco::core::PathManager::getCachedPath(raco::core::PathManager::FolderTypeKeys::Shader), newProjectFolder / shaderSubdirectory);
}

TEST_F(RaCoProjectFixture, launchApplicationWithResourceSubFoldersCachedPathsAreSetToUserProjectsDirectoryAndSubFolders) {
	raco::components::RaCoPreferences::instance().userProjectsDirectory = QString::fromStdString(test_path().string());
	application.switchActiveRaCoProject({}, {});

	const auto& prefs = raco::components::RaCoPreferences::instance();
	std::filesystem::create_directory(test_path() / prefs.meshSubdirectory.toStdString());
	std::filesystem::create_directory(test_path() / prefs.scriptSubdirectory.toStdString());
	std::filesystem::create_directory(test_path() / prefs.interfaceSubdirectory.toStdString());
	std::filesystem::create_directory(test_path() / prefs.imageSubdirectory.toStdString());
	std::filesystem::create_directory(test_path() / prefs.shaderSubdirectory.toStdString());

	auto newProjectFolder = raco::components::RaCoPreferences::instance().userProjectsDirectory.toStdString();

	const auto& defaultResourceDirectories = application.activeRaCoProject().project()->settings()->defaultResourceDirectories_;
	auto imageSubdirectory = defaultResourceDirectories->imageSubdirectory_.asString();
	auto meshSubdirectory = defaultResourceDirectories->meshSubdirectory_.asString();
	auto scriptSubdirectory = defaultResourceDirectories->scriptSubdirectory_.asString();
	auto interfaceSubdirectory = defaultResourceDirectories->interfaceSubdirectory_.asString();
	auto shaderSubdirectory = defaultResourceDirectories->shaderSubdirectory_.asString();

	ASSERT_EQ(raco::core::PathManager::getCachedPath(raco::core::PathManager::FolderTypeKeys::Project), newProjectFolder);
	ASSERT_EQ(raco::core::PathManager::getCachedPath(raco::core::PathManager::FolderTypeKeys::Mesh), newProjectFolder + "/" + meshSubdirectory);
	ASSERT_EQ(raco::core::PathManager::getCachedPath(raco::core::PathManager::FolderTypeKeys::Script), newProjectFolder + "/" + scriptSubdirectory);
	ASSERT_EQ(raco::core::PathManager::getCachedPath(raco::core::PathManager::FolderTypeKeys::Interface), newProjectFolder + "/" + interfaceSubdirectory);
	ASSERT_EQ(raco::core::PathManager::getCachedPath(raco::core::PathManager::FolderTypeKeys::Image), newProjectFolder + "/" + imageSubdirectory);
	ASSERT_EQ(raco::core::PathManager::getCachedPath(raco::core::PathManager::FolderTypeKeys::Shader), newProjectFolder + "/" + shaderSubdirectory);
}

TEST_F(RaCoProjectFixture, saveAs_set_path_updates_cached_path) {
	std::string imageSubdirectory = u8"images abc";
	std::string meshSubdirectory = u8"meshes def";
	std::string scriptSubdirectory = u8"shared";
	std::string interfaceSubdirectory = u8"shared";
	std::string shaderSubdirectory = u8"shared";

	using namespace raco::user_types;

	std::string msg;
	ASSERT_TRUE(application.activeRaCoProject().saveAs((test_path() / "project.rca").string().c_str(), msg, false));

	const auto& settings = application.activeRaCoProject().project()->settings();
	const auto& commandInterface = application.activeRaCoProject().commandInterface();

	commandInterface->set({settings, &ProjectSettings::defaultResourceDirectories_, &DefaultResourceDirectories::imageSubdirectory_}, imageSubdirectory);
	commandInterface->set({settings, &ProjectSettings::defaultResourceDirectories_, &DefaultResourceDirectories::meshSubdirectory_}, meshSubdirectory);
	commandInterface->set({settings, &ProjectSettings::defaultResourceDirectories_, &DefaultResourceDirectories::scriptSubdirectory_}, scriptSubdirectory);
	commandInterface->set({settings, &ProjectSettings::defaultResourceDirectories_, &DefaultResourceDirectories::interfaceSubdirectory_}, interfaceSubdirectory);
	commandInterface->set({settings, &ProjectSettings::defaultResourceDirectories_, &DefaultResourceDirectories::shaderSubdirectory_}, shaderSubdirectory);
	application.doOneLoop();

	std::string newProjectFolder = application.activeRaCoProject().project()->currentFolder();

	ASSERT_EQ(raco::core::PathManager::getCachedPath(raco::core::PathManager::FolderTypeKeys::Mesh), newProjectFolder + "/" + meshSubdirectory);
	ASSERT_EQ(raco::core::PathManager::getCachedPath(raco::core::PathManager::FolderTypeKeys::Script), newProjectFolder + "/" + scriptSubdirectory);
	ASSERT_EQ(raco::core::PathManager::getCachedPath(raco::core::PathManager::FolderTypeKeys::Interface), newProjectFolder + "/" + interfaceSubdirectory);
	ASSERT_EQ(raco::core::PathManager::getCachedPath(raco::core::PathManager::FolderTypeKeys::Image), newProjectFolder + "/" + imageSubdirectory);
	ASSERT_EQ(raco::core::PathManager::getCachedPath(raco::core::PathManager::FolderTypeKeys::Shader), newProjectFolder + "/" + shaderSubdirectory);
}

TEST_F(RaCoProjectFixture, saveAs_with_new_id_set_path_updates_cached_path) {
	std::string imageSubdirectory = u8"images abc";
	std::string meshSubdirectory = u8"meshes def";
	std::string scriptSubdirectory = u8"shared";
	std::string interfaceSubdirectory = u8"shared";
	std::string shaderSubdirectory = u8"shared";

	using namespace raco::user_types;

	std::string msg;
	ASSERT_TRUE(application.saveAsWithNewIDs((test_path() / "project.rca").string().c_str(), msg, false));

	const auto& settings = application.activeRaCoProject().project()->settings();
	const auto& commandInterface = application.activeRaCoProject().commandInterface();

	commandInterface->set({settings, &ProjectSettings::defaultResourceDirectories_, &DefaultResourceDirectories::imageSubdirectory_}, imageSubdirectory);
	commandInterface->set({settings, &ProjectSettings::defaultResourceDirectories_, &DefaultResourceDirectories::meshSubdirectory_}, meshSubdirectory);
	commandInterface->set({settings, &ProjectSettings::defaultResourceDirectories_, &DefaultResourceDirectories::scriptSubdirectory_}, scriptSubdirectory);
	commandInterface->set({settings, &ProjectSettings::defaultResourceDirectories_, &DefaultResourceDirectories::interfaceSubdirectory_}, interfaceSubdirectory);
	commandInterface->set({settings, &ProjectSettings::defaultResourceDirectories_, &DefaultResourceDirectories::shaderSubdirectory_}, shaderSubdirectory);
	application.doOneLoop();

	std::string newProjectFolder = application.activeRaCoProject().project()->currentFolder();

	ASSERT_EQ(raco::core::PathManager::getCachedPath(raco::core::PathManager::FolderTypeKeys::Mesh), newProjectFolder + "/" + meshSubdirectory);
	ASSERT_EQ(raco::core::PathManager::getCachedPath(raco::core::PathManager::FolderTypeKeys::Script), newProjectFolder + "/" + scriptSubdirectory);
	ASSERT_EQ(raco::core::PathManager::getCachedPath(raco::core::PathManager::FolderTypeKeys::Interface), newProjectFolder + "/" + interfaceSubdirectory);
	ASSERT_EQ(raco::core::PathManager::getCachedPath(raco::core::PathManager::FolderTypeKeys::Image), newProjectFolder + "/" + imageSubdirectory);
	ASSERT_EQ(raco::core::PathManager::getCachedPath(raco::core::PathManager::FolderTypeKeys::Shader), newProjectFolder + "/" + shaderSubdirectory);
}

TEST_F(RaCoProjectFixture, saveAsNewProjectGeneratesResourceSubFolders) {
	auto newProjectFolder = (test_path() / "newProject").string();
	std::filesystem::create_directory(newProjectFolder);

	// TODO: use these after RAOS-690 is merged:
	// std::string imageSubdirectory = newProjectFolder + u8"/images äöüß";
	// std::string meshSubdirectory = u8"meshes 滴滴启动纽交所退市";
	// workaround until RAOS-690 is merged:
	std::string imageSubdirectory = newProjectFolder + u8"/images abc";
	std::string meshSubdirectory = u8"meshes def";

	std::string scriptSubdirectory = u8"shared";
	std::string interfaceSubdirectory = u8"shared";
	std::string shaderSubdirectory = u8"shared";

	using namespace raco::user_types;

	const auto& settings = application.activeRaCoProject().project()->settings();
	const auto& commandInterface = application.activeRaCoProject().commandInterface();
	commandInterface->set({settings, &ProjectSettings::defaultResourceDirectories_, &DefaultResourceDirectories::imageSubdirectory_}, imageSubdirectory);
	commandInterface->set({settings, &ProjectSettings::defaultResourceDirectories_, &DefaultResourceDirectories::meshSubdirectory_}, meshSubdirectory);
	commandInterface->set({settings, &ProjectSettings::defaultResourceDirectories_, &DefaultResourceDirectories::scriptSubdirectory_}, scriptSubdirectory);
	commandInterface->set({settings, &ProjectSettings::defaultResourceDirectories_, &DefaultResourceDirectories::interfaceSubdirectory_}, interfaceSubdirectory);
	commandInterface->set({settings, &ProjectSettings::defaultResourceDirectories_, &DefaultResourceDirectories::shaderSubdirectory_}, shaderSubdirectory);
	std::string msg;
	ASSERT_TRUE(application.activeRaCoProject().saveAs(QString::fromStdString(newProjectFolder + "/project.rca"), msg));

	ASSERT_EQ(raco::core::PathManager::getCachedPath(raco::core::PathManager::FolderTypeKeys::Project).string(), newProjectFolder);

	ASSERT_EQ(raco::core::PathManager::getCachedPath(raco::core::PathManager::FolderTypeKeys::Image).string(), imageSubdirectory);
	ASSERT_TRUE(raco::utils::u8path(imageSubdirectory).existsDirectory());

	ASSERT_EQ(raco::core::PathManager::getCachedPath(raco::core::PathManager::FolderTypeKeys::Mesh).string(), newProjectFolder + "/" + meshSubdirectory);
	ASSERT_TRUE(raco::utils::u8path(newProjectFolder + "/" + meshSubdirectory).existsDirectory());

	ASSERT_EQ(raco::core::PathManager::getCachedPath(raco::core::PathManager::FolderTypeKeys::Script).string(), newProjectFolder + "/" + scriptSubdirectory);
	ASSERT_TRUE(raco::utils::u8path(newProjectFolder + "/" + scriptSubdirectory).existsDirectory());

	ASSERT_EQ(raco::core::PathManager::getCachedPath(raco::core::PathManager::FolderTypeKeys::Interface).string(), newProjectFolder + "/" + interfaceSubdirectory);
	ASSERT_TRUE(raco::utils::u8path(newProjectFolder + "/" + interfaceSubdirectory).existsDirectory());

	ASSERT_EQ(raco::core::PathManager::getCachedPath(raco::core::PathManager::FolderTypeKeys::Shader).string(), newProjectFolder + "/" + shaderSubdirectory);
	ASSERT_TRUE(raco::utils::u8path(newProjectFolder + "/" + shaderSubdirectory).existsDirectory());
}

TEST_F(RaCoProjectFixture, saveAsThenCreateNewProjectResetsCachedPaths) {
	raco::components::RaCoPreferences::instance().userProjectsDirectory = QString::fromStdString(test_path().string());

	const auto& prefs = raco::components::RaCoPreferences::instance();
	std::filesystem::create_directory(test_path() / prefs.meshSubdirectory.toStdString());
	std::filesystem::create_directory(test_path() / prefs.scriptSubdirectory.toStdString());
	std::filesystem::create_directory(test_path() / prefs.interfaceSubdirectory.toStdString());
	std::filesystem::create_directory(test_path() / prefs.imageSubdirectory.toStdString());
	std::filesystem::create_directory(test_path() / prefs.shaderSubdirectory.toStdString());
	std::filesystem::create_directory(test_path() / "newProject");

	std::string msg;
	ASSERT_TRUE(application.activeRaCoProject().saveAs(QString::fromStdString((test_path() / "newProject/project.rca").string()), msg));
	application.switchActiveRaCoProject("", {});

	const auto& defaultResourceDirectories = application.activeRaCoProject().project()->settings()->defaultResourceDirectories_;
	auto imageSubdirectory = defaultResourceDirectories->imageSubdirectory_.asString();
	auto meshSubdirectory = defaultResourceDirectories->meshSubdirectory_.asString();
	auto scriptSubdirectory = defaultResourceDirectories->scriptSubdirectory_.asString();
	auto interfaceSubdirectory = defaultResourceDirectories->interfaceSubdirectory_.asString();
	auto shaderSubdirectory = defaultResourceDirectories->shaderSubdirectory_.asString();

	auto newProjectFolder = raco::components::RaCoPreferences::instance().userProjectsDirectory.toStdString();

	ASSERT_EQ(raco::core::PathManager::getCachedPath(raco::core::PathManager::FolderTypeKeys::Project), newProjectFolder);
	ASSERT_EQ(raco::core::PathManager::getCachedPath(raco::core::PathManager::FolderTypeKeys::Mesh), newProjectFolder + "/" + meshSubdirectory);
	ASSERT_EQ(raco::core::PathManager::getCachedPath(raco::core::PathManager::FolderTypeKeys::Script), newProjectFolder + "/" + scriptSubdirectory);
	ASSERT_EQ(raco::core::PathManager::getCachedPath(raco::core::PathManager::FolderTypeKeys::Interface), newProjectFolder + "/" + interfaceSubdirectory);
	ASSERT_EQ(raco::core::PathManager::getCachedPath(raco::core::PathManager::FolderTypeKeys::Image), newProjectFolder + "/" + imageSubdirectory);
	ASSERT_EQ(raco::core::PathManager::getCachedPath(raco::core::PathManager::FolderTypeKeys::Shader), newProjectFolder + "/" + shaderSubdirectory);
}

TEST_F(RaCoProjectFixture, saveAsThenLoadProjectProperlySetCachedPaths) {
	auto newProjectFolder = test_path() / "newProject";
	std::filesystem::create_directory(newProjectFolder);

	std::string msg;
	ASSERT_TRUE(application.activeRaCoProject().saveAs(QString::fromStdString((newProjectFolder / "project.rca").string()), msg));
	application.switchActiveRaCoProject("", {});
	application.switchActiveRaCoProject(QString::fromStdString((newProjectFolder / "project.rca").string()), {});

	const auto& defaultResourceDirectories = application.activeRaCoProject().project()->settings()->defaultResourceDirectories_;
	auto imageSubdirectory = defaultResourceDirectories->imageSubdirectory_.asString();
	auto meshSubdirectory = defaultResourceDirectories->meshSubdirectory_.asString();
	auto scriptSubdirectory = defaultResourceDirectories->scriptSubdirectory_.asString();
	auto interfaceSubdirectory = defaultResourceDirectories->interfaceSubdirectory_.asString();
	auto shaderSubdirectory = defaultResourceDirectories->shaderSubdirectory_.asString();

	ASSERT_EQ(raco::core::PathManager::getCachedPath(raco::core::PathManager::FolderTypeKeys::Project), newProjectFolder);
	ASSERT_EQ(raco::core::PathManager::getCachedPath(raco::core::PathManager::FolderTypeKeys::Mesh), newProjectFolder / meshSubdirectory);
	ASSERT_EQ(raco::core::PathManager::getCachedPath(raco::core::PathManager::FolderTypeKeys::Script), newProjectFolder / scriptSubdirectory);
	ASSERT_EQ(raco::core::PathManager::getCachedPath(raco::core::PathManager::FolderTypeKeys::Interface), newProjectFolder / interfaceSubdirectory);
	ASSERT_EQ(raco::core::PathManager::getCachedPath(raco::core::PathManager::FolderTypeKeys::Image), newProjectFolder / imageSubdirectory);
	ASSERT_EQ(raco::core::PathManager::getCachedPath(raco::core::PathManager::FolderTypeKeys::Shader), newProjectFolder / shaderSubdirectory);
}

TEST_F(RaCoProjectFixture, loadingBrokenJSONFileThrowsException) {
	auto jsonPath = (test_path() / "brokenJSON.rca").string();

	raco::utils::file::write(jsonPath, R"(
{
    "externalProjects": {
    },
	)");
	ASSERT_THROW(application.switchActiveRaCoProject(QString::fromStdString(jsonPath), {}), std::runtime_error);
}

TEST_F(RaCoProjectFixture, saveLoadAsZip) {
	{
		auto linkedScene = raco::createLinkedScene(*application.activeRaCoProject().commandInterface(), test_path());
		auto lua = std::get<raco::user_types::SLuaScript>(linkedScene);
		const auto nodeRotEuler{application.activeRaCoProject().commandInterface()->createObject(raco::user_types::Node::typeDescription.typeName, "node_eul")};
		const auto nodeRotQuat{application.activeRaCoProject().commandInterface()->createObject(raco::user_types::Node::typeDescription.typeName, "node_quat")};

		application.activeRaCoProject().commandInterface()->addLink({lua, {"outputs", "translation"}}, {nodeRotEuler, {"translation"}});
		application.activeRaCoProject().commandInterface()->addLink({lua, {"outputs", "translation"}}, {nodeRotQuat, {"translation"}});
		application.activeRaCoProject().commandInterface()->addLink({lua, {"outputs", "rotation3"}}, {nodeRotEuler, {"rotation"}});
		application.activeRaCoProject().commandInterface()->addLink({lua, {"outputs", "rotation4"}}, {nodeRotQuat, {"rotation"}});
		application.activeRaCoProject().commandInterface()->set({application.activeRaCoProject().project()->settings(), &raco::user_types::ProjectSettings::saveAsZip_}, true);

		std::string msg;
		application.activeRaCoProject().saveAs((test_path() / "project.rca").string().c_str(), msg);
	}
	{
		application.switchActiveRaCoProject(QString::fromStdString((test_path() / "project.rca").string()), {});
		ASSERT_EQ(5, application.activeRaCoProject().project()->instances().size());
		ASSERT_EQ(5, application.activeRaCoProject().project()->links().size());
	}
}

TEST_F(RaCoProjectFixture, saveLoadRotationLinksGetReinstated) {
	{
		auto linkedScene = raco::createLinkedScene(*application.activeRaCoProject().commandInterface(), test_path());
		auto lua = std::get<raco::user_types::SLuaScript>(linkedScene);
		const auto nodeRotEuler{application.activeRaCoProject().commandInterface()->createObject(raco::user_types::Node::typeDescription.typeName, "node_eul")};
		const auto nodeRotQuat{application.activeRaCoProject().commandInterface()->createObject(raco::user_types::Node::typeDescription.typeName, "node_quat")};

		application.activeRaCoProject().commandInterface()->addLink({lua, {"outputs", "translation"}}, {nodeRotEuler, {"translation"}});
		application.activeRaCoProject().commandInterface()->addLink({lua, {"outputs", "translation"}}, {nodeRotQuat, {"translation"}});
		application.activeRaCoProject().commandInterface()->addLink({lua, {"outputs", "rotation3"}}, {nodeRotEuler, {"rotation"}});
		application.activeRaCoProject().commandInterface()->addLink({lua, {"outputs", "rotation4"}}, {nodeRotQuat, {"rotation"}});

		std::string msg;
		application.activeRaCoProject().saveAs((test_path() / "project.rca").string().c_str(), msg);
	}
	{
		application.switchActiveRaCoProject(QString::fromStdString((test_path() / "project.rca").string()), {});
		ASSERT_EQ(5, application.activeRaCoProject().project()->links().size());
	}
}

TEST_F(RaCoProjectFixture, saveLoadRotationInvalidLinksGetReinstated) {
	{
		auto linkedScene = raco::createLinkedScene(*application.activeRaCoProject().commandInterface(), test_path());
		auto lua = std::get<raco::user_types::SLuaScript>(linkedScene);
		const auto nodeRotEuler{application.activeRaCoProject().commandInterface()->createObject(raco::user_types::Node::typeDescription.typeName, "node_eul")};
		const auto nodeRotQuat{application.activeRaCoProject().commandInterface()->createObject(raco::user_types::Node::typeDescription.typeName, "node_quat")};

		application.activeRaCoProject().commandInterface()->addLink({lua, {"outputs", "translation"}}, {nodeRotEuler, {"translation"}});
		application.activeRaCoProject().commandInterface()->addLink({lua, {"outputs", "translation"}}, {nodeRotQuat, {"translation"}});
		application.activeRaCoProject().commandInterface()->addLink({lua, {"outputs", "rotation3"}}, {nodeRotEuler, {"rotation"}});
		application.activeRaCoProject().commandInterface()->addLink({lua, {"outputs", "rotation4"}}, {nodeRotQuat, {"rotation"}});
		application.doOneLoop();

		application.activeRaCoProject().commandInterface()->set({lua, {"uri"}}, std::string());
		application.doOneLoop();

		std::string msg;
		application.activeRaCoProject().saveAs((test_path() / "project.rca").string().c_str(), msg);
	}
	{
		application.switchActiveRaCoProject(QString::fromStdString((test_path() / "project.rca").string()), {});
		ASSERT_EQ(5, application.activeRaCoProject().project()->links().size());
		for (const auto& link : application.activeRaCoProject().project()->links()) {
			ASSERT_FALSE(link->isValid());
		}
		application.doOneLoop();
	}
}

TEST_F(RaCoProjectFixture, saveLoadRotationInvalidLinksGetReinstatedWithDifferentTypes) {
	{
		auto linkedScene = raco::createLinkedScene(*application.activeRaCoProject().commandInterface(), test_path());
		auto lua = std::get<raco::user_types::SLuaScript>(linkedScene);
		const auto nodeRotEuler{application.activeRaCoProject().commandInterface()->createObject(raco::user_types::Node::typeDescription.typeName, "node_eul")};
		const auto nodeRotQuat{application.activeRaCoProject().commandInterface()->createObject(raco::user_types::Node::typeDescription.typeName, "node_quat")};

		application.activeRaCoProject().commandInterface()->addLink({lua, {"outputs", "translation"}}, {nodeRotEuler, {"translation"}});
		application.activeRaCoProject().commandInterface()->addLink({lua, {"outputs", "translation"}}, {nodeRotQuat, {"translation"}});
		application.activeRaCoProject().commandInterface()->addLink({lua, {"outputs", "rotation3"}}, {nodeRotEuler, {"rotation"}});
		application.activeRaCoProject().commandInterface()->addLink({lua, {"outputs", "rotation4"}}, {nodeRotQuat, {"rotation"}});
		application.doOneLoop();

		application.activeRaCoProject().commandInterface()->set({lua, {"uri"}}, std::string());
		application.doOneLoop();

		std::string msg;
		application.activeRaCoProject().saveAs((test_path() / "project.rca").string().c_str(), msg);
	}
	{
		application.switchActiveRaCoProject(QString::fromStdString((test_path() / "project.rca").string()), {});
		application.doOneLoop();

		auto lua = raco::core::Queries::findByName(application.activeRaCoProject().project()->instances(), "lua_script");
		auto switchedRotationTypes = makeFile("switchedTypes.lua", R"(
function interface(IN,OUT)
	OUT.translation = Type:Vec3f()
	OUT.rotation3 = Type:Vec4f()
	OUT.rotation4 = Type:Vec3f()
end
function run(IN,OUT)
end
	)");
		application.activeRaCoProject().commandInterface()->set({lua, {"uri"}}, switchedRotationTypes);
		application.doOneLoop();
	}
}

TEST_F(RaCoProjectFixture, copyPasteShallowAnimationReferencingAnimationChannel) {
	std::string clipboard;
	{
		application.activeRaCoProject().project()->setCurrentPath((test_path() / "project.rca").string());
		auto path = (test_path() / "meshes" / "InterpolationTest" / "InterpolationTest.gltf").string();

		auto [scenegraph, dummyCacheEntry] = raco::getMeshSceneGraphWithHandler(application.activeRaCoProject().meshCache(), {path, 0, false});

		application.activeRaCoProject().commandInterface()->insertAssetScenegraph(scenegraph, path, nullptr);
		application.doOneLoop();

		auto anim = raco::core::Queries::findByName(application.activeRaCoProject().project()->instances(), "Step Scale");
		ASSERT_NE(anim, nullptr);
		clipboard = application.activeRaCoProject().commandInterface()->copyObjects({anim});
	}
	{
		application.activeRaCoProject().commandInterface()->pasteObjects(clipboard);
		ASSERT_NO_FATAL_FAILURE(application.doOneLoop());
	}
}

TEST_F(RaCoProjectFixture, copyPasteDeepAnimationReferencingAnimationChannel) {
	std::string clipboard;
	{
		application.activeRaCoProject().project()->setCurrentPath((test_path() / "project.rca").string());
		auto path = (test_path() / "meshes" / "InterpolationTest" / "InterpolationTest.gltf").string();
		auto [scenegraph, dummyCacheEntry] = raco::getMeshSceneGraphWithHandler(application.activeRaCoProject().meshCache(), {path, 0, false});
		application.activeRaCoProject().commandInterface()->insertAssetScenegraph(scenegraph, path, nullptr);
		application.doOneLoop();

		auto anim = raco::core::Queries::findByName(application.activeRaCoProject().project()->instances(), "Step Scale");
		ASSERT_NE(anim, nullptr);
		clipboard = application.activeRaCoProject().commandInterface()->copyObjects({anim}, true);
	}
	{
		application.activeRaCoProject().commandInterface()->pasteObjects(clipboard);
		ASSERT_NO_FATAL_FAILURE(application.doOneLoop());
	}
}

TEST_F(RaCoProjectFixture, copyPasteShallowLuaScriptReferencingLuaScriptModule) {
	std::string clipboard;
	{
		application.activeRaCoProject().project()->setCurrentPath((test_path() / "project.rca").string());
		auto module = application.activeRaCoProject().commandInterface()->createObject(raco::user_types::LuaScriptModule::typeDescription.typeName, "m");
		auto script = application.activeRaCoProject().commandInterface()->createObject(raco::user_types::LuaScript::typeDescription.typeName, "s");
		application.doOneLoop();

		application.activeRaCoProject().commandInterface()->set({module, &raco::user_types::LuaScriptModule::uri_}, test_path().append("scripts/moduleDefinition.lua").string());
		application.doOneLoop();

		application.activeRaCoProject().commandInterface()->set({script, &raco::user_types::LuaScript::uri_}, test_path().append("scripts/moduleDependency.lua").string());
		application.doOneLoop();

		application.activeRaCoProject().commandInterface()->set({script, {"luaModules", "coalas"}}, module);
		application.doOneLoop();

		clipboard = application.activeRaCoProject().commandInterface()->copyObjects({script});
	}
	{
		application.activeRaCoProject().commandInterface()->pasteObjects(clipboard);
		ASSERT_NO_FATAL_FAILURE(application.doOneLoop());
	}
}

TEST_F(RaCoProjectFixture, copyPasteDeepLuaScriptReferencingLuaScriptModule) {
	std::string clipboard;
	{
		application.activeRaCoProject().project()->setCurrentPath((test_path() / "project.rca").string());
		auto module = application.activeRaCoProject().commandInterface()->createObject(raco::user_types::LuaScriptModule::typeDescription.typeName, "m");
		auto script = application.activeRaCoProject().commandInterface()->createObject(raco::user_types::LuaScript::typeDescription.typeName, "s");
		application.doOneLoop();

		application.activeRaCoProject().commandInterface()->set({module, &raco::user_types::LuaScriptModule::uri_}, test_path().append("scripts/moduleDefinition.lua").string());
		application.doOneLoop();

		application.activeRaCoProject().commandInterface()->set({script, &raco::user_types::LuaScript::uri_}, test_path().append("scripts/moduleDependency.lua").string());
		application.doOneLoop();

		application.activeRaCoProject().commandInterface()->set({script, {"luaModules", "coalas"}}, module);
		application.doOneLoop();

		clipboard = application.activeRaCoProject().commandInterface()->copyObjects({script}, true);
	}
	{
		application.activeRaCoProject().commandInterface()->pasteObjects(clipboard);
		ASSERT_NO_FATAL_FAILURE(application.doOneLoop());
	}
}

#if (!defined(__linux))
// Skip this test in Linux because TC build on Linux seems to not properly change permissions
TEST_F(RaCoProjectFixture, readOnlyProject_appTitleSuffix) {
	std::string clipboard;
	auto projectPath = (test_path() / "project.rca");
	{
		application.activeRaCoProject().project()->setCurrentPath((test_path() / "project.rca").string());
		std::string msg;
		application.activeRaCoProject().saveAs((test_path() / "project.rca").string().c_str(), msg);
	}

	std::error_code ec;
	std::filesystem::permissions(projectPath, std::filesystem::perms::owner_read, ec);
	ASSERT_TRUE(!ec) << "Failed to set permissons. Error code: " << ec.value() << " Error message: '" << ec.message() << "'";

	{
		application.switchActiveRaCoProject(QString::fromStdString((test_path() / "project.rca").string()), {});

		auto expectedAppTitle = fmt::format("{} -  ({}) <read-only>", QCoreApplication::applicationName().toStdString(), application.activeProjectPath());
		EXPECT_EQ(application.generateApplicationTitle().toStdString(), expectedAppTitle);
	}

	std::filesystem::permissions(projectPath, std::filesystem::perms::all, ec);
	ASSERT_TRUE(!ec) << "Failed to set permissons. Error code: " << ec.value() << " Error message: '" << ec.message() << "'";

	{
		application.switchActiveRaCoProject(QString::fromStdString((test_path() / "project.rca").string()), {});

		auto expectedAppTitle = fmt::format("{} -  ({})", QCoreApplication::applicationName().toStdString(), application.activeProjectPath());
		EXPECT_EQ(application.generateApplicationTitle().toStdString(), expectedAppTitle);
	}
}

#endif

TEST_F(RaCoProjectFixture, loadDoubleModuleReferenceWithoutError) {
	// RAOS-819: If a script references two modules make sure we/ramses-logic does not log an error even though the scripts are all correct.
	class CaptureLog : public spdlog::sinks::base_sink<std::mutex> {
	public:
		virtual void sink_it_(const spdlog::details::log_msg& msg) {
			msgs_.emplace_back(msg);
		}
		virtual void flush_() {}

		bool containsError() {
			return std::find_if(std::begin(msgs_), std::end(msgs_), [](const spdlog::details::log_msg& msg) {
				return msg.level >= spdlog::level::err;
			}) != std::end(msgs_);
		}

	private:
		std::vector<spdlog::details::log_msg> msgs_;
	};
	spdlog::drop_all();
	raco::log_system::init();
	const auto logsink = std::make_shared<CaptureLog>();
	raco::log_system::registerSink(logsink);
	application.switchActiveRaCoProject(QString::fromStdString((test_path() / "loadDoubleModuleReferenceWithoutError.rca").string()), {});
	ASSERT_TRUE(application.activeRaCoProject().project() != nullptr);
	ASSERT_FALSE(logsink->containsError());
	raco::log_system::unregisterSink(logsink);
}

TEST_F(RaCoProjectFixture, saveLoadScenegraphOrder) {
	std::string msg;
	{
		SEditorObject node1 = create<Node>("node1");
		SEditorObject node2 = create<Node>("node2");
		ASSERT_TRUE(application.activeRaCoProject().saveAs((test_path() / "project.rca").string().c_str(), msg, true));
	}

	application.switchActiveRaCoProject(QString::fromStdString((test_path() / "project.rca").string()), {});

	auto rootNodeNames = [this]() {
		std::vector<std::string> names;
		for (auto obj : project().instances()) {
			names.emplace_back(obj->objectName());
		}
		return names;
	};

	EXPECT_EQ(rootNodeNames(), std::vector<std::string>({"project", "node1", "node2"}));

	auto node2 = raco::core::Queries::findByName(project().instances(), "node2");
	commandInterface().moveScenegraphChildren({node2}, {}, 0);
	EXPECT_EQ(rootNodeNames(), std::vector<std::string>({"node2", "project", "node1"}));
	ASSERT_TRUE(application.activeRaCoProject().saveAs((test_path() / "project2.rca").string().c_str(), msg));

	application.switchActiveRaCoProject(QString::fromStdString((test_path() / "project2.rca").string()), {});
	EXPECT_EQ(rootNodeNames(), std::vector<std::string>({"node2", "project", "node1"}));
}
