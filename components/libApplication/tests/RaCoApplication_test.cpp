/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "testing/TestEnvironmentCore.h"

#include "core/PathManager.h"
#include "core/Queries.h"
#include "application/RaCoApplication.h"
#include "user_types/Mesh.h"
#include "user_types/Node.h"
#include "user_types/Material.h"
#include "user_types/MeshNode.h"
#include "ramses_adaptor/SceneBackend.h"
#include "ramses_base/BaseEngineBackend.h"

using raco::application::RaCoApplication;
using raco::components::Naming;

class RaCoApplicationFixture : public RacoBaseTest<> {
public:
	raco::ramses_base::HeadlessEngineBackend backend{};
	RaCoApplication application{backend};
};

TEST_F(RaCoApplicationFixture, exportNewProject) {
	application.dataChangeDispatcher()->dispatch(*application.activeRaCoProject().recorder());

	std::string error;
	auto success = application.exportProject(
		application.activeRaCoProject(),
		(cwd_path() / "new.ramses").string().c_str(),
		(cwd_path() / "new.logic").string().c_str(),
		false,
		error);
	ASSERT_TRUE(success);
}

TEST_F(RaCoApplicationFixture, exportDuckProject) {
	auto* commandInterface = application.activeRaCoProject().commandInterface();

	auto mesh = commandInterface->createObject(raco::user_types::Mesh::typeDescription.typeName, Naming::format("MeshDuck"));
	commandInterface->set({mesh, {"uri"}}, std::string{(cwd_path() / "meshes" / "Duck.glb").string()});

	auto material = commandInterface->createObject(raco::user_types::Material::typeDescription.typeName, Naming::format("MaterialDuck"));
	commandInterface->set({material, {"uriVertex"}}, (cwd_path() / "shaders" / "basic.vert").string());
	commandInterface->set({material, {"uriFragment"}}, (cwd_path() / "shaders" / "basic.frag").string());

	auto node = commandInterface->createObject(raco::user_types::Node::typeDescription.typeName, Naming::format("NodeDuck"));
	auto meshNode = commandInterface->createObject(raco::user_types::MeshNode::typeDescription.typeName, Naming::format("MeshNodeDuck"));
	commandInterface->moveScenegraphChild(meshNode, node);

	commandInterface->set(raco::core::ValueHandle{meshNode, {"mesh"}}, mesh);
	commandInterface->set(raco::core::ValueHandle{meshNode, {"materials", "material", "material"}}, material);

	commandInterface->set(raco::core::ValueHandle{meshNode, {"materials", "material", "uniforms", "u_color", "x"}}, 1.0);

	commandInterface->set(raco::core::ValueHandle{meshNode, {"translation", "y"}}, -2.0);
	commandInterface->set(raco::core::ValueHandle{meshNode, {"rotation", "x"}}, 90.0);
	commandInterface->set(raco::core::ValueHandle{meshNode, {"scale", "x"}}, 20.0);
	commandInterface->set(raco::core::ValueHandle{meshNode, {"scale", "y"}}, 20.0);
	commandInterface->set(raco::core::ValueHandle{meshNode, {"scale", "z"}}, 20.0);

	application.dataChangeDispatcher()->dispatch(*application.activeRaCoProject().recorder());

	std::string error;
	auto success = application.exportProject(
		application.activeRaCoProject(),
		(cwd_path() / "new.ramses").string().c_str(),
		(cwd_path() / "new.logic").string().c_str(),
		true,
		error);
	ASSERT_TRUE(success);
}

TEST_F(RaCoApplicationFixture, importglTFScenegraphCorrectNodeAmount) {
	auto* commandInterface = application.activeRaCoProject().commandInterface();
	commandInterface->deleteObjects(application.activeRaCoProject().project()->instances());

	raco::core::MeshDescriptor desc;
	desc.absPath = cwd_path().append("meshes/CesiumMilkTruck/CesiumMilkTruck.gltf").string();
	desc.bakeAllSubmeshes = false;
	auto importSuccess = commandInterface->importAssetScenegraph(desc.absPath, nullptr);

	// generated objects:
	// * Wheels
	// * Cesium_Milk_truck-0
	// * Cesium_Milk_truck-1
	// * Cesium_Milk_truck-2
	// * CesiumMilkTruck.gltf
	// * Yup2Zup
	// * Cesium_Milk_Truck
	// * Cesium_Milk_Truck_meshnodes
	// * Cesium_Milk_Truck_meshnode_0
	// * Cesium_Milk_Truck_meshnode_1
	// * Cesium_Milk_Truck_meshnode_2
	// * Node
	// * Wheels
	// * Node.001
	// * Wheels.001
	// TODO: Multimaterial meshes: This amount will change soon?

	ASSERT_TRUE(importSuccess);
	ASSERT_EQ(application.activeRaCoProject().project()->instances().size(), 15);
}

TEST_F(RaCoApplicationFixture, importglTFScenegraphMeshWithNegativeScaleWillNotBeImported) {
	auto* commandInterface = application.activeRaCoProject().commandInterface();
	commandInterface->deleteObjects(application.activeRaCoProject().project()->instances());

	raco::core::MeshDescriptor desc;
	desc.absPath = cwd_path().append("meshes/negativeScaleQuad.gltf").string();
	desc.bakeAllSubmeshes = false;

	// As long as Assimp flips positive scaling values when encountering negative values, we prevent importing
	ASSERT_FALSE(commandInterface->importAssetScenegraph(desc.absPath, nullptr));
}

TEST_F(RaCoApplicationFixture, importglTFScenegraphCachedMeshPathGetsChanged) {
	auto* commandInterface = application.activeRaCoProject().commandInterface();
	commandInterface->deleteObjects(application.activeRaCoProject().project()->instances());
	raco::core::PathManager::setCachedPath(raco::core::PathManager::MESH_SUB_DIRECTORY, "");

	raco::core::MeshDescriptor desc;
	desc.absPath = cwd_path().append("meshes/CesiumMilkTruck/CesiumMilkTruck.gltf").string();
	desc.bakeAllSubmeshes = false;
	commandInterface->importAssetScenegraph(desc.absPath, nullptr);
	application.dataChangeDispatcher()->dispatch(*application.activeRaCoProject().recorder());

	ASSERT_EQ(raco::core::PathManager::getCachedPath(raco::core::PathManager::MESH_SUB_DIRECTORY), (cwd_path() / "meshes/CesiumMilkTruck").generic_string());
}

TEST_F(RaCoApplicationFixture, importglTFScenegraphCorrectScenegraphStructureTruck) {
	auto* commandInterface = application.activeRaCoProject().commandInterface();
	commandInterface->deleteObjects(application.activeRaCoProject().project()->instances());

	raco::core::MeshDescriptor desc;
	desc.absPath = cwd_path().append("meshes/CesiumMilkTruck/CesiumMilkTruck.gltf").string();
	desc.bakeAllSubmeshes = false;
	commandInterface->importAssetScenegraph(desc.absPath, nullptr);
	application.dataChangeDispatcher()->dispatch(*application.activeRaCoProject().recorder());

	// structure of scenegraph as currently imported by Assimp:
	// - CesiumMilkTruck.gltf
	// -- Yup2Zup
	// --- Cesium_Milk_Truck
	// ---- Cesium_Milk_Truck_meshnodes
	// ----- Cesium_Milk_Truck_meshnode_0
	// ----- Cesium_Milk_Truck_meshnode_1
	// ----- Cesium_Milk_Truck_meshnode_2
	// ---- Node
	// ----- Wheels
	// ---- Node.001
	// ----- Wheels.001
	std::map<std::string, std::string> parentMap = {
		{"Yup2Zup", "CesiumMilkTruck.gltf"},
		{"Cesium_Milk_Truck", "Yup2Zup"},
		{"Cesium_Milk_Truck_meshnodes", "Cesium_Milk_Truck"},
		{"Cesium_Milk_Truck_meshnode_0", "Cesium_Milk_Truck_meshnodes"},
		{"Cesium_Milk_Truck_meshnode_1", "Cesium_Milk_Truck_meshnodes"},
		{"Cesium_Milk_Truck_meshnode_2", "Cesium_Milk_Truck_meshnodes"},
		{"Node", "Cesium_Milk_Truck"},
		{"Wheels", "Node"},
		{"Node.001", "Cesium_Milk_Truck"},
		{"Wheels.001", "Node.001"}};

	auto projectInstances = commandInterface->project()->instances();
	for (const auto& parentPair : parentMap) {
		auto [expectedChildName, expectedParentName] = parentPair;
		auto expectedChild = raco::core::Queries::findByName(raco::core::Queries::filterForNotResource(projectInstances), expectedChildName);
		auto expectedParent = raco::core::Queries::findByName(raco::core::Queries::filterForNotResource(projectInstances), expectedParentName);
		ASSERT_EQ(expectedChild->getParent(), expectedParent);
	}
}

TEST_F(RaCoApplicationFixture, importglTFScenegraphCorrectRootNodeInsertion) {
	auto* commandInterface = application.activeRaCoProject().commandInterface();
	commandInterface->deleteObjects(application.activeRaCoProject().project()->instances());
	auto myRoot = commandInterface->createObject(raco::user_types::Node::typeDescription.typeName, "myRoot");

	raco::core::MeshDescriptor desc;
	desc.absPath = cwd_path().append("meshes/CesiumMilkTruck/CesiumMilkTruck.gltf").string();
	desc.bakeAllSubmeshes = false;
	commandInterface->importAssetScenegraph(desc.absPath, myRoot);

	auto yup2Zup = raco::core::Queries::findByName(commandInterface->project()->instances(), "Yup2Zup");
	auto meshSceneRoot = raco::core::Queries::findByName(commandInterface->project()->instances(), "CesiumMilkTruck.gltf");
	ASSERT_EQ(yup2Zup->getParent(), meshSceneRoot);
	ASSERT_EQ(meshSceneRoot->getParent(), myRoot);
}

TEST_F(RaCoApplicationFixture, importglTFScenegraphCorrectRootNodeRenaming) {
	auto* commandInterface = application.activeRaCoProject().commandInterface();
	commandInterface->deleteObjects(application.activeRaCoProject().project()->instances());

	raco::core::MeshDescriptor desc;
	desc.absPath = cwd_path().append("meshes/ToyCar/ToyCar.gltf").string();
	desc.bakeAllSubmeshes = false;
	auto importSuccess = commandInterface->importAssetScenegraph(desc.absPath, nullptr);
	ASSERT_TRUE(importSuccess);

	auto root = raco::core::Queries::findByName(commandInterface->project()->instances(), "ToyCar.gltf");
	ASSERT_NE(root, nullptr);
}

TEST_F(RaCoApplicationFixture, importglTFScenegraphImportSceneGraphTwice) {
	auto* commandInterface = application.activeRaCoProject().commandInterface();
	commandInterface->deleteObjects(application.activeRaCoProject().project()->instances());
	auto firstRoot = commandInterface->createObject(raco::user_types::Node::typeDescription.typeName, "myRoot");
	auto secondRoot = commandInterface->createObject(raco::user_types::Node::typeDescription.typeName, "myRoot");

	raco::core::MeshDescriptor desc;
	desc.absPath = cwd_path().append("meshes/CesiumMilkTruck/CesiumMilkTruck.gltf").string();
	desc.bakeAllSubmeshes = false;
	ASSERT_TRUE(commandInterface->importAssetScenegraph(desc.absPath, firstRoot));
	ASSERT_TRUE(commandInterface->importAssetScenegraph(desc.absPath, secondRoot));

	//double amount of scenegraph nodes (see importglTFScenegraphCorrectNodeAmount) - 4 duplicate meshes + 2 roots
	ASSERT_EQ(commandInterface->project()->instances().size(), 28);
}

TEST_F(RaCoApplicationFixture, importglTFScenegraphUnbakedMeshesGetTransformed) {
	auto* commandInterface = application.activeRaCoProject().commandInterface();
	commandInterface->deleteObjects(application.activeRaCoProject().project()->instances());

	raco::core::MeshDescriptor desc;
	desc.absPath = cwd_path().append("meshes/CesiumMilkTruck/CesiumMilkTruck.gltf").string();
	desc.bakeAllSubmeshes = false;
	commandInterface->importAssetScenegraph(desc.absPath, nullptr);
	application.dataChangeDispatcher()->dispatch(*application.activeRaCoProject().recorder());

	auto instances = commandInterface->project()->instances();
	auto yup2Zup = raco::core::ValueHandle{raco::core::Queries::findByName(instances, "Yup2Zup")};
	auto node = raco::core::ValueHandle{raco::core::Queries::findByName(instances, "Node")};
	auto node001 = raco::core::ValueHandle{raco::core::Queries::findByName(instances, "Node.001")};

	constexpr auto DELTA = 0.0001;
	ASSERT_NEAR(yup2Zup.get("rotation").get("x").asDouble(), 90.0, DELTA);
	ASSERT_NEAR(yup2Zup.get("rotation").get("y").asDouble(), 45.0, DELTA);
	ASSERT_NEAR(yup2Zup.get("rotation").get("z").asDouble(), 30.0, DELTA);

	ASSERT_NEAR(node.get("translation").get("x").asDouble(), 1.43267, DELTA);
	ASSERT_NEAR(node.get("translation").get("y").asDouble(), 0.0, DELTA);
	ASSERT_NEAR(node.get("translation").get("z").asDouble(), -0.42772, DELTA);

	ASSERT_NEAR(node001.get("translation").get("x").asDouble(), -1.35233, DELTA);
	ASSERT_NEAR(node001.get("translation").get("y").asDouble(), 0.0, DELTA);
	ASSERT_NEAR(node001.get("translation").get("z").asDouble(), -0.42772, DELTA);

	ASSERT_NEAR(node001.get("scale").get("x").asDouble(), 2.0, DELTA);
	ASSERT_NEAR(node001.get("scale").get("y").asDouble(), 0.0, DELTA);
	ASSERT_NEAR(node001.get("scale").get("z").asDouble(), 1.5, DELTA);
}

TEST_F(RaCoApplicationFixture, importglTFScenegraphCorrectAutomaticMaterialAssignment) {
	auto* commandInterface = application.activeRaCoProject().commandInterface();
	commandInterface->deleteObjects(application.activeRaCoProject().project()->instances());

	auto truck = commandInterface->createObject(raco::user_types::Material::typeDescription.typeName, "truck");
	auto glass = commandInterface->createObject(raco::user_types::Material::typeDescription.typeName, "glass");
	auto windowTrim = commandInterface->createObject(raco::user_types::Material::typeDescription.typeName, "window_trim");
	auto wheels = commandInterface->createObject(raco::user_types::Material::typeDescription.typeName, "wheels");

	raco::core::MeshDescriptor desc;
	desc.absPath = cwd_path().append("meshes/CesiumMilkTruck/CesiumMilkTruck.gltf").string();
	desc.bakeAllSubmeshes = false;
	commandInterface->importAssetScenegraph(desc.absPath, nullptr);

	std::map<std::string, raco::core::SEditorObject> materialMap = {
		{"Cesium_Milk_Truck_meshnode_0", truck},
		{"Cesium_Milk_Truck_meshnode_1", glass},
		{"Cesium_Milk_Truck_meshnode_2", windowTrim},
		{"Wheels", wheels},
		{"Wheels.001", wheels},
	};

	for (auto instance : application.activeRaCoProject().project()->instances()) {
		if (instance->getTypeDescription().typeName == raco::user_types::MeshNode::typeDescription.typeName) {
			auto activeMaterial = raco::core::ValueHandle(instance).get("materials")[0].get("material").asRef();
			auto expectedMaterial = materialMap[instance->objectName()];

			ASSERT_EQ(activeMaterial, expectedMaterial);
		}
	}
}

TEST_F(RaCoApplicationFixture, LuaScriptRuntimeErrorCausesInformationForAllScripts) {
	auto* commandInterface = application.activeRaCoProject().commandInterface();

	auto const workingScript{ commandInterface->createObject(raco::user_types::LuaScript::typeDescription.typeName) };
	auto const runtimeErrorScript{commandInterface->createObject(raco::user_types::LuaScript::typeDescription.typeName)};

	commandInterface->set(raco::core::ValueHandle{ runtimeErrorScript, {"uri"}}, cwd_path().append("scripts/runtime-error.lua").string());
	commandInterface->set(raco::core::ValueHandle{ workingScript, {"uri"} }, cwd_path().append("scripts/SimpleScript.lua").string());

	EXPECT_FALSE(application.activeRaCoProject().errors()->hasError(workingScript));
	EXPECT_FALSE(application.activeRaCoProject().errors()->hasError(runtimeErrorScript));

	commandInterface->set(raco::core::ValueHandle{ runtimeErrorScript, {"luaInputs"} }.get("choice"), 1);
	application.doOneLoop();

	EXPECT_TRUE(application.activeRaCoProject().errors()->hasError(runtimeErrorScript));
	EXPECT_EQ(application.activeRaCoProject().errors()->getError(runtimeErrorScript).level(), raco::core::ErrorLevel::ERROR);
	EXPECT_TRUE(application.activeRaCoProject().errors()->getError(runtimeErrorScript).message().find("value") != std::string::npos);
	EXPECT_TRUE(application.activeRaCoProject().errors()->hasError(workingScript));
	EXPECT_EQ(application.activeRaCoProject().errors()->getError(workingScript).level(), raco::core::ErrorLevel::INFORMATION);
	EXPECT_TRUE(application.activeRaCoProject().errors()->getError(workingScript).message().find("runtime error") != std::string::npos);
}

TEST_F(RaCoApplicationFixture, LuaScriptFixingRuntimeErrorRemovesLogicError) {
	auto* commandInterface = application.activeRaCoProject().commandInterface();

	auto emptyScript{commandInterface->createObject(raco::user_types::LuaScript::typeDescription.typeName)};
	auto runtimeErrorScript{commandInterface->createObject(raco::user_types::LuaScript::typeDescription.typeName)};
	commandInterface->set(raco::core::ValueHandle{runtimeErrorScript, {"uri"}}, cwd_path().append("scripts/runtime-error.lua").string());
	commandInterface->set(raco::core::ValueHandle{runtimeErrorScript, {"luaInputs"}}.get("choice"), 1);

	application.doOneLoop();
	commandInterface->set(raco::core::ValueHandle{runtimeErrorScript, {"luaInputs"}}.get("choice"), 0);
	application.doOneLoop();

	EXPECT_FALSE(application.activeRaCoProject().errors()->hasError(runtimeErrorScript));
	EXPECT_FALSE(application.activeRaCoProject().errors()->hasError(emptyScript));
}

TEST_F(RaCoApplicationFixture, LuaScriptFixingRuntimeErrorDoesNotDeleteOtherErrors) {
	auto* commandInterface = application.activeRaCoProject().commandInterface();

	auto emptyURIScript{ commandInterface->createObject(raco::user_types::LuaScript::typeDescription.typeName) };
	auto compileErrorScript{commandInterface->createObject(raco::user_types::LuaScript::typeDescription.typeName)};
	auto runtimeErrorScript1{commandInterface->createObject(raco::user_types::LuaScript::typeDescription.typeName)};
	auto runtimeErrorScript2{ commandInterface->createObject(raco::user_types::LuaScript::typeDescription.typeName) };
	commandInterface->set(raco::core::ValueHandle{ compileErrorScript, {"uri"} }, cwd_path().append("scripts/compile-error.lua").string());
	commandInterface->set(raco::core::ValueHandle{runtimeErrorScript1, {"uri"}}, cwd_path().append("scripts/runtime-error.lua").string());
	commandInterface->set(raco::core::ValueHandle{runtimeErrorScript1, {"luaInputs"}}.get("choice"), 1);
	commandInterface->set(raco::core::ValueHandle{ runtimeErrorScript2, {"uri"} }, cwd_path().append("scripts/runtime-error.lua").string());
	commandInterface->set(raco::core::ValueHandle{ runtimeErrorScript2, {"luaInputs"} }.get("choice"), 0);

	application.doOneLoop();
	EXPECT_EQ(application.activeRaCoProject().errors()->getError(runtimeErrorScript1).level(), raco::core::ErrorLevel::ERROR);
	EXPECT_EQ(application.activeRaCoProject().errors()->getError(runtimeErrorScript2).level(), raco::core::ErrorLevel::INFORMATION);
	EXPECT_EQ(application.activeRaCoProject().errors()->getError(compileErrorScript).level(), raco::core::ErrorLevel::ERROR);
	commandInterface->set(raco::core::ValueHandle{runtimeErrorScript1, {"luaInputs"}}.get("choice"), 0);
	commandInterface->set(raco::core::ValueHandle{ runtimeErrorScript2, {"luaInputs"} }.get("choice"), 1);
	application.doOneLoop();

	EXPECT_EQ(application.activeRaCoProject().errors()->getError(runtimeErrorScript2).level(), raco::core::ErrorLevel::ERROR);
	EXPECT_EQ(application.activeRaCoProject().errors()->getError(runtimeErrorScript1).level(), raco::core::ErrorLevel::INFORMATION);
	EXPECT_EQ(application.activeRaCoProject().errors()->getError(compileErrorScript).level(), raco::core::ErrorLevel::ERROR);
	EXPECT_TRUE(application.activeRaCoProject().errors()->hasError(raco::core::ValueHandle(emptyURIScript, { "uri" })));

	commandInterface->set(raco::core::ValueHandle{ runtimeErrorScript2, {"luaInputs"} }.get("choice"), 0);
	application.doOneLoop();
}

TEST_F(RaCoApplicationFixture, LuaScriptNewestRuntimeErrorGetsProperlyUpdated) {
	auto* commandInterface = application.activeRaCoProject().commandInterface();

	auto runtimeErrorScript1{commandInterface->createObject(raco::user_types::LuaScript::typeDescription.typeName)};
	auto runtimeErrorScript2{commandInterface->createObject(raco::user_types::LuaScript::typeDescription.typeName)};
	commandInterface->set(raco::core::ValueHandle{runtimeErrorScript1, {"uri"}}, cwd_path().append("scripts/runtime-error.lua").string());
	commandInterface->set(raco::core::ValueHandle{runtimeErrorScript2, {"uri"}}, cwd_path().append("scripts/runtime-error.lua").string());
	commandInterface->set(raco::core::ValueHandle{runtimeErrorScript1, {"luaInputs"}}.get("choice"), 1);
	commandInterface->set(raco::core::ValueHandle{runtimeErrorScript2, {"luaInputs"}}.get("choice"), 0);

	application.doOneLoop();
	EXPECT_EQ(application.activeRaCoProject().errors()->getError(runtimeErrorScript1).level(), raco::core::ErrorLevel::ERROR);
	EXPECT_EQ(application.activeRaCoProject().errors()->getError(runtimeErrorScript2).level(), raco::core::ErrorLevel::INFORMATION);

	commandInterface->set(raco::core::ValueHandle{runtimeErrorScript1, {"luaInputs"}}.get("choice"), 0);

	application.doOneLoop();
	EXPECT_TRUE(application.activeRaCoProject().errors()->getAllErrors().empty());

	commandInterface->set(raco::core::ValueHandle{runtimeErrorScript2, {"luaInputs"}}.get("choice"), 1);

	application.doOneLoop();
	EXPECT_EQ(application.activeRaCoProject().errors()->getError(runtimeErrorScript1).level(), raco::core::ErrorLevel::INFORMATION);
	EXPECT_EQ(application.activeRaCoProject().errors()->getError(runtimeErrorScript2).level(), raco::core::ErrorLevel::ERROR);

	commandInterface->set(raco::core::ValueHandle{runtimeErrorScript2, {"luaInputs"}}.get("choice"), 0);

	application.doOneLoop();
	EXPECT_TRUE(application.activeRaCoProject().errors()->getAllErrors().empty());
}

TEST_F(RaCoApplicationFixture, LuaScriptCompileErrorDoesNotCauseErrorForAllScripts) {
	auto* commandInterface = application.activeRaCoProject().commandInterface();

	auto emptyScript{commandInterface->createObject(raco::user_types::LuaScript::typeDescription.typeName)};
	auto compileErrorScript{commandInterface->createObject(raco::user_types::LuaScript::typeDescription.typeName)};
	commandInterface->set(raco::core::ValueHandle{compileErrorScript, {"uri"}}, cwd_path().append("scripts/compile-error.lua").string());

	application.doOneLoop();
	
	EXPECT_TRUE(application.activeRaCoProject().errors()->hasError(compileErrorScript));
	EXPECT_FALSE(application.activeRaCoProject().errors()->hasError(emptyScript));
}

TEST_F(RaCoApplicationFixture, LuaScriptDeletingScriptWithRunTimeErrorUpdatesAllErrors) {
	auto* commandInterface = application.activeRaCoProject().commandInterface();

	auto emptyScript{commandInterface->createObject(raco::user_types::LuaScript::typeDescription.typeName)};
	auto runtimeErrorScript{commandInterface->createObject(raco::user_types::LuaScript::typeDescription.typeName)};
	auto node{commandInterface->createObject(raco::user_types::Node::typeDescription.typeName)};
	auto mesh{commandInterface->createObject(raco::user_types::Mesh::typeDescription.typeName)};
	commandInterface->set(raco::core::ValueHandle{runtimeErrorScript, {"uri"}}, cwd_path().append("scripts/runtime-error.lua").string());
	commandInterface->set(raco::core::ValueHandle{runtimeErrorScript, {"luaInputs"}}.get("choice"), 1);

	application.doOneLoop();

	ASSERT_TRUE(application.activeRaCoProject().errors()->hasError(runtimeErrorScript));
	ASSERT_TRUE(application.activeRaCoProject().errors()->hasError(emptyScript));
	ASSERT_TRUE(application.activeRaCoProject().errors()->hasError(node));
	// resources don't show Ramses Logic errors as of now
	ASSERT_FALSE(application.activeRaCoProject().errors()->hasError(mesh));

	commandInterface->deleteObjects({runtimeErrorScript});

	application.doOneLoop();
	ASSERT_FALSE(application.activeRaCoProject().errors()->hasError(emptyScript));
	ASSERT_FALSE(application.activeRaCoProject().errors()->hasError(node));
	ASSERT_FALSE(application.activeRaCoProject().errors()->hasError(mesh));
	// "empty URI" error
	ASSERT_TRUE(application.activeRaCoProject().errors()->hasError(raco::core::ValueHandle{mesh, {"uri"}}));
}