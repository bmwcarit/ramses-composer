/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "core/Queries.h"
#include "ramses_adaptor/SceneBackend.h"
#include "ramses_base/BaseEngineBackend.h"
#include "application/RaCoProject.h"
#include "application/RaCoApplication.h"
#include "components/RaCoPreferences.h"
#include "core/PathManager.h"
#include "testing/TestEnvironmentCore.h"
#include "testing/TestUtil.h"
#include "user_types/LuaScript.h"
#include "user_types/LuaScriptModule.h"
#include "user_types/Material.h"
#include "user_types/Mesh.h"
#include "user_types/MeshNode.h"
#include "user_types/Node.h"
#include "utils/PathUtils.h"

class RaCoProjectFixture : public RacoBaseTest<> {
public:
	raco::ramses_base::HeadlessEngineBackend backend{};
};

using raco::application::RaCoApplication;

TEST_F(RaCoProjectFixture, saveLoadWithLink) {
	{
		RaCoApplication app{backend};
		raco::createLinkedScene(*app.activeRaCoProject().commandInterface(), cwd_path());
		ASSERT_TRUE(app.activeRaCoProject().saveAs((cwd_path() / "project.rcp").string().c_str()));
	}
	{
		RaCoApplication app{backend, (cwd_path() / "project.rcp").string().c_str()};
		ASSERT_EQ(1, app.activeRaCoProject().project()->links().size());
	}
}


TEST_F(RaCoProjectFixture, saveLoadWithRunningAnimation) {
	{
		RaCoApplication app{backend};
		auto [anim, animChannel, node, link] = raco::createAnimatedScene(*app.activeRaCoProject().commandInterface(), cwd_path());
		app.activeRaCoProject().commandInterface()->set({anim, {"play"}}, true);
		app.activeRaCoProject().commandInterface()->set({anim, {"loop"}}, true);
		ASSERT_TRUE(app.activeRaCoProject().saveAs((cwd_path() / "project.rcp").string().c_str()));
		app.doOneLoop();
		app.doOneLoop();
		app.doOneLoop();
	}
	{
		RaCoApplication app{backend, (cwd_path() / "project.rcp").string().c_str()};
		ASSERT_EQ(1, app.activeRaCoProject().project()->links().size());
		ASSERT_TRUE(app.activeRaCoProject().project()->links().front()->isValid());
	}
}

TEST_F(RaCoProjectFixture, saveLoadWithPausedAnimation) {
	{
		RaCoApplication app{backend};
		auto [anim, animChannel, node, link] = raco::createAnimatedScene(*app.activeRaCoProject().commandInterface(), cwd_path());
		app.activeRaCoProject().commandInterface()->set({anim, {"play"}}, true);
		app.activeRaCoProject().commandInterface()->set({anim, {"loop"}}, true);
		app.doOneLoop();
		app.doOneLoop();
		app.activeRaCoProject().commandInterface()->set({anim, {"play"}}, false);
		app.doOneLoop();
		ASSERT_TRUE(app.activeRaCoProject().saveAs((cwd_path() / "project.rcp").string().c_str()));
	}
	{
		RaCoApplication app{backend, (cwd_path() / "project.rcp").string().c_str()};
		ASSERT_EQ(1, app.activeRaCoProject().project()->links().size());
		ASSERT_TRUE(app.activeRaCoProject().project()->links().front()->isValid());
	}
}

TEST_F(RaCoProjectFixture, saveLoadWithStoppedAnimation) {
	{
		RaCoApplication app{backend};
		auto [anim, animChannel, node, link] = raco::createAnimatedScene(*app.activeRaCoProject().commandInterface(), cwd_path());
		app.activeRaCoProject().commandInterface()->set({anim, {"play"}}, true);
		app.activeRaCoProject().commandInterface()->set({anim, {"loop"}}, true);
		app.activeRaCoProject().commandInterface()->set({anim, {"rewindOnStop"}}, true);
		app.doOneLoop();
		app.activeRaCoProject().commandInterface()->set({anim, {"play"}}, false);
		app.doOneLoop();
		ASSERT_TRUE(app.activeRaCoProject().saveAs((cwd_path() / "project.rcp").string().c_str()));
	}
	{
		RaCoApplication app{backend, (cwd_path() / "project.rcp").string().c_str()};
		ASSERT_EQ(1, app.activeRaCoProject().project()->links().size());
		ASSERT_TRUE(app.activeRaCoProject().project()->links().front()->isValid());
	}
}

TEST_F(RaCoProjectFixture, saveLoadWithBrokenLink) {
	{
		RaCoApplication app{backend};
		auto linkedScene = raco::createLinkedScene(*app.activeRaCoProject().commandInterface(), cwd_path());
		raco::utils::file::write((cwd_path() / "lua_script.lua").string(), R"(
function interface()
	OUT.newTranslation = VEC3F
end
function run()
end
	)");
		ASSERT_TRUE(app.activeRaCoProject().saveAs((cwd_path() / "project.rcp").string().c_str()));
	}
	{
		RaCoApplication app{backend, (cwd_path() / "project.rcp").string().c_str()};
		ASSERT_EQ(1, app.activeRaCoProject().project()->links().size());
		ASSERT_FALSE(app.activeRaCoProject().project()->links()[0]->isValid());
		auto node = raco::core::Queries::findByName(app.activeRaCoProject().project()->instances(), "node");
		ASSERT_TRUE(app.activeRaCoProject().errors()->hasError(node));
	}
}

TEST_F(RaCoProjectFixture, saveLoadWithBrokenAndValidLink) {
	{
		RaCoApplication app{backend};
		const auto luaScript{app.activeRaCoProject().commandInterface()->createObject(raco::user_types::LuaScript::typeDescription.typeName, "lua_script", "lua_script_id")};
		const auto node{app.activeRaCoProject().commandInterface()->createObject(raco::user_types::Node::typeDescription.typeName, "node", "node_id")};
		raco::utils::file::write((cwd_path() / "lua_script.lua").string(), R"(
function interface()
	OUT.translation = VEC3F
	OUT.rotation = VEC3F
end
function run()
end
	)");
		app.activeRaCoProject().commandInterface()->set({luaScript, {"uri"}}, (cwd_path() / "lua_script.lua").string());
		app.activeRaCoProject().commandInterface()->addLink({luaScript, {"luaOutputs", "translation"}}, {node, {"translation"}});
		app.activeRaCoProject().commandInterface()->addLink({luaScript, {"luaOutputs", "rotation"}}, {node, {"rotation"}});

		ASSERT_EQ(2, app.activeRaCoProject().project()->links().size());
		ASSERT_TRUE(app.activeRaCoProject().project()->links()[0]->isValid());
		ASSERT_TRUE(app.activeRaCoProject().project()->links()[1]->isValid());

		ASSERT_TRUE(app.activeRaCoProject().saveAs((cwd_path() / "project.rcp").string().c_str()));
	}

	raco::utils::file::write((cwd_path() / "lua_script.lua").string(), R"(
function interface()
	OUT.newTranslation = VEC3F
	OUT.rotation = VEC3F
end
function run()
end
	)");

	{
		RaCoApplication app{backend, (cwd_path() / "project.rcp").string().c_str()};
		ASSERT_EQ(2, app.activeRaCoProject().project()->links().size());
		ASSERT_FALSE(app.activeRaCoProject().project()->links()[0]->isValid());
		ASSERT_TRUE(app.activeRaCoProject().project()->links()[1]->isValid());
		auto node = raco::core::Queries::findByName(app.activeRaCoProject().project()->instances(), "node");
		ASSERT_TRUE(app.activeRaCoProject().errors()->hasError(node));
	}

	raco::utils::file::write((cwd_path() / "lua_script.lua").string(), R"(
function interface()
	OUT.translation = VEC3F
	OUT.rotation = VEC3F
end
function run()
end
	)");

	{
		RaCoApplication app{backend, (cwd_path() / "project.rcp").string().c_str()};
		ASSERT_EQ(2, app.activeRaCoProject().project()->links().size());
		ASSERT_TRUE(app.activeRaCoProject().project()->links()[0]->isValid());
		ASSERT_TRUE(app.activeRaCoProject().project()->links()[1]->isValid());
		auto node = raco::core::Queries::findByName(app.activeRaCoProject().project()->instances(), "node");
		ASSERT_FALSE(app.activeRaCoProject().errors()->hasError(node));
	}
}

TEST_F(RaCoProjectFixture, saveWithValidLinkLoadWithBrokenLink) {
	{
		RaCoApplication app{backend};
		auto linkedScene = raco::createLinkedScene(*app.activeRaCoProject().commandInterface(), cwd_path());
		ASSERT_TRUE(app.activeRaCoProject().saveAs((cwd_path() / "project.rcp").string().c_str()));
	}
		raco::utils::file::write((cwd_path() / "lua_script.lua").string(), R"(
function interface()
	OUT.newTranslation = VEC3F
end
function run()
end
	)");

	{
		RaCoApplication app{backend, (cwd_path() / "project.rcp").string().c_str()};
		ASSERT_EQ(1, app.activeRaCoProject().project()->links().size());
		ASSERT_FALSE(app.activeRaCoProject().project()->links()[0]->isValid());
		auto node = raco::core::Queries::findByName(app.activeRaCoProject().project()->instances(), "node");
		ASSERT_TRUE(app.activeRaCoProject().errors()->hasError(node));
	}
}


TEST_F(RaCoProjectFixture, saveWithBrokenLinkLoadWithValidLink) {
	{
		RaCoApplication app{backend};
		auto linkedScene = raco::createLinkedScene(*app.activeRaCoProject().commandInterface(), cwd_path());
		raco::utils::file::write((cwd_path() / "lua_script.lua").string(), R"(
function interface()
	OUT.newTranslation = VEC3F
end
function run()
end
	)");
		ASSERT_TRUE(app.activeRaCoProject().saveAs((cwd_path() / "project.rcp").string().c_str()));
	}

	raco::utils::file::write((cwd_path() / "lua_script.lua").string(), R"(
function interface()
	OUT.translation = VEC3F
end
function run()
end
	)");

	{
		RaCoApplication app{backend, (cwd_path() / "project.rcp").string().c_str()};
		ASSERT_EQ(1, app.activeRaCoProject().project()->links().size());
		ASSERT_TRUE(app.activeRaCoProject().project()->links()[0]->isValid());
	}
}


TEST_F(RaCoProjectFixture, saveLoadWithLinkRemoveOutputPropertyBeforeLoading) {
	{
		RaCoApplication app{backend};
		raco::createLinkedScene(*app.activeRaCoProject().commandInterface(), cwd_path());
		ASSERT_TRUE(app.activeRaCoProject().saveAs((cwd_path() / "project.rcp").string().c_str()));
	}

	// replace OUT.translation with OUT.scale in external script file
	raco::utils::file::write((cwd_path() / "lua_script.lua").string(), R"(
function interface()
	OUT.scale = VEC3F
end
function run()
end
	)");

	{
		RaCoApplication app{backend, (cwd_path() / "project.rcp").string().c_str()};
		app.doOneLoop();
		ASSERT_EQ(app.activeRaCoProject().project()->links().size(), 1);
		ASSERT_EQ(app.activeRaCoProject().project()->links()[0]->isValid(), false);
	}
}

TEST_F(RaCoProjectFixture, saveLoadWithLuaScriptNewOutputPropertyGetsCalculated) {
	auto luaScriptPath = (cwd_path() / "lua_script.lua").string();

	raco::utils::file::write(luaScriptPath, R"(
function interface()
	IN.integer = INT
	OUT.integer = INT
end


function run()
	OUT.integer = IN.integer
end

	)");

	{
		RaCoApplication app{backend};
		const auto luaScript{app.activeRaCoProject().commandInterface()->createObject(raco::user_types::LuaScript::typeDescription.typeName, "lua_script", "lua_script_id")};
		app.activeRaCoProject().commandInterface()->set(raco::core::ValueHandle{luaScript, {"uri"}}, luaScriptPath);
		app.activeRaCoProject().commandInterface()->set(raco::core::ValueHandle{luaScript, {"luaInputs", "integer"}}, 5);
		ASSERT_TRUE(app.activeRaCoProject().saveAs((cwd_path() / "project.rcp").string().c_str()));
	}

	raco::utils::file::write((cwd_path() / "lua_script.lua").string(), R"(
function interface()
	IN.integer = INT
	OUT.integerTwo = INT
end


function run()
	OUT.integerTwo = IN.integer
end

	)");
	{
		RaCoApplication app{backend, (cwd_path() / "project.rcp").string().c_str()};
		app.doOneLoop();
		auto luaScript = raco::core::Queries::findByName(app.activeRaCoProject().project()->instances(), "lua_script");
		auto newPropertyOutput = raco::core::ValueHandle{luaScript, {"luaOutputs", "integerTwo"}}.asInt();
		ASSERT_EQ(newPropertyOutput, 5);
	}
}

TEST_F(RaCoProjectFixture, saveAsMeshRerootRelativeURIHierarchyDown) {
	RaCoApplication app{backend};
	app.activeRaCoProject().project()->setCurrentPath((cwd_path() / "project.file").string());
	auto mesh = app.activeRaCoProject().commandInterface()->createObject(raco::user_types::Mesh::typeDescription.typeName, "mesh", "mesh_id");

	std::string relativeUri{"Duck.glb"};
	app.activeRaCoProject().commandInterface()->set({mesh, {"uri"}}, relativeUri);
	
	QDir().mkdir((cwd_path() / "project").string().c_str());
	ASSERT_TRUE(app.activeRaCoProject().saveAs((cwd_path() / "project" / "project.file").string().c_str()));
	std::string newRelativeDuckPath{"../" + relativeUri};

	ASSERT_EQ(app.activeRaCoProject().project()->instances().back()->get("uri")->asString(), newRelativeDuckPath);
}


TEST_F(RaCoProjectFixture, saveAsThenLoadAnimationKeepsChannelAmount) {
	RaCoApplication app{backend};
	auto* commandInterface = app.activeRaCoProject().commandInterface();

	raco::core::MeshDescriptor desc;
	desc.absPath = cwd_path().append("meshes/CesiumMilkTruck/CesiumMilkTruck.gltf").string();
	desc.bakeAllSubmeshes = false;

	auto scenegraph = commandInterface->meshCache()->getMeshScenegraph(desc);
	commandInterface->insertAssetScenegraph(*scenegraph, desc.absPath, nullptr);
	commandInterface->createObject(raco::user_types::Animation::typeDescription.typeName, "userAnim", "userAnim");

	ASSERT_TRUE(app.activeRaCoProject().saveAs((cwd_path() / "anims.rca").string().c_str()));
	app.switchActiveRaCoProject("");
	app.switchActiveRaCoProject(QString::fromStdString((cwd_path() / "anims.rca").string()));

	auto anims = raco::core::Queries::filterByTypeName(app.activeRaCoProject().project()->instances(), {raco::user_types::Animation::typeDescription.typeName});
	auto importedAnim = raco::core::Queries::findByName(anims, "Wheels");
	auto userAnim = raco::core::Queries::findByName(anims, "userAnim");

	ASSERT_EQ(userAnim->as<raco::user_types::Animation>()->animationChannels.asTable().size(), raco::user_types::Animation::ANIMATION_CHANNEL_AMOUNT);
	ASSERT_EQ(importedAnim->as<raco::user_types::Animation>()->animationChannels.asTable().size(), 2);
}

TEST_F(RaCoProjectFixture, saveAsMeshRerootRelativeURIHierarchyUp) {
	RaCoApplication app{backend};
	app.activeRaCoProject().project()->setCurrentPath((cwd_path() / "project" / "project.file").string());
	auto mesh = app.activeRaCoProject().commandInterface()->createObject(raco::user_types::Mesh::typeDescription.typeName, "mesh", "mesh_id");

	std::string relativeUri{"Duck.glb"};
	app.activeRaCoProject().commandInterface()->set({mesh, {"uri"}}, relativeUri);

	ASSERT_TRUE(app.activeRaCoProject().saveAs((cwd_path() / "project.file").string().c_str()));
	std::string newRelativeDuckPath{"project/" + relativeUri};

	ASSERT_EQ(app.activeRaCoProject().project()->instances().back()->get("uri")->asString(), newRelativeDuckPath);
}

TEST_F(RaCoProjectFixture, saveAsMaterialRerootRelativeURI) {
	RaCoApplication app{backend};
	app.activeRaCoProject().project()->setCurrentPath((cwd_path() / "project.file").string());
	auto mat = app.activeRaCoProject().commandInterface()->createObject(raco::user_types::Material::typeDescription.typeName, "material");

	std::string relativeUri{"relativeURI"};
	app.activeRaCoProject().commandInterface()->set({mat, {"uriVertex"}}, relativeUri);
	app.activeRaCoProject().commandInterface()->set({mat, {"uriFragment"}}, relativeUri);

	QDir().mkdir((cwd_path() / "project").string().c_str());
	ASSERT_TRUE(app.activeRaCoProject().saveAs((cwd_path() / "project" / "project.file").string().c_str()));
	std::string newRelativePath{"../" + relativeUri};

	ASSERT_EQ(app.activeRaCoProject().project()->instances().back()->get("uriVertex")->asString(), newRelativePath);
	ASSERT_EQ(app.activeRaCoProject().project()->instances().back()->get("uriFragment")->asString(), newRelativePath);
}

TEST_F(RaCoProjectFixture, saveAsLuaScriptRerootRelativeURI) {
	RaCoApplication app{backend};
	app.activeRaCoProject().project()->setCurrentPath((cwd_path() / "project.file").string());
	auto lua = app.activeRaCoProject().commandInterface()->createObject(raco::user_types::LuaScript::typeDescription.typeName, "lua");

	std::string relativeUri{"relativeURI"};
	app.activeRaCoProject().commandInterface()->set({lua, {"uri"}}, relativeUri);

	QDir().mkdir((cwd_path() / "project").string().c_str());
	ASSERT_TRUE(app.activeRaCoProject().saveAs((cwd_path() / "project" / "project.file").string().c_str()));
	std::string newRelativePath{"../" + relativeUri};

	ASSERT_EQ(app.activeRaCoProject().project()->instances().back()->get("uri")->asString(), newRelativePath);
}

TEST_F(RaCoProjectFixture, saveAsSimulateSavingFromNewProjectCorrectlyRerootedRelativeURI) {
	RaCoApplication app{backend};
	std::filesystem::create_directory(cwd_path() / "project");
	app.activeRaCoProject().project()->setCurrentPath((cwd_path() / "project").string());
	auto mesh = app.activeRaCoProject().commandInterface()->createObject(raco::user_types::Mesh::typeDescription.typeName, "lua");

	std::string relativeUri{"relativeURI.ctm"};
	app.activeRaCoProject().commandInterface()->set({mesh, {"uri"}}, relativeUri);

	QDir().mkdir((cwd_path() / "project").string().c_str());
	ASSERT_TRUE(app.activeRaCoProject().saveAs((cwd_path() / "project" / "project.file").string().c_str()));

	ASSERT_EQ(raco::core::ValueHandle(mesh, {"uri"}).asString(), relativeUri);
}

TEST_F(RaCoProjectFixture, saveAsToDifferentDriveSetsRelativeURIsToAbsolute) {
	RaCoApplication app{backend};
	std::filesystem::create_directory(cwd_path() / "project");
	app.activeRaCoProject().project()->setCurrentPath((cwd_path() / "project").string());
	auto mesh = app.activeRaCoProject().commandInterface()->createObject(raco::user_types::Mesh::typeDescription.typeName, "lua");

	std::string relativeUri{"relativeURI.ctm"};
	app.activeRaCoProject().commandInterface()->set({mesh, {"uri"}}, relativeUri);

	// Usually, an ASSERT_TRUE could be put around saving of RaCo projects,
	// here saving this will fail if the drive Z: does not exist. 
	app.activeRaCoProject().saveAs("Z:/projectOnDifferentDrive.rca");

	ASSERT_EQ(raco::core::ValueHandle(mesh, {"uri"}).asString(), (cwd_path() / "project" / relativeUri).generic_string());
}

TEST_F(RaCoProjectFixture, idChange) {
	RaCoApplication app{backend};
	app.activeRaCoProject().project()->setCurrentPath((cwd_path() / "project.file").string());
	app.doOneLoop();
	ASSERT_EQ(123u, app.sceneBackend()->currentSceneIdValue());

	app.activeRaCoProject().commandInterface()->set({app.activeRaCoProject().project()->settings(), {"sceneId"}}, 1024);
	app.doOneLoop();
	ASSERT_EQ(1024u, app.sceneBackend()->currentSceneIdValue());
}

TEST_F(RaCoProjectFixture, enableTimerFlagChange) {
	RaCoApplication app{backend};
	const auto PROJECT_PATH = QString::fromStdString((cwd_path() / "project.file").string());
	app.activeRaCoProject().commandInterface()->set({app.activeRaCoProject().project()->settings(), {"enableTimerFlag"}}, true);
	ASSERT_TRUE(app.activeRaCoProject().saveAs(PROJECT_PATH));
	app.switchActiveRaCoProject(PROJECT_PATH);
	ASSERT_EQ(app.activeRaCoProject().project()->settings()->enableTimerFlag_.asBool(), true);
}

TEST_F(RaCoProjectFixture, restoredLinkWorksInLogicEngine) {
	RaCoApplication app{backend};
	auto start{app.activeRaCoProject().commandInterface()->createObject(raco::user_types::LuaScript::typeDescription.typeName, "start", "start")};
	app.activeRaCoProject().commandInterface()->set({start, {"uri"}}, cwd_path().append("scripts/SimpleScript.lua").string());
	app.doOneLoop();

	auto end{app.activeRaCoProject().commandInterface()->createObject(raco::user_types::LuaScript::typeDescription.typeName, "end", "end")};
	app.activeRaCoProject().commandInterface()->set({end, {"uri"}}, cwd_path().append("scripts/SimpleScript.lua").string());
	app.doOneLoop();

	app.activeRaCoProject().commandInterface()->addLink({start, {"luaOutputs", "out_float"}}, {end, {"luaInputs", "in_float"}});
	app.doOneLoop();

	app.activeRaCoProject().commandInterface()->set({end, {"uri"}}, std::string());
	app.doOneLoop();
	ASSERT_EQ(app.activeRaCoProject() .project()->links().size(), 1);
	ASSERT_FALSE(app.activeRaCoProject() .project()->links()[0]->isValid());

	app.activeRaCoProject().commandInterface()->set({end, {"uri"}}, cwd_path().append("scripts/SimpleScript.lua").string());
	app.activeRaCoProject().commandInterface()->set({start, {"luaInputs", "in_float"}}, 3.0);
	app.doOneLoop();
	ASSERT_EQ(raco::core::ValueHandle(end, {"luaInputs", "in_float"}).asDouble(), 3.0);
}

TEST_F(RaCoProjectFixture, brokenLinkDoesNotResetProperties) {
	RaCoApplication app{backend};
	auto linkStart{app.activeRaCoProject().commandInterface()->createObject(raco::user_types::LuaScript::typeDescription.typeName, "start", "start")};
	app.activeRaCoProject().commandInterface()->set({linkStart, {"uri"}}, cwd_path().append("scripts/types-scalar.lua").string());

	auto linkEnd{app.activeRaCoProject().commandInterface()->createObject(raco::user_types::LuaScript::typeDescription.typeName, "end", "end")};
	app.activeRaCoProject().commandInterface()->set({linkEnd, {"uri"}}, cwd_path().append("scripts/types-scalar.lua").string());
	app.doOneLoop();

	app.activeRaCoProject().commandInterface()->addLink({linkStart, {"luaOutputs", "ointeger"}}, {linkEnd, {"luaInputs", "integer"}});
	app.doOneLoop();

	app.activeRaCoProject().commandInterface()->set({linkStart, {"luaInputs", "integer"}}, 10);
	app.doOneLoop();

	app.activeRaCoProject().commandInterface()->set({linkStart, {"uri"}}, std::string());
	app.doOneLoop();

	app.activeRaCoProject().commandInterface()->set({linkEnd, {"luaInputs", "float"}}, 20.0);
	app.doOneLoop();

	auto output_int = raco::core::ValueHandle{linkEnd, {"luaOutputs", "ointeger"}}.asInt();
	auto output_bool = raco::core::ValueHandle{linkEnd, {"luaOutputs", "flag"}}.asBool();
	ASSERT_EQ(output_int, 40);
	ASSERT_EQ(output_bool, true);
}

TEST_F(RaCoProjectFixture, launchApplicationWithNoResourceSubFoldersCachedPathsAreSetToUserProjectsDirectoryAndSubFolders) {
	auto newProjectFolder = (cwd_path() / "newProject").generic_string();
	std::filesystem::create_directory(newProjectFolder);
	raco::components::RaCoPreferences::instance().userProjectsDirectory = QString::fromStdString(newProjectFolder);
	RaCoApplication app{backend};

	const auto& prefs = raco::components::RaCoPreferences::instance();
	ASSERT_EQ(raco::core::PathManager::getCachedPath(raco::core::PathManager::FolderTypeKeys::Project), newProjectFolder);
	ASSERT_EQ(raco::core::PathManager::getCachedPath(raco::core::PathManager::FolderTypeKeys::Mesh), newProjectFolder + "/" + prefs.meshSubdirectory.toStdString());
	ASSERT_EQ(raco::core::PathManager::getCachedPath(raco::core::PathManager::FolderTypeKeys::Script), newProjectFolder + "/" + prefs.scriptSubdirectory.toStdString());
	ASSERT_EQ(raco::core::PathManager::getCachedPath(raco::core::PathManager::FolderTypeKeys::Image), newProjectFolder + "/" + prefs.imageSubdirectory.toStdString());
	ASSERT_EQ(raco::core::PathManager::getCachedPath(raco::core::PathManager::FolderTypeKeys::Shader), newProjectFolder + "/" + prefs.shaderSubdirectory.toStdString());
}

TEST_F(RaCoProjectFixture, launchApplicationWithResourceSubFoldersCachedPathsAreSetToUserProjectsDirectoryAndSubFolders) {
	raco::components::RaCoPreferences::instance().userProjectsDirectory = QString::fromStdString(cwd_path().generic_string());
	const auto& prefs = raco::components::RaCoPreferences::instance();
	std::filesystem::create_directory(cwd_path() / prefs.meshSubdirectory.toStdString()); 
	std::filesystem::create_directory(cwd_path() / prefs.scriptSubdirectory.toStdString()); 
	std::filesystem::create_directory(cwd_path() / prefs.imageSubdirectory.toStdString()); 
	std::filesystem::create_directory(cwd_path() / prefs.shaderSubdirectory.toStdString()); 

	RaCoApplication app{backend};
	auto newProjectFolder = raco::components::RaCoPreferences::instance().userProjectsDirectory.toStdString();

	ASSERT_EQ(raco::core::PathManager::getCachedPath(raco::core::PathManager::FolderTypeKeys::Project), newProjectFolder);
	ASSERT_EQ(raco::core::PathManager::getCachedPath(raco::core::PathManager::FolderTypeKeys::Mesh), newProjectFolder + "/" + prefs.meshSubdirectory.toStdString());
	ASSERT_EQ(raco::core::PathManager::getCachedPath(raco::core::PathManager::FolderTypeKeys::Script), newProjectFolder + "/" + prefs.scriptSubdirectory.toStdString());
	ASSERT_EQ(raco::core::PathManager::getCachedPath(raco::core::PathManager::FolderTypeKeys::Image), newProjectFolder + "/" + prefs.imageSubdirectory.toStdString());
	ASSERT_EQ(raco::core::PathManager::getCachedPath(raco::core::PathManager::FolderTypeKeys::Shader), newProjectFolder + "/" + prefs.shaderSubdirectory.toStdString());
}

TEST_F(RaCoProjectFixture, saveAsNewProjectGeneratesResourceSubFolders) {
	auto newProjectFolder = (cwd_path() / "newProject").generic_string();
	std::filesystem::create_directory(newProjectFolder);

	RaCoApplication app{backend};
	ASSERT_TRUE(app.activeRaCoProject().saveAs(QString::fromStdString(newProjectFolder + "/project.rca")));
	
	const auto& prefs = raco::components::RaCoPreferences::instance();
	ASSERT_EQ(raco::core::PathManager::getCachedPath(raco::core::PathManager::FolderTypeKeys::Project), newProjectFolder);

	ASSERT_EQ(raco::core::PathManager::getCachedPath(raco::core::PathManager::FolderTypeKeys::Mesh), newProjectFolder + "/" + prefs.meshSubdirectory.toStdString());
	ASSERT_TRUE(raco::utils::path::isExistingDirectory(newProjectFolder + "/" + prefs.meshSubdirectory.toStdString()));

	ASSERT_EQ(raco::core::PathManager::getCachedPath(raco::core::PathManager::FolderTypeKeys::Script), newProjectFolder + "/" + prefs.scriptSubdirectory.toStdString());
	ASSERT_TRUE(raco::utils::path::isExistingDirectory(newProjectFolder + "/" + prefs.scriptSubdirectory.toStdString()));
	
	ASSERT_EQ(raco::core::PathManager::getCachedPath(raco::core::PathManager::FolderTypeKeys::Image), newProjectFolder + "/" + prefs.imageSubdirectory.toStdString());
	ASSERT_TRUE(raco::utils::path::isExistingDirectory(newProjectFolder + "/" + prefs.imageSubdirectory.toStdString()));
	
	ASSERT_EQ(raco::core::PathManager::getCachedPath(raco::core::PathManager::FolderTypeKeys::Shader), newProjectFolder + "/" + prefs.shaderSubdirectory.toStdString());
	ASSERT_TRUE(raco::utils::path::isExistingDirectory(newProjectFolder + "/" + prefs.shaderSubdirectory.toStdString()));
}

TEST_F(RaCoProjectFixture, saveAsThenCreateNewProjectResetsCachedPaths) {
	raco::components::RaCoPreferences::instance().userProjectsDirectory = QString::fromStdString(cwd_path().generic_string());
	const auto& prefs = raco::components::RaCoPreferences::instance();
	std::filesystem::create_directory(cwd_path() / prefs.meshSubdirectory.toStdString());
	std::filesystem::create_directory(cwd_path() / prefs.scriptSubdirectory.toStdString());
	std::filesystem::create_directory(cwd_path() / prefs.imageSubdirectory.toStdString());
	std::filesystem::create_directory(cwd_path() / prefs.shaderSubdirectory.toStdString());
	std::filesystem::create_directory(cwd_path() / "newProject");

	RaCoApplication app{backend};
	ASSERT_TRUE(app.activeRaCoProject().saveAs(QString::fromStdString((cwd_path() / "newProject/project.rca").generic_string())));
	app.switchActiveRaCoProject("");

	auto newProjectFolder = raco::components::RaCoPreferences::instance().userProjectsDirectory.toStdString();

	ASSERT_EQ(raco::core::PathManager::getCachedPath(raco::core::PathManager::FolderTypeKeys::Project), newProjectFolder);
	ASSERT_EQ(raco::core::PathManager::getCachedPath(raco::core::PathManager::FolderTypeKeys::Mesh), newProjectFolder + "/" + prefs.meshSubdirectory.toStdString());
	ASSERT_EQ(raco::core::PathManager::getCachedPath(raco::core::PathManager::FolderTypeKeys::Script), newProjectFolder + "/" + prefs.scriptSubdirectory.toStdString());
	ASSERT_EQ(raco::core::PathManager::getCachedPath(raco::core::PathManager::FolderTypeKeys::Image), newProjectFolder + "/" + prefs.imageSubdirectory.toStdString());
	ASSERT_EQ(raco::core::PathManager::getCachedPath(raco::core::PathManager::FolderTypeKeys::Shader), newProjectFolder + "/" + prefs.shaderSubdirectory.toStdString());
}

TEST_F(RaCoProjectFixture, saveAsThenLoadProjectProperlySetCachedPaths) {
	auto newProjectFolder = (cwd_path() / "newProject").generic_string();
	std::filesystem::create_directory(newProjectFolder);

	RaCoApplication app{backend};
	ASSERT_TRUE(app.activeRaCoProject().saveAs(QString::fromStdString(newProjectFolder + "/project.rca")));
	app.switchActiveRaCoProject("");
	app.switchActiveRaCoProject(QString::fromStdString(newProjectFolder + "/project.rca"));

	const auto& prefs = raco::components::RaCoPreferences::instance();
	ASSERT_EQ(raco::core::PathManager::getCachedPath(raco::core::PathManager::FolderTypeKeys::Project), newProjectFolder);
	ASSERT_EQ(raco::core::PathManager::getCachedPath(raco::core::PathManager::FolderTypeKeys::Mesh), newProjectFolder + "/" + prefs.meshSubdirectory.toStdString());
	ASSERT_EQ(raco::core::PathManager::getCachedPath(raco::core::PathManager::FolderTypeKeys::Script), newProjectFolder + "/" + prefs.scriptSubdirectory.toStdString());
	ASSERT_EQ(raco::core::PathManager::getCachedPath(raco::core::PathManager::FolderTypeKeys::Image), newProjectFolder + "/" + prefs.imageSubdirectory.toStdString());
	ASSERT_EQ(raco::core::PathManager::getCachedPath(raco::core::PathManager::FolderTypeKeys::Shader), newProjectFolder + "/" + prefs.shaderSubdirectory.toStdString());
}

TEST_F(RaCoProjectFixture, loadingBrokenJSONFileThrowsException) {
	auto jsonPath = (cwd_path() / "brokenJSON.rca").string();

	raco::utils::file::write(jsonPath, R"(
{
    "externalProjects": {
    },
	)");
	RaCoApplication app{backend};
	ASSERT_THROW(app.switchActiveRaCoProject(QString::fromStdString(jsonPath)), std::runtime_error);
}

TEST_F(RaCoProjectFixture, saveLoadRotationLinksGetReinstated) {
	{
		RaCoApplication app{backend};
		auto linkedScene = raco::createLinkedScene(*app.activeRaCoProject().commandInterface(), cwd_path());
		auto lua = std::get<raco::user_types::SLuaScript>(linkedScene);
		const auto nodeRotEuler{app.activeRaCoProject().commandInterface()->createObject(raco::user_types::Node::typeDescription.typeName, "node_eul", "node_eul")};
		const auto nodeRotQuat{app.activeRaCoProject().commandInterface()->createObject(raco::user_types::Node::typeDescription.typeName, "node_quat", "node_quat")};

		app.activeRaCoProject().commandInterface()->addLink({lua, {"luaOutputs", "translation"}}, {nodeRotEuler, {"translation"}});
		app.activeRaCoProject().commandInterface()->addLink({lua, {"luaOutputs", "translation"}}, {nodeRotQuat, {"translation"}});
		app.activeRaCoProject().commandInterface()->addLink({lua, {"luaOutputs", "rotation3"}}, {nodeRotEuler, {"rotation"}});
		app.activeRaCoProject().commandInterface()->addLink({lua, {"luaOutputs", "rotation4"}}, {nodeRotQuat, {"rotation"}});

		app.activeRaCoProject().saveAs((cwd_path() / "project.rcp").string().c_str());
	}
	{
		RaCoApplication app{backend, (cwd_path() / "project.rcp").string().c_str()};
		ASSERT_EQ(5, app.activeRaCoProject().project()->links().size());
	}
}

TEST_F(RaCoProjectFixture, saveLoadRotationInvalidLinksGetReinstated) {
	{
		RaCoApplication app{backend};
		auto linkedScene = raco::createLinkedScene(*app.activeRaCoProject().commandInterface(), cwd_path());
		auto lua = std::get<raco::user_types::SLuaScript>(linkedScene);
		const auto nodeRotEuler{app.activeRaCoProject().commandInterface()->createObject(raco::user_types::Node::typeDescription.typeName, "node_eul", "node_eul")};
		const auto nodeRotQuat{app.activeRaCoProject().commandInterface()->createObject(raco::user_types::Node::typeDescription.typeName, "node_quat", "node_quat")};

		app.activeRaCoProject().commandInterface()->addLink({lua, {"luaOutputs", "translation"}}, {nodeRotEuler, {"translation"}});
		app.activeRaCoProject().commandInterface()->addLink({lua, {"luaOutputs", "translation"}}, {nodeRotQuat, {"translation"}});
		app.activeRaCoProject().commandInterface()->addLink({lua, {"luaOutputs", "rotation3"}}, {nodeRotEuler, {"rotation"}});
		app.activeRaCoProject().commandInterface()->addLink({lua, {"luaOutputs", "rotation4"}}, {nodeRotQuat, {"rotation"}});
		app.doOneLoop();

		app.activeRaCoProject().commandInterface()->set({lua, {"uri"}}, std::string());
		app.doOneLoop();

		app.activeRaCoProject().saveAs((cwd_path() / "project.rcp").string().c_str());
	}
	{
		RaCoApplication app{backend, (cwd_path() / "project.rcp").string().c_str()};
		ASSERT_EQ(5, app.activeRaCoProject().project()->links().size());
		for (const auto& link : app.activeRaCoProject().project()->links()) {
			ASSERT_FALSE(link->isValid());
		}
		app.doOneLoop();
	}
}

TEST_F(RaCoProjectFixture, saveLoadRotationInvalidLinksGetReinstatedWithDifferentTypes) {
	{
		RaCoApplication app{backend};
		auto linkedScene = raco::createLinkedScene(*app.activeRaCoProject().commandInterface(), cwd_path());
		auto lua = std::get<raco::user_types::SLuaScript>(linkedScene);
		const auto nodeRotEuler{app.activeRaCoProject().commandInterface()->createObject(raco::user_types::Node::typeDescription.typeName, "node_eul", "node_eul")};
		const auto nodeRotQuat{app.activeRaCoProject().commandInterface()->createObject(raco::user_types::Node::typeDescription.typeName, "node_quat", "node_quat")};

		app.activeRaCoProject().commandInterface()->addLink({lua, {"luaOutputs", "translation"}}, {nodeRotEuler, {"translation"}});
		app.activeRaCoProject().commandInterface()->addLink({lua, {"luaOutputs", "translation"}}, {nodeRotQuat, {"translation"}});
		app.activeRaCoProject().commandInterface()->addLink({lua, {"luaOutputs", "rotation3"}}, {nodeRotEuler, {"rotation"}});
		app.activeRaCoProject().commandInterface()->addLink({lua, {"luaOutputs", "rotation4"}}, {nodeRotQuat, {"rotation"}});
		app.doOneLoop();

		app.activeRaCoProject().commandInterface()->set({lua, {"uri"}}, std::string());
		app.doOneLoop();

		app.activeRaCoProject().saveAs((cwd_path() / "project.rcp").string().c_str());
	}
	{
		RaCoApplication app{backend, (cwd_path() / "project.rcp").string().c_str()};
		app.doOneLoop();

		auto lua = raco::core::Queries::findByName(app.activeRaCoProject().project()->instances(), "lua_script");
		auto switchedRotationTypes = makeFile("switchedTypes.lua", R"(
function interface()
	OUT.translation = VEC3F
	OUT.rotation3 = VEC4F
	OUT.rotation4 = VEC3F
end
function run()
end
	)");
		app.activeRaCoProject().commandInterface()->set({lua, {"uri"}}, switchedRotationTypes);
		app.doOneLoop();
	}
}


TEST_F(RaCoProjectFixture, copyPasteShallowAnimationReferencingAnimationChannel) {
	std::string clipboard;
	{
		RaCoApplication app{backend};
		app.activeRaCoProject().project()->setCurrentPath((cwd_path() / "project.file").string());
		auto path = (cwd_path() / "meshes" / "InterpolationTest" / "InterpolationTest.gltf").string();
		auto scenegraph = app.activeRaCoProject().meshCache()->getMeshScenegraph({path, 0, false});
		app.activeRaCoProject().commandInterface()->insertAssetScenegraph(*scenegraph, path, nullptr);
		app.doOneLoop();

		auto anim = raco::core::Queries::findByName(app.activeRaCoProject().project()->instances(), "Step Scale");
		ASSERT_NE(anim, nullptr);
		clipboard = app.activeRaCoProject().commandInterface()->copyObjects({anim});
	}
	{
		RaCoApplication app{backend};
		app.activeRaCoProject().commandInterface()->pasteObjects(clipboard);
		ASSERT_NO_FATAL_FAILURE(app.doOneLoop());
	}
}


TEST_F(RaCoProjectFixture, copyPasteDeepAnimationReferencingAnimationChannel) {
	std::string clipboard;
	{
		RaCoApplication app{backend};
		app.activeRaCoProject().project()->setCurrentPath((cwd_path() / "project.file").string());
		auto path = (cwd_path() / "meshes" / "InterpolationTest" / "InterpolationTest.gltf").string();
		auto scenegraph = app.activeRaCoProject().meshCache()->getMeshScenegraph({path, 0, false});
		app.activeRaCoProject().commandInterface()->insertAssetScenegraph(*scenegraph, path, nullptr);
		app.doOneLoop();

		auto anim = raco::core::Queries::findByName(app.activeRaCoProject().project()->instances(), "Step Scale");
		ASSERT_NE(anim, nullptr);
		clipboard = app.activeRaCoProject().commandInterface()->copyObjects({anim}, true);
	}
	{
		RaCoApplication app{backend};
		app.activeRaCoProject().commandInterface()->pasteObjects(clipboard);
		ASSERT_NO_FATAL_FAILURE(app.doOneLoop());
	}
}

TEST_F(RaCoProjectFixture, copyPasteShallowLuaScriptReferencingLuaScriptModule) {
	std::string clipboard;
	{
		RaCoApplication app{ backend };
		app.activeRaCoProject().project()->setCurrentPath((cwd_path() / "project.file").string());
		auto module = app.activeRaCoProject().commandInterface()->createObject(raco::user_types::LuaScriptModule::typeDescription.typeName, "m", "m");
		auto script = app.activeRaCoProject().commandInterface()->createObject(raco::user_types::LuaScript::typeDescription.typeName, "s", "s");
		app.doOneLoop();

		app.activeRaCoProject().commandInterface()->set({ module, &raco::user_types::LuaScriptModule::uri_ }, cwd_path().append("scripts/moduleDefinition.lua").string());
		app.doOneLoop();

		app.activeRaCoProject().commandInterface()->set({ script, &raco::user_types::LuaScript::uri_ }, cwd_path().append("scripts/moduleDependency.lua").string());
		app.doOneLoop();

		app.activeRaCoProject().commandInterface()->set({ script, {"luaModules", "coalas"} }, module);
		app.doOneLoop();

		clipboard = app.activeRaCoProject().commandInterface()->copyObjects({ script });
	}
	{
		RaCoApplication app{ backend };
		app.activeRaCoProject().commandInterface()->pasteObjects(clipboard);
		ASSERT_NO_FATAL_FAILURE(app.doOneLoop());
	}
}

TEST_F(RaCoProjectFixture, copyPasteDeepLuaScriptReferencingLuaScriptModule) {
	std::string clipboard;
	{
		RaCoApplication app{ backend };
		app.activeRaCoProject().project()->setCurrentPath((cwd_path() / "project.file").string());
		auto module = app.activeRaCoProject().commandInterface()->createObject(raco::user_types::LuaScriptModule::typeDescription.typeName, "m", "m");
		auto script = app.activeRaCoProject().commandInterface()->createObject(raco::user_types::LuaScript::typeDescription.typeName, "s", "s");
		app.doOneLoop();

		app.activeRaCoProject().commandInterface()->set({ module, &raco::user_types::LuaScriptModule::uri_ }, cwd_path().append("scripts/moduleDefinition.lua").string());
		app.doOneLoop();

		app.activeRaCoProject().commandInterface()->set({ script, &raco::user_types::LuaScript::uri_ }, cwd_path().append("scripts/moduleDependency.lua").string());
		app.doOneLoop();

		app.activeRaCoProject().commandInterface()->set({ script, {"luaModules", "coalas"} }, module);
		app.doOneLoop();

		clipboard = app.activeRaCoProject().commandInterface()->copyObjects({ script }, true);
	}
	{
		RaCoApplication app{ backend };
		app.activeRaCoProject().commandInterface()->pasteObjects(clipboard);
		ASSERT_NO_FATAL_FAILURE(app.doOneLoop());
	}
}

#if (!defined(__linux))
// Skip this test in Linux because TC build on Linux seems to not properly change permissions
TEST_F(RaCoProjectFixture, readOnlyProject_appTitleSuffix) {
	std::string clipboard;
	auto projectPath = (cwd_path() / "project.rca");
	{
		RaCoApplication app{backend};
		app.activeRaCoProject().project()->setCurrentPath((cwd_path() / "project.rca").string());
		app.activeRaCoProject().saveAs((cwd_path() / "project.rca").string().c_str());
	}

	std::error_code ec;
	std::filesystem::permissions(projectPath, std::filesystem::perms::owner_read, ec);
	ASSERT_TRUE(!ec) << "Failed to set permissons. Error code: " << ec.value() << " Error message: '" << ec.message() << "'";

	{
		RaCoApplication app{backend, (cwd_path() / "project.rca").string().c_str()};

		auto expectedAppTitle = fmt::format("{} -  ({}) <read-only>", RaCoApplication::APPLICATION_NAME.toStdString(), app.activeProjectPath());
		EXPECT_EQ(app.generateApplicationTitle().toStdString(), expectedAppTitle);
	}

	std::filesystem::permissions(projectPath, std::filesystem::perms::all, ec);
	ASSERT_TRUE(!ec) << "Failed to set permissons. Error code: " << ec.value() << " Error message: '" << ec.message() << "'";

	{
		RaCoApplication app{backend, (cwd_path() / "project.rca").string().c_str()};

		auto expectedAppTitle = fmt::format("{} -  ({})", RaCoApplication::APPLICATION_NAME.toStdString(), app.activeProjectPath());
		EXPECT_EQ(app.generateApplicationTitle().toStdString(), expectedAppTitle);
	}
}
#endif