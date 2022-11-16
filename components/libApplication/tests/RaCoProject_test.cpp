/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
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
#include "components/RaCoNameConstants.h"
#include "core/PathManager.h"
#include "spdlog/sinks/base_sink.h"
#include "testing/TestEnvironmentCore.h"
#include "testing/TestUtil.h"
#include "user_types/LuaScript.h"
#include "user_types/LuaScriptModule.h"
#include "user_types/Material.h"
#include "user_types/Mesh.h"
#include "user_types/MeshNode.h"
#include "user_types/Node.h"
#include "utils/u8path.h"

class RaCoProjectFixture : public RacoBaseTest<> {
public:
	raco::ramses_base::HeadlessEngineBackend backend{raco::ramses_base::BaseEngineBackend::maxFeatureLevel};
};

using raco::application::RaCoApplication;
using raco::application::RaCoApplicationLaunchSettings;
using raco::names::PROJECT_FILE_EXTENSION;

TEST_F(RaCoProjectFixture, saveLoadWithLink) {
	{
		RaCoApplication app{backend};
		raco::createLinkedScene(*app.activeRaCoProject().commandInterface(), test_path());
		std::string msg;
		ASSERT_TRUE(app.activeRaCoProject().saveAs((test_path() / "project.rca").string().c_str(), msg));
	}
	{
		RaCoApplicationLaunchSettings settings;
		settings.initialProject = (test_path() / "project.rca").string().c_str();

		RaCoApplication app{backend, settings};
		ASSERT_EQ(1, app.activeRaCoProject().project()->links().size());
	}
}

TEST_F(RaCoProjectFixture, saveLoadWithBrokenLink) {
	{
		RaCoApplication app{backend};
		auto linkedScene = raco::createLinkedScene(*app.activeRaCoProject().commandInterface(), test_path());
		raco::utils::file::write((test_path() / "lua_script.lua").string(), R"(
function interface(IN,OUT)
	OUT.newTranslation = Type:Vec3f()
end
function run(IN,OUT)
end
	)");
		std::string msg;
		ASSERT_TRUE(app.activeRaCoProject().saveAs((test_path() / "project.rca").string().c_str(), msg));
	}
	{
		RaCoApplicationLaunchSettings settings;
		settings.initialProject = (test_path() / "project.rca").string().c_str();

		RaCoApplication app{backend, settings};
		ASSERT_EQ(1, app.activeRaCoProject().project()->links().size());
		ASSERT_FALSE((*app.activeRaCoProject().project()->links().begin())->isValid());
		auto node = raco::core::Queries::findByName(app.activeRaCoProject().project()->instances(), "node");
		ASSERT_TRUE(app.activeRaCoProject().errors()->hasError(node));
	}
}

TEST_F(RaCoProjectFixture, saveLoadWithBrokenAndValidLink) {
	{
		RaCoApplication app{backend};
		const auto luaScript{app.activeRaCoProject().commandInterface()->createObject(raco::user_types::LuaScript::typeDescription.typeName, "lua_script")};
		const auto node{app.activeRaCoProject().commandInterface()->createObject(raco::user_types::Node::typeDescription.typeName, "node")};
		raco::utils::file::write((test_path() / "lua_script.lua").string(), R"(
function interface(IN,OUT)
	OUT.translation = Type:Vec3f()
	OUT.rotation = Type:Vec3f()
end
function run(IN,OUT)
end
	)");
		app.activeRaCoProject().commandInterface()->set({luaScript, {"uri"}}, (test_path() / "lua_script.lua").string());
		app.activeRaCoProject().commandInterface()->addLink({luaScript, {"outputs", "translation"}}, {node, {"translation"}});
		app.activeRaCoProject().commandInterface()->addLink({luaScript, {"outputs", "rotation"}}, {node, {"rotation"}});

		checkLinks(*app.activeRaCoProject().project(),
			{{{luaScript, {"outputs", "translation"}}, {node, {"translation"}}, true},
				{{luaScript, {"outputs", "rotation"}}, {node, {"rotation"}}, true}});

		std::string msg;
		ASSERT_TRUE(app.activeRaCoProject().saveAs((test_path() / "project.rca").string().c_str(), msg));
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
		RaCoApplicationLaunchSettings settings;
		settings.initialProject = (test_path() / "project.rca").string().c_str();

		RaCoApplication app{backend, settings};
		auto node = raco::core::Queries::findByName(app.activeRaCoProject().project()->instances(), "node");
		auto luaScript = raco::core::Queries::findByName(app.activeRaCoProject().project()->instances(), "lua_script");

		checkLinks(*app.activeRaCoProject().project(),
			{{{luaScript, {"outputs", "translation"}}, {node, {"translation"}}, false},
				{{luaScript, {"outputs", "rotation"}}, {node, {"rotation"}}, true}});

		ASSERT_TRUE(app.activeRaCoProject().errors()->hasError(node));
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
		RaCoApplicationLaunchSettings settings;
		settings.initialProject = (test_path() / "project.rca").string().c_str();

		RaCoApplication app{backend, settings};
		auto node = raco::core::Queries::findByName(app.activeRaCoProject().project()->instances(), "node");
		auto luaScript = raco::core::Queries::findByName(app.activeRaCoProject().project()->instances(), "lua_script");

		checkLinks(*app.activeRaCoProject().project(),
			{{{luaScript, {"outputs", "translation"}}, {node, {"translation"}}, true},
				{{luaScript, {"outputs", "rotation"}}, {node, {"rotation"}}, true}});

		ASSERT_FALSE(app.activeRaCoProject().errors()->hasError(node));
	}
}

TEST_F(RaCoProjectFixture, saveWithValidLinkLoadWithBrokenLink) {
	{
		RaCoApplication app{backend};
		auto linkedScene = raco::createLinkedScene(*app.activeRaCoProject().commandInterface(), test_path());
		std::string msg;
		ASSERT_TRUE(app.activeRaCoProject().saveAs((test_path() / "project.rca").string().c_str(), msg));
	}
		raco::utils::file::write((test_path() / "lua_script.lua").string(), R"(
function interface(IN,OUT)
	OUT.newTranslation = Type:Vec3f()
end
function run(IN,OUT)
end
	)");

	{
		RaCoApplicationLaunchSettings settings;
		settings.initialProject = (test_path() / "project.rca").string().c_str();

		RaCoApplication app{backend, settings};
		ASSERT_EQ(1, app.activeRaCoProject().project()->links().size());
		ASSERT_FALSE((*app.activeRaCoProject().project()->links().begin())->isValid());
		auto node = raco::core::Queries::findByName(app.activeRaCoProject().project()->instances(), "node");
		ASSERT_TRUE(app.activeRaCoProject().errors()->hasError(node));
	}
}


TEST_F(RaCoProjectFixture, saveWithBrokenLinkLoadWithValidLink) {
	{
		RaCoApplication app{backend};
		auto linkedScene = raco::createLinkedScene(*app.activeRaCoProject().commandInterface(), test_path());
		raco::utils::file::write((test_path() / "lua_script.lua").string(), R"(
function interface(IN,OUT)
	OUT.newTranslation = Type:Vec3f()
end
function run(IN,OUT)
end
	)");
		std::string msg;
		ASSERT_TRUE(app.activeRaCoProject().saveAs((test_path() / "project.rca").string().c_str(), msg));
	}

	raco::utils::file::write((test_path() / "lua_script.lua").string(), R"(
function interface(IN,OUT)
	OUT.translation = Type:Vec3f()
end
function run(IN,OUT)
end
	)");

	{
		RaCoApplicationLaunchSettings settings;
		settings.initialProject = (test_path() / "project.rca").string().c_str();

		RaCoApplication app{backend, settings};
		ASSERT_EQ(1, app.activeRaCoProject().project()->links().size());
		ASSERT_TRUE((*app.activeRaCoProject().project()->links().begin())->isValid());
	}
}


TEST_F(RaCoProjectFixture, saveLoadWithLinkRemoveOutputPropertyBeforeLoading) {
	{
		RaCoApplication app{backend};
		raco::createLinkedScene(*app.activeRaCoProject().commandInterface(), test_path());
		std::string msg;
		ASSERT_TRUE(app.activeRaCoProject().saveAs((test_path() / "project.rca").string().c_str(), msg));
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
		RaCoApplicationLaunchSettings settings;
		settings.initialProject = (test_path() / "project.rca").string().c_str();

		RaCoApplication app{backend, settings};
		app.doOneLoop();
		ASSERT_EQ(app.activeRaCoProject().project()->links().size(), 1);
		ASSERT_FALSE((*app.activeRaCoProject().project()->links().begin())->isValid());
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
		RaCoApplication app{backend};
		const auto luaScript{app.activeRaCoProject().commandInterface()->createObject(raco::user_types::LuaScript::typeDescription.typeName, "lua_script")};
		app.activeRaCoProject().commandInterface()->set(raco::core::ValueHandle{luaScript, {"uri"}}, luaScriptPath);
		app.activeRaCoProject().commandInterface()->set(raco::core::ValueHandle{luaScript, {"inputs", "integer"}}, 5);
		std::string msg;
		ASSERT_TRUE(app.activeRaCoProject().saveAs((test_path() / "project.rca").string().c_str(), msg));
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
		RaCoApplicationLaunchSettings settings;
		settings.initialProject = (test_path() / "project.rca").string().c_str();

		RaCoApplication app{backend, settings};
		app.doOneLoop();
		auto luaScript = raco::core::Queries::findByName(app.activeRaCoProject().project()->instances(), "lua_script");
		auto newPropertyOutput = raco::core::ValueHandle{luaScript, {"outputs", "integerTwo"}}.asInt();
		ASSERT_EQ(newPropertyOutput, 5);
	}
}

TEST_F(RaCoProjectFixture, saveAsMeshRerootRelativeURIHierarchyDown) {
	RaCoApplication app{backend};
	app.activeRaCoProject().project()->setCurrentPath((test_path() / "project.rca").string());
	auto mesh = app.activeRaCoProject().commandInterface()->createObject(raco::user_types::Mesh::typeDescription.typeName, "mesh");

	std::string relativeUri{"Duck.glb"};
	app.activeRaCoProject().commandInterface()->set({mesh, {"uri"}}, relativeUri);
	
	QDir().mkdir((test_path() / "project").string().c_str());
	std::string msg;
	ASSERT_TRUE(app.activeRaCoProject().saveAs((test_path() / "project" / "project.rca").string().c_str(), msg));
	std::string newRelativeDuckPath{"../" + relativeUri};

	ASSERT_EQ(app.activeRaCoProject().project()->instances().back()->get("uri")->asString(), newRelativeDuckPath);
}


TEST_F(RaCoProjectFixture, saveAsThenLoadAnimationKeepsChannelAmount) {
	RaCoApplication app{backend};
	auto* commandInterface = app.activeRaCoProject().commandInterface();

	raco::core::MeshDescriptor desc;
	desc.absPath = test_path().append("meshes/CesiumMilkTruck/CesiumMilkTruck.gltf").string();
	desc.bakeAllSubmeshes = false;

	auto scenegraph = commandInterface->meshCache()->getMeshScenegraph(desc);
	commandInterface->insertAssetScenegraph(*scenegraph, desc.absPath, nullptr);
	commandInterface->createObject(raco::user_types::Animation::typeDescription.typeName, "userAnim");

	std::string msg;
	ASSERT_TRUE(app.activeRaCoProject().saveAs((test_path() / "anims.rca").string().c_str(), msg));
	app.switchActiveRaCoProject("", {});
	app.switchActiveRaCoProject(QString::fromStdString((test_path() / "anims.rca").string()), {});

	auto anims = raco::core::Queries::filterByTypeName(app.activeRaCoProject().project()->instances(), {raco::user_types::Animation::typeDescription.typeName});
	auto importedAnim = raco::core::Queries::findByName(anims, "Wheels");
	auto userAnim = raco::core::Queries::findByName(anims, "userAnim");

	ASSERT_EQ(userAnim->as<raco::user_types::Animation>()->animationChannels_.asTable().size(), raco::user_types::Animation::ANIMATION_CHANNEL_AMOUNT);
	ASSERT_EQ(importedAnim->as<raco::user_types::Animation>()->animationChannels_.asTable().size(), 2);
}

TEST_F(RaCoProjectFixture, saveAsMeshRerootRelativeURIHierarchyUp) {
	RaCoApplication app{backend};
	app.activeRaCoProject().project()->setCurrentPath((test_path() / "project" / "project.rca").string());
	auto mesh = app.activeRaCoProject().commandInterface()->createObject(raco::user_types::Mesh::typeDescription.typeName, "mesh");

	std::string relativeUri{"Duck.glb"};
	app.activeRaCoProject().commandInterface()->set({mesh, {"uri"}}, relativeUri);

	std::string msg;
	ASSERT_TRUE(app.activeRaCoProject().saveAs((test_path() / "project.rca").string().c_str(), msg));
	std::string newRelativeDuckPath{"project/" + relativeUri};

	ASSERT_EQ(app.activeRaCoProject().project()->instances().back()->get("uri")->asString(), newRelativeDuckPath);
}

TEST_F(RaCoProjectFixture, saveAsMaterialRerootRelativeURI) {
	RaCoApplication app{backend};
	app.activeRaCoProject().project()->setCurrentPath((test_path() / "project.rca").string());
	auto mat = app.activeRaCoProject().commandInterface()->createObject(raco::user_types::Material::typeDescription.typeName, "material");

	std::string relativeUri{"relativeURI"};
	app.activeRaCoProject().commandInterface()->set({mat, {"uriVertex"}}, relativeUri);
	app.activeRaCoProject().commandInterface()->set({mat, {"uriFragment"}}, relativeUri);

	QDir().mkdir((test_path() / "project").string().c_str());
	std::string msg;
	ASSERT_TRUE(app.activeRaCoProject().saveAs((test_path() / "project" / "project.rca").string().c_str(), msg));
	std::string newRelativePath{"../" + relativeUri};

	ASSERT_EQ(app.activeRaCoProject().project()->instances().back()->get("uriVertex")->asString(), newRelativePath);
	ASSERT_EQ(app.activeRaCoProject().project()->instances().back()->get("uriFragment")->asString(), newRelativePath);
}

TEST_F(RaCoProjectFixture, saveAsLuaScriptRerootRelativeURI) {
	RaCoApplication app{backend};
	app.activeRaCoProject().project()->setCurrentPath((test_path() / "project.rca").string());
	auto lua = app.activeRaCoProject().commandInterface()->createObject(raco::user_types::LuaScript::typeDescription.typeName, "lua");

	std::string relativeUri{"relativeURI"};
	app.activeRaCoProject().commandInterface()->set({lua, {"uri"}}, relativeUri);

	QDir().mkdir((test_path() / "project").string().c_str());
	std::string msg;
	ASSERT_TRUE(app.activeRaCoProject().saveAs((test_path() / "project" / "project.rca").string().c_str(), msg));
	std::string newRelativePath{"../" + relativeUri};

	ASSERT_EQ(app.activeRaCoProject().project()->instances().back()->get("uri")->asString(), newRelativePath);
}

TEST_F(RaCoProjectFixture, saveAsSimulateSavingFromNewProjectCorrectlyRerootedRelativeURI) {
	RaCoApplication app{backend};
	std::filesystem::create_directory(test_path() / "project");
	app.activeRaCoProject().project()->setCurrentPath((test_path() / "project").string());
	auto mesh = app.activeRaCoProject().commandInterface()->createObject(raco::user_types::Mesh::typeDescription.typeName, "lua");

	std::string relativeUri{"relativeURI.ctm"};
	app.activeRaCoProject().commandInterface()->set({mesh, {"uri"}}, relativeUri);

	QDir().mkdir((test_path() / "project").string().c_str());
	std::string msg;
	ASSERT_TRUE(app.activeRaCoProject().saveAs((test_path() / "project" / "project.rca").string().c_str(), msg));

	ASSERT_EQ(raco::core::ValueHandle(mesh, {"uri"}).asString(), relativeUri);
}

#if defined(_WIN32)
TEST_F(RaCoProjectFixture, saveAsToDifferentDriveSetsRelativeURIsToAbsolute) {
	RaCoApplication app{backend};
	std::filesystem::create_directory(test_path() / "project");
	app.activeRaCoProject().project()->setCurrentPath((test_path() / "project").string());
	auto mesh = app.activeRaCoProject().commandInterface()->createObject(raco::user_types::Mesh::typeDescription.typeName, "lua");

	std::string relativeUri{"relativeURI.ctm"};
	app.activeRaCoProject().commandInterface()->set({mesh, {"uri"}}, relativeUri);

	// Usually, an ASSERT_TRUE could be put around saving of RaCo projects,
	// here saving this will fail if the drive Z: does not exist. 
	std::string msg;
	app.activeRaCoProject().saveAs("Z:/projectOnDifferentDrive.rca", msg);

	ASSERT_EQ(raco::core::ValueHandle(mesh, {"uri"}).asString(), (test_path() / "project" / relativeUri).string());
}
#endif

TEST_F(RaCoProjectFixture, saveAsWithNewID) {
	RaCoApplication app{backend};
	std::string msg;
	const auto objectInitID = app.activeRaCoProject().project()->settings()->objectID();
	ASSERT_TRUE(app.activeRaCoProject().saveAs((test_path() / "project.rca").string().c_str(), msg, false, true));
	const auto objectNewID = app.activeRaCoProject().project()->settings()->objectID();
	ASSERT_NE(objectInitID, objectNewID);

	raco::core::LoadContext loadContext;
	auto project = raco::application::RaCoProject::loadFromFile(QString::fromUtf8((test_path() / "project.rca").string().data()), &app, loadContext);
	ASSERT_EQ(project->project()->settings()->objectID(), objectNewID);
}

TEST_F(RaCoProjectFixture, saveAsWithNewIDSamePath) {
	RaCoApplication app{backend};
	std::string msg;
	const auto objectInitID = app.activeRaCoProject().project()->settings()->objectID();
	ASSERT_TRUE(app.activeRaCoProject().saveAs((test_path() / "project.rca").string().c_str(), msg, false, true));
	const auto objectNewID1 = app.activeRaCoProject().project()->settings()->objectID();
	ASSERT_NE(objectNewID1, objectInitID);
	ASSERT_TRUE(app.activeRaCoProject().saveAs((test_path() / "project.rca").string().c_str(), msg, false, true));
	const auto objectNewID2 = app.activeRaCoProject().project()->settings()->objectID();
	ASSERT_NE(objectNewID1, objectNewID2);
}

TEST_F(RaCoProjectFixture, saveAsSetProjectName) {
	RaCoApplication app{backend};
	std::string msg;
	ASSERT_TRUE(app.activeRaCoProject().project()->settings()->objectName().empty());
	ASSERT_TRUE(app.activeRaCoProject().saveAs((test_path() / "project.rca").string().c_str(), msg, true, false));
	ASSERT_EQ("project", app.activeRaCoProject().project()->settings()->objectName());

	raco::core::LoadContext loadContext;
	auto project = raco::application::RaCoProject::loadFromFile(QString::fromUtf8((test_path() / "project.rca").string().data()), &app, loadContext);
	ASSERT_EQ("project", project->project()->settings()->objectName());
}

TEST_F(RaCoProjectFixture, idChange) {
	RaCoApplication app{backend};
	app.activeRaCoProject().project()->setCurrentPath((test_path() / "project.rca").string());
	app.doOneLoop();
	ASSERT_EQ(123u, app.sceneBackend()->currentSceneIdValue());

	app.activeRaCoProject().commandInterface()->set({app.activeRaCoProject().project()->settings(), {"sceneId"}}, 1024);
	app.doOneLoop();
	ASSERT_EQ(1024u, app.sceneBackend()->currentSceneIdValue());
}

TEST_F(RaCoProjectFixture, restoredLinkWorksInLogicEngine) {
	RaCoApplication app{backend};
	auto start{app.activeRaCoProject().commandInterface()->createObject(raco::user_types::LuaScript::typeDescription.typeName, "start")};
	app.activeRaCoProject().commandInterface()->set({start, {"uri"}}, test_path().append("scripts/SimpleScript.lua").string());
	app.doOneLoop();

	auto end{app.activeRaCoProject().commandInterface()->createObject(raco::user_types::LuaScript::typeDescription.typeName, "end")};
	app.activeRaCoProject().commandInterface()->set({end, {"uri"}}, test_path().append("scripts/SimpleScript.lua").string());
	app.doOneLoop();

	app.activeRaCoProject().commandInterface()->addLink({start, {"outputs", "out_float"}}, {end, {"inputs", "in_float"}});
	app.doOneLoop();

	app.activeRaCoProject().commandInterface()->set({end, {"uri"}}, std::string());
	app.doOneLoop();
	ASSERT_EQ(app.activeRaCoProject() .project()->links().size(), 1);
	ASSERT_FALSE((*app.activeRaCoProject() .project()->links().begin())->isValid());

	app.activeRaCoProject().commandInterface()->set({end, {"uri"}}, test_path().append("scripts/SimpleScript.lua").string());
	app.activeRaCoProject().commandInterface()->set({start, {"inputs", "in_float"}}, 3.0);
	app.doOneLoop();
	ASSERT_EQ(raco::core::ValueHandle(end, {"inputs", "in_float"}).asDouble(), 3.0);
}

TEST_F(RaCoProjectFixture, brokenLinkDoesNotResetProperties) {
	RaCoApplication app{backend};
	auto linkStart{app.activeRaCoProject().commandInterface()->createObject(raco::user_types::LuaScript::typeDescription.typeName, "start")};
	app.activeRaCoProject().commandInterface()->set({linkStart, {"uri"}}, test_path().append("scripts/types-scalar.lua").string());

	auto linkEnd{app.activeRaCoProject().commandInterface()->createObject(raco::user_types::LuaScript::typeDescription.typeName, "end")};
	app.activeRaCoProject().commandInterface()->set({linkEnd, {"uri"}}, test_path().append("scripts/types-scalar.lua").string());
	app.doOneLoop();

	app.activeRaCoProject().commandInterface()->addLink({linkStart, {"outputs", "ointeger"}}, {linkEnd, {"inputs", "integer"}});
	app.doOneLoop();

	app.activeRaCoProject().commandInterface()->set({linkStart, {"inputs", "integer"}}, 10);
	app.doOneLoop();

	app.activeRaCoProject().commandInterface()->set({linkStart, {"uri"}}, std::string());
	app.doOneLoop();

	app.activeRaCoProject().commandInterface()->set({linkEnd, {"inputs", "float"}}, 20.0);
	app.doOneLoop();

	auto output_int = raco::core::ValueHandle{linkEnd, {"outputs", "ointeger"}}.asInt();
	auto output_bool = raco::core::ValueHandle{linkEnd, {"outputs", "flag"}}.asBool();
	ASSERT_EQ(output_int, 40);
	ASSERT_EQ(output_bool, true);
}

TEST_F(RaCoProjectFixture, launchApplicationWithNoResourceSubFoldersCachedPathsAreSetToUserProjectsDirectoryAndSubFolders) {
	auto newProjectFolder = test_path() / "newProject";
	std::filesystem::create_directory(newProjectFolder);
	raco::components::RaCoPreferences::instance().userProjectsDirectory = QString::fromStdString(newProjectFolder.string());
	RaCoApplication app{backend};
	
	const auto& defaultResourceDirectories = app.activeRaCoProject().project()->settings()->defaultResourceDirectories_;
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

	const auto& prefs = raco::components::RaCoPreferences::instance();
	std::filesystem::create_directory(test_path() / prefs.meshSubdirectory.toStdString());
	std::filesystem::create_directory(test_path() / prefs.scriptSubdirectory.toStdString());
	std::filesystem::create_directory(test_path() / prefs.interfaceSubdirectory.toStdString());
	std::filesystem::create_directory(test_path() / prefs.imageSubdirectory.toStdString());
	std::filesystem::create_directory(test_path() / prefs.shaderSubdirectory.toStdString()); 

	RaCoApplication app{backend};
	auto newProjectFolder = raco::components::RaCoPreferences::instance().userProjectsDirectory.toStdString();

	const auto& defaultResourceDirectories = app.activeRaCoProject().project()->settings()->defaultResourceDirectories_;
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

	RaCoApplication app{backend};
	std::string msg;
	ASSERT_TRUE(app.activeRaCoProject().saveAs((test_path() / "project.rca").string().c_str(), msg, false, false));

	const auto& settings = app.activeRaCoProject().project()->settings();
	const auto& commandInterface = app.activeRaCoProject().commandInterface();

	commandInterface->set({settings, &ProjectSettings::defaultResourceDirectories_, &ProjectSettings::DefaultResourceDirectories::imageSubdirectory_}, imageSubdirectory);
	commandInterface->set({settings, &ProjectSettings::defaultResourceDirectories_, &ProjectSettings::DefaultResourceDirectories::meshSubdirectory_}, meshSubdirectory);
	commandInterface->set({settings, &ProjectSettings::defaultResourceDirectories_, &ProjectSettings::DefaultResourceDirectories::scriptSubdirectory_}, scriptSubdirectory);
	commandInterface->set({settings, &ProjectSettings::defaultResourceDirectories_, &ProjectSettings::DefaultResourceDirectories::interfaceSubdirectory_}, interfaceSubdirectory);
	commandInterface->set({settings, &ProjectSettings::defaultResourceDirectories_, &ProjectSettings::DefaultResourceDirectories::shaderSubdirectory_}, shaderSubdirectory);
	app.doOneLoop();

	std::string newProjectFolder = app.activeRaCoProject().project()->currentFolder();

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

	RaCoApplication app{backend};
	std::string msg;
	ASSERT_TRUE(app.activeRaCoProject().saveAs((test_path() / "project.rca").string().c_str(), msg,  false, true));

	const auto& settings = app.activeRaCoProject().project()->settings();
	const auto& commandInterface = app.activeRaCoProject().commandInterface();

	commandInterface->set({settings, &ProjectSettings::defaultResourceDirectories_, &ProjectSettings::DefaultResourceDirectories::imageSubdirectory_}, imageSubdirectory);
	commandInterface->set({settings, &ProjectSettings::defaultResourceDirectories_, &ProjectSettings::DefaultResourceDirectories::meshSubdirectory_}, meshSubdirectory);
	commandInterface->set({settings, &ProjectSettings::defaultResourceDirectories_, &ProjectSettings::DefaultResourceDirectories::scriptSubdirectory_}, scriptSubdirectory);
	commandInterface->set({settings, &ProjectSettings::defaultResourceDirectories_, &ProjectSettings::DefaultResourceDirectories::interfaceSubdirectory_}, interfaceSubdirectory);
	commandInterface->set({settings, &ProjectSettings::defaultResourceDirectories_, &ProjectSettings::DefaultResourceDirectories::shaderSubdirectory_}, shaderSubdirectory);
	app.doOneLoop();

	std::string newProjectFolder = app.activeRaCoProject().project()->currentFolder();
	
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
	//std::string imageSubdirectory = newProjectFolder + u8"/images äöüß";
	//std::string meshSubdirectory = u8"meshes 滴滴启动纽交所退市";
	// workaround until RAOS-690 is merged:
	std::string imageSubdirectory = newProjectFolder + u8"/images abc";
	std::string meshSubdirectory = u8"meshes def";


	std::string scriptSubdirectory = u8"shared";
	std::string interfaceSubdirectory = u8"shared";
	std::string shaderSubdirectory = u8"shared";

	using namespace raco::user_types;

	RaCoApplication app{backend};
	const auto& settings = app.activeRaCoProject().project()->settings();
	const auto& commandInterface = app.activeRaCoProject().commandInterface();
	commandInterface->set({settings, &ProjectSettings::defaultResourceDirectories_, &ProjectSettings::DefaultResourceDirectories::imageSubdirectory_}, imageSubdirectory);
	commandInterface->set({settings, &ProjectSettings::defaultResourceDirectories_, &ProjectSettings::DefaultResourceDirectories::meshSubdirectory_}, meshSubdirectory);
	commandInterface->set({settings, &ProjectSettings::defaultResourceDirectories_, &ProjectSettings::DefaultResourceDirectories::scriptSubdirectory_}, scriptSubdirectory);
	commandInterface->set({settings, &ProjectSettings::defaultResourceDirectories_, &ProjectSettings::DefaultResourceDirectories::interfaceSubdirectory_}, interfaceSubdirectory);
	commandInterface->set({settings, &ProjectSettings::defaultResourceDirectories_, &ProjectSettings::DefaultResourceDirectories::shaderSubdirectory_}, shaderSubdirectory);
	std::string msg;
	ASSERT_TRUE(app.activeRaCoProject().saveAs(QString::fromStdString(newProjectFolder + "/project.rca"), msg));

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

	RaCoApplication app{backend};
	std::string msg;
	ASSERT_TRUE(app.activeRaCoProject().saveAs(QString::fromStdString((test_path() / "newProject/project.rca").string()), msg));
	app.switchActiveRaCoProject("", {});

	const auto& defaultResourceDirectories = app.activeRaCoProject().project()->settings()->defaultResourceDirectories_;
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

	RaCoApplication app{backend};
	std::string msg;
	ASSERT_TRUE(app.activeRaCoProject().saveAs(QString::fromStdString((newProjectFolder / "project.rca").string()), msg));
	app.switchActiveRaCoProject("", {});
	app.switchActiveRaCoProject(QString::fromStdString((newProjectFolder / "project.rca").string()), {});

	const auto& defaultResourceDirectories = app.activeRaCoProject().project()->settings()->defaultResourceDirectories_;
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
	RaCoApplication app{backend};
	ASSERT_THROW(app.switchActiveRaCoProject(QString::fromStdString(jsonPath), {}), std::runtime_error);
}

TEST_F(RaCoProjectFixture, saveLoadAsZip) {
	{
		RaCoApplication app{backend};
		auto linkedScene = raco::createLinkedScene(*app.activeRaCoProject().commandInterface(), test_path());
		auto lua = std::get<raco::user_types::SLuaScript>(linkedScene);
		const auto nodeRotEuler{app.activeRaCoProject().commandInterface()->createObject(raco::user_types::Node::typeDescription.typeName, "node_eul")};
		const auto nodeRotQuat{app.activeRaCoProject().commandInterface()->createObject(raco::user_types::Node::typeDescription.typeName, "node_quat")};

		app.activeRaCoProject().commandInterface()->addLink({lua, {"outputs", "translation"}}, {nodeRotEuler, {"translation"}});
		app.activeRaCoProject().commandInterface()->addLink({lua, {"outputs", "translation"}}, {nodeRotQuat, {"translation"}});
		app.activeRaCoProject().commandInterface()->addLink({lua, {"outputs", "rotation3"}}, {nodeRotEuler, {"rotation"}});
		app.activeRaCoProject().commandInterface()->addLink({lua, {"outputs", "rotation4"}}, {nodeRotQuat, {"rotation"}});
		app.activeRaCoProject().commandInterface()->set({app.activeRaCoProject().project()->settings(), &raco::user_types::ProjectSettings::saveAsZip_}, true);

		std::string msg;
		app.activeRaCoProject().saveAs((test_path() / "project.rca").string().c_str(), msg);
	}
	{
		RaCoApplicationLaunchSettings settings;
		settings.initialProject = (test_path() / "project.rca").string().c_str();

		RaCoApplication app{backend, settings};
		ASSERT_EQ(10, app.activeRaCoProject().project()->instances().size());
		ASSERT_EQ(5, app.activeRaCoProject().project()->links().size());
	}
}

TEST_F(RaCoProjectFixture, saveLoadRotationLinksGetReinstated) {
	{
		RaCoApplication app{backend};
		auto linkedScene = raco::createLinkedScene(*app.activeRaCoProject().commandInterface(), test_path());
		auto lua = std::get<raco::user_types::SLuaScript>(linkedScene);
		const auto nodeRotEuler{app.activeRaCoProject().commandInterface()->createObject(raco::user_types::Node::typeDescription.typeName, "node_eul")};
		const auto nodeRotQuat{app.activeRaCoProject().commandInterface()->createObject(raco::user_types::Node::typeDescription.typeName, "node_quat")};

		app.activeRaCoProject().commandInterface()->addLink({lua, {"outputs", "translation"}}, {nodeRotEuler, {"translation"}});
		app.activeRaCoProject().commandInterface()->addLink({lua, {"outputs", "translation"}}, {nodeRotQuat, {"translation"}});
		app.activeRaCoProject().commandInterface()->addLink({lua, {"outputs", "rotation3"}}, {nodeRotEuler, {"rotation"}});
		app.activeRaCoProject().commandInterface()->addLink({lua, {"outputs", "rotation4"}}, {nodeRotQuat, {"rotation"}});

		std::string msg;
		app.activeRaCoProject().saveAs((test_path() / "project.rca").string().c_str(), msg);
	}
	{
		RaCoApplicationLaunchSettings settings;
		settings.initialProject = (test_path() / "project.rca").string().c_str();

		RaCoApplication app{backend, settings};
		ASSERT_EQ(5, app.activeRaCoProject().project()->links().size());
	}
}

TEST_F(RaCoProjectFixture, saveLoadRotationInvalidLinksGetReinstated) {
	{
		RaCoApplication app{backend};
		auto linkedScene = raco::createLinkedScene(*app.activeRaCoProject().commandInterface(), test_path());
		auto lua = std::get<raco::user_types::SLuaScript>(linkedScene);
		const auto nodeRotEuler{app.activeRaCoProject().commandInterface()->createObject(raco::user_types::Node::typeDescription.typeName, "node_eul")};
		const auto nodeRotQuat{app.activeRaCoProject().commandInterface()->createObject(raco::user_types::Node::typeDescription.typeName, "node_quat")};

		app.activeRaCoProject().commandInterface()->addLink({lua, {"outputs", "translation"}}, {nodeRotEuler, {"translation"}});
		app.activeRaCoProject().commandInterface()->addLink({lua, {"outputs", "translation"}}, {nodeRotQuat, {"translation"}});
		app.activeRaCoProject().commandInterface()->addLink({lua, {"outputs", "rotation3"}}, {nodeRotEuler, {"rotation"}});
		app.activeRaCoProject().commandInterface()->addLink({lua, {"outputs", "rotation4"}}, {nodeRotQuat, {"rotation"}});
		app.doOneLoop();

		app.activeRaCoProject().commandInterface()->set({lua, {"uri"}}, std::string());
		app.doOneLoop();

		std::string msg;
		app.activeRaCoProject().saveAs((test_path() / "project.rca").string().c_str(), msg);
	}
	{
		RaCoApplicationLaunchSettings settings;
		settings.initialProject = (test_path() / "project.rca").string().c_str();

		RaCoApplication app{backend, settings};
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
		auto linkedScene = raco::createLinkedScene(*app.activeRaCoProject().commandInterface(), test_path());
		auto lua = std::get<raco::user_types::SLuaScript>(linkedScene);
		const auto nodeRotEuler{app.activeRaCoProject().commandInterface()->createObject(raco::user_types::Node::typeDescription.typeName, "node_eul")};
		const auto nodeRotQuat{app.activeRaCoProject().commandInterface()->createObject(raco::user_types::Node::typeDescription.typeName, "node_quat")};
		
		app.activeRaCoProject().commandInterface()->addLink({lua, {"outputs", "translation"}}, {nodeRotEuler, {"translation"}});
		app.activeRaCoProject().commandInterface()->addLink({lua, {"outputs", "translation"}}, {nodeRotQuat, {"translation"}});
		app.activeRaCoProject().commandInterface()->addLink({lua, {"outputs", "rotation3"}}, {nodeRotEuler, {"rotation"}});
		app.activeRaCoProject().commandInterface()->addLink({lua, {"outputs", "rotation4"}}, {nodeRotQuat, {"rotation"}});
		app.doOneLoop();

		app.activeRaCoProject().commandInterface()->set({lua, {"uri"}}, std::string());
		app.doOneLoop();

		std::string msg;
		app.activeRaCoProject().saveAs((test_path() / "project.rca").string().c_str(), msg);
	}
	{
		RaCoApplicationLaunchSettings settings;
		settings.initialProject = (test_path() / "project.rca").string().c_str();

		RaCoApplication app{backend, settings};
		app.doOneLoop();

		auto lua = raco::core::Queries::findByName(app.activeRaCoProject().project()->instances(), "lua_script");
		auto switchedRotationTypes = makeFile("switchedTypes.lua", R"(
function interface(IN,OUT)
	OUT.translation = Type:Vec3f()
	OUT.rotation3 = Type:Vec4f()
	OUT.rotation4 = Type:Vec3f()
end
function run(IN,OUT)
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
		app.activeRaCoProject().project()->setCurrentPath((test_path() / "project.rca").string());
		auto path = (test_path() / "meshes" / "InterpolationTest" / "InterpolationTest.gltf").string();
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
		app.activeRaCoProject().project()->setCurrentPath((test_path() / "project.rca").string());
		auto path = (test_path() / "meshes" / "InterpolationTest" / "InterpolationTest.gltf").string();
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
		app.activeRaCoProject().project()->setCurrentPath((test_path() / "project.rca").string());
		auto module = app.activeRaCoProject().commandInterface()->createObject(raco::user_types::LuaScriptModule::typeDescription.typeName, "m");
		auto script = app.activeRaCoProject().commandInterface()->createObject(raco::user_types::LuaScript::typeDescription.typeName, "s");
		app.doOneLoop();

		app.activeRaCoProject().commandInterface()->set({ module, &raco::user_types::LuaScriptModule::uri_ }, test_path().append("scripts/moduleDefinition.lua").string());
		app.doOneLoop();

		app.activeRaCoProject().commandInterface()->set({ script, &raco::user_types::LuaScript::uri_ }, test_path().append("scripts/moduleDependency.lua").string());
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
		app.activeRaCoProject().project()->setCurrentPath((test_path() / "project.rca").string());
		auto module = app.activeRaCoProject().commandInterface()->createObject(raco::user_types::LuaScriptModule::typeDescription.typeName, "m");
		auto script = app.activeRaCoProject().commandInterface()->createObject(raco::user_types::LuaScript::typeDescription.typeName, "s");
		app.doOneLoop();

		app.activeRaCoProject().commandInterface()->set({ module, &raco::user_types::LuaScriptModule::uri_ }, test_path().append("scripts/moduleDefinition.lua").string());
		app.doOneLoop();

		app.activeRaCoProject().commandInterface()->set({ script, &raco::user_types::LuaScript::uri_ }, test_path().append("scripts/moduleDependency.lua").string());
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
	auto projectPath = (test_path() / "project.rca");
	{
		RaCoApplication app{backend};
		app.activeRaCoProject().project()->setCurrentPath((test_path() / "project.rca").string());
		std::string msg;
		app.activeRaCoProject().saveAs((test_path() / "project.rca").string().c_str(), msg);
	}

	std::error_code ec;
	std::filesystem::permissions(projectPath, std::filesystem::perms::owner_read, ec);
	ASSERT_TRUE(!ec) << "Failed to set permissons. Error code: " << ec.value() << " Error message: '" << ec.message() << "'";

	{
		RaCoApplicationLaunchSettings settings;
		settings.initialProject = (test_path() / "project.rca").string().c_str();

		RaCoApplication app{backend, settings};

		auto expectedAppTitle = fmt::format("{} -  ({}) <read-only>", QCoreApplication::applicationName().toStdString(), app.activeProjectPath());
		EXPECT_EQ(app.generateApplicationTitle().toStdString(), expectedAppTitle);
	}

	std::filesystem::permissions(projectPath, std::filesystem::perms::all, ec);
	ASSERT_TRUE(!ec) << "Failed to set permissons. Error code: " << ec.value() << " Error message: '" << ec.message() << "'";

	{
		RaCoApplicationLaunchSettings settings;
		settings.initialProject = (test_path() / "project.rca").string().c_str();

		RaCoApplication app{backend, settings};

		auto expectedAppTitle = fmt::format("{} -  ({})", QCoreApplication::applicationName().toStdString(), app.activeProjectPath());
		EXPECT_EQ(app.generateApplicationTitle().toStdString(), expectedAppTitle);
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
	raco::core::LoadContext loadContext;
	RaCoApplication app{backend};
	auto project = app.activeRaCoProject().loadFromFile(QString::fromUtf8((test_path() / "loadDoubleModuleReferenceWithoutError.rca").string().data()), &app, loadContext);
	ASSERT_TRUE(project != nullptr);
	ASSERT_FALSE(logsink->containsError());
	raco::log_system::unregisterSink(logsink);
}
