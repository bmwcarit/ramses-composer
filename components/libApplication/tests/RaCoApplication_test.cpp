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
#include "user_types/Animation.h"
#include "user_types/AnimationChannel.h"
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
	commandInterface->moveScenegraphChildren({meshNode}, node);

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


	auto scenegraph = commandInterface->meshCache()->getMeshScenegraph(desc);
	commandInterface->insertAssetScenegraph(*scenegraph, desc.absPath, nullptr);

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
	// * Wheels (Animation)
	// * Wheels.ch0
	// * Wheels.ch1
	// TODO: Multimaterial meshes: This amount will change soon?

	ASSERT_EQ(application.activeRaCoProject().project()->instances().size(), 18);

	ASSERT_EQ(application.activeRaCoProject().project()->links().size(), 2) << "CesiumMilkTruck should have two spinning wheel animations";
}

TEST_F(RaCoApplicationFixture, importglTFScenegraphVectorsGetLinked) {
	auto* commandInterface = application.activeRaCoProject().commandInterface();
	commandInterface->deleteObjects(application.activeRaCoProject().project()->instances());

	raco::core::MeshDescriptor desc;
	desc.absPath = cwd_path().append("meshes/InterpolationTest/InterpolationTest.gltf").string();
	desc.bakeAllSubmeshes = false;


	auto scenegraph = commandInterface->meshCache()->getMeshScenegraph(desc);
	commandInterface->insertAssetScenegraph(*scenegraph, desc.absPath, nullptr);

	ASSERT_EQ(application.activeRaCoProject().project()->links().size(), 9) << "InterpolationTest has 9 animations, all of them should be running";
}

TEST_F(RaCoApplicationFixture, importglTFScenegraphMeshWithNegativeScaleWillBeImported) {
	auto* commandInterface = application.activeRaCoProject().commandInterface();
	commandInterface->deleteObjects(application.activeRaCoProject().project()->instances());

	raco::core::MeshDescriptor desc;
	desc.absPath = cwd_path().append("meshes/negativeScaleQuad.gltf").string();
	desc.bakeAllSubmeshes = false;


	auto scenegraph = commandInterface->meshCache()->getMeshScenegraph(desc);
	commandInterface->insertAssetScenegraph(*scenegraph, desc.absPath, nullptr);
	auto node = raco::core::ValueHandle(raco::core::Queries::findByName(raco::core::Queries::filterForNotResource(commandInterface->project()->instances()), "Quad"));
	constexpr auto DELTA = 0.0001;

	ASSERT_NEAR(node.get("scale").get("x").asDouble(), 3.0, DELTA);
	ASSERT_NEAR(node.get("scale").get("y").asDouble(), -1.0, DELTA);
	ASSERT_NEAR(node.get("scale").get("z").asDouble(), 2.0, DELTA);
}

TEST_F(RaCoApplicationFixture, importglTFScenegraphCachedMeshPathGetsChanged) {
	auto* commandInterface = application.activeRaCoProject().commandInterface();
	commandInterface->deleteObjects(application.activeRaCoProject().project()->instances());
	raco::core::PathManager::setCachedPath(raco::core::PathManager::FolderTypeKeys::Mesh, "");

	raco::core::MeshDescriptor desc;
	desc.absPath = cwd_path().append("meshes/CesiumMilkTruck/CesiumMilkTruck.gltf").string();
	desc.bakeAllSubmeshes = false;


	auto scenegraph = commandInterface->meshCache()->getMeshScenegraph(desc);
	commandInterface->insertAssetScenegraph(*scenegraph, desc.absPath, nullptr);
	application.dataChangeDispatcher()->dispatch(*application.activeRaCoProject().recorder());

	ASSERT_EQ(raco::core::PathManager::getCachedPath(raco::core::PathManager::FolderTypeKeys::Mesh), (cwd_path() / "meshes/CesiumMilkTruck").generic_string());
}

TEST_F(RaCoApplicationFixture, importglTFScenegraphCorrectScenegraphStructureTruck) {
	auto* commandInterface = application.activeRaCoProject().commandInterface();
	commandInterface->deleteObjects(application.activeRaCoProject().project()->instances());

	raco::core::MeshDescriptor desc;
	desc.absPath = cwd_path().append("meshes/CesiumMilkTruck/CesiumMilkTruck.gltf").string();
	desc.bakeAllSubmeshes = false;


	auto scenegraph = commandInterface->meshCache()->getMeshScenegraph(desc);
	commandInterface->insertAssetScenegraph(*scenegraph, desc.absPath, nullptr);
	application.dataChangeDispatcher()->dispatch(*application.activeRaCoProject().recorder());

	// structure of scenegraph as currently imported by tinyGLTF:
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


	auto scenegraph = commandInterface->meshCache()->getMeshScenegraph(desc);
	commandInterface->insertAssetScenegraph(*scenegraph, desc.absPath, myRoot);

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


	auto scenegraph = commandInterface->meshCache()->getMeshScenegraph(desc);
	commandInterface->insertAssetScenegraph(*scenegraph, desc.absPath, nullptr);

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


	auto sceneGraph = application.activeRaCoProject().meshCache()->getMeshScenegraph(desc);
	commandInterface->insertAssetScenegraph(*sceneGraph, desc.absPath, firstRoot);
	commandInterface->insertAssetScenegraph(*sceneGraph, desc.absPath, secondRoot);

	//double amount of scenegraph nodes (see importglTFScenegraphCorrectNodeAmount) - 4 duplicate meshes - 2 duplicate AnimationChannels + 2 roots
	ASSERT_EQ(commandInterface->project()->instances().size(), 32);
}

TEST_F(RaCoApplicationFixture, importglTFScenegraphImportSceneGraphTwiceButMeshesGetChangedBeforeSecondImport) {
	auto* commandInterface = application.activeRaCoProject().commandInterface();
	commandInterface->deleteObjects(application.activeRaCoProject().project()->instances());
	auto firstRoot = commandInterface->createObject(raco::user_types::Node::typeDescription.typeName, "myRoot");
	auto secondRoot = commandInterface->createObject(raco::user_types::Node::typeDescription.typeName, "myRoot");

	raco::core::MeshDescriptor desc;
	desc.absPath = cwd_path().append("meshes/CesiumMilkTruck/CesiumMilkTruck.gltf").string();
	desc.bakeAllSubmeshes = false;


	auto sceneGraph = application.activeRaCoProject().meshCache()->getMeshScenegraph(desc);
	commandInterface->insertAssetScenegraph(*sceneGraph, desc.absPath, firstRoot);

	auto allMeshes = raco::core::Queries::filterByTypeName(commandInterface->project()->instances(), {raco::user_types::Mesh::typeDescription.typeName});
	commandInterface->set({allMeshes[0], {"uri"}}, std::string("blah"));

	commandInterface->set({allMeshes[1], {"bakeMeshes"}}, true);

	commandInterface->set({allMeshes[2], {"meshIndex"}}, 99);

	commandInterface->set({allMeshes[3], {"meshIndex"}}, 99);
	commandInterface->set({allMeshes[3], {"bakeMeshes"}}, true);
	commandInterface->set({allMeshes[3], {"uri"}}, std::string("blah"));

	commandInterface->insertAssetScenegraph(*sceneGraph, desc.absPath, secondRoot);

	//double amount of scenegraph nodes (see importglTFScenegraphCorrectNodeAmount), 8 meshes - 2 duplicate AnimationChannels + 2 roots
	ASSERT_EQ(commandInterface->project()->instances().size(), 36);
}

TEST_F(RaCoApplicationFixture, importglTFScenegraphImportSceneGraphTwiceButAnimationChannelsGetChangedBeforeSecondImport) {
	auto* commandInterface = application.activeRaCoProject().commandInterface();
	commandInterface->deleteObjects(application.activeRaCoProject().project()->instances());
	auto firstRoot = commandInterface->createObject(raco::user_types::Node::typeDescription.typeName, "myRoot");
	auto secondRoot = commandInterface->createObject(raco::user_types::Node::typeDescription.typeName, "myRoot");

	raco::core::MeshDescriptor desc;
	desc.absPath = cwd_path().append("meshes/CesiumMilkTruck/CesiumMilkTruck.gltf").string();
	desc.bakeAllSubmeshes = false;


	auto sceneGraph = application.activeRaCoProject().meshCache()->getMeshScenegraph(desc);
	commandInterface->insertAssetScenegraph(*sceneGraph, desc.absPath, firstRoot);

	auto allAnimChannels = raco::core::Queries::filterByTypeName(commandInterface->project()->instances(), {raco::user_types::AnimationChannel::typeDescription.typeName});
	commandInterface->set({allAnimChannels[0], {"animationIndex"}}, 2);

	commandInterface->set({allAnimChannels[1], {"samplerIndex"}}, 3);

	commandInterface->insertAssetScenegraph(*sceneGraph, desc.absPath, secondRoot);

	//double amount of scenegraph nodes (see importglTFScenegraphCorrectNodeAmount), 4 meshes + 2 roots
	ASSERT_EQ(commandInterface->project()->instances().size(), 34);
}

TEST_F(RaCoApplicationFixture, importglTFScenegraphUnbakedMeshesGetTransformed) {
	auto* commandInterface = application.activeRaCoProject().commandInterface();
	commandInterface->deleteObjects(application.activeRaCoProject().project()->instances());

	raco::core::MeshDescriptor desc;
	desc.absPath = cwd_path().append("meshes/CesiumMilkTruck/CesiumMilkTruck.gltf").string();
	desc.bakeAllSubmeshes = false;


	auto scenegraph = commandInterface->meshCache()->getMeshScenegraph(desc);
	commandInterface->insertAssetScenegraph(*scenegraph, desc.absPath, nullptr);
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


	auto scenegraph = commandInterface->meshCache()->getMeshScenegraph(desc);
	commandInterface->insertAssetScenegraph(*scenegraph, desc.absPath, nullptr);

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

TEST_F(RaCoApplicationFixture, importglTFScenegraphUnmarkedNodesDoNotGetImported) {
	auto* commandInterface = application.activeRaCoProject().commandInterface();
	commandInterface->deleteObjects(application.activeRaCoProject().project()->instances());

	raco::core::MeshDescriptor desc;
	desc.absPath = cwd_path().append("meshes/CesiumMilkTruck/CesiumMilkTruck.gltf").string();
	desc.bakeAllSubmeshes = false;


	auto scenegraph = commandInterface->meshCache()->getMeshScenegraph(desc);

	auto nodeToRemoveIndex = std::find_if(scenegraph->nodes.begin(), scenegraph->nodes.end(), [](const auto& node) { return node->name == "Node.001"; }) - scenegraph->nodes.begin();
	for (auto& node : scenegraph->nodes) {
			if (node->parentIndex == nodeToRemoveIndex) {
				node.reset();
			}
		}
	scenegraph->nodes[nodeToRemoveIndex].reset();

	commandInterface->insertAssetScenegraph(*scenegraph, desc.absPath, nullptr);

	// structure of modified scenegraph:
	// - CesiumMilkTruck.gltf
	// -- Yup2Zup
	// --- Cesium_Milk_Truck
	// ---- Cesium_Milk_Truck_meshnodes
	// ----- Cesium_Milk_Truck_meshnode_0
	// ----- Cesium_Milk_Truck_meshnode_1
	// ----- Cesium_Milk_Truck_meshnode_2
	// ---- Node
	// ----- Wheels
	// Deselected nodes:
	// ---- Node.001
	// ----- Wheels.001

	ASSERT_EQ(raco::core::Queries::findByName(application.activeRaCoProject().project()->instances(), "Node.001"), nullptr);
	ASSERT_EQ(raco::core::Queries::findByName(application.activeRaCoProject().project()->instances(), "Wheels.001"), nullptr);
	ASSERT_EQ(raco::core::Queries::findByName(application.activeRaCoProject().project()->instances(), "Cesium_Milk_Truck")->children_.asTable().size(), 2);	
}

TEST_F(RaCoApplicationFixture, importglTFScenegraphImportedAnimationDoesNotGetPartitioned) {
	auto* commandInterface = application.activeRaCoProject().commandInterface();
	commandInterface->deleteObjects(application.activeRaCoProject().project()->instances());

	raco::core::MeshDescriptor desc;
	desc.absPath = cwd_path().append("meshes/RiggedFigure/RiggedFigure.gltf").string();
	desc.bakeAllSubmeshes = false;


	auto scenegraph = commandInterface->meshCache()->getMeshScenegraph(desc);

	commandInterface->insertAssetScenegraph(*scenegraph, desc.absPath, nullptr);

	auto allAnims = raco::core::Queries::filterByTypeName(application.activeRaCoProject().project()->instances(), {raco::user_types::Animation::typeDescription.typeName});

	// 57 AnimationChannels in 1 Animation object
	ASSERT_EQ(allAnims.size(), 1);
	ASSERT_EQ(allAnims.front()->get("animationChannels")->asTable().size(), 57);
}

TEST_F(RaCoApplicationFixture, importglTFScenegraphDeselectedAnimationsDoNotGetImported) {
	auto* commandInterface = application.activeRaCoProject().commandInterface();
	commandInterface->deleteObjects(application.activeRaCoProject().project()->instances());

	raco::core::MeshDescriptor desc;
	desc.absPath = cwd_path().append("meshes/CesiumMilkTruck/CesiumMilkTruck.gltf").string();
	desc.bakeAllSubmeshes = false;


	auto scenegraph = commandInterface->meshCache()->getMeshScenegraph(desc);

	scenegraph->animations.front().reset();

	commandInterface->insertAssetScenegraph(*scenegraph, desc.absPath, nullptr);

	auto allAnims = raco::core::Queries::filterByTypeName(application.activeRaCoProject().project()->instances(), {raco::user_types::Animation::typeDescription.typeName});
	ASSERT_TRUE(allAnims.empty());
	ASSERT_TRUE(application.activeRaCoProject().project()->links().empty());
}


TEST_F(RaCoApplicationFixture, importglTFScenegraphDeselectedNodesWillNotCreateLinksWithAnimations) {
	auto* commandInterface = application.activeRaCoProject().commandInterface();
	commandInterface->deleteObjects(application.activeRaCoProject().project()->instances());

	raco::core::MeshDescriptor desc;
	desc.absPath = cwd_path().append("meshes/CesiumMilkTruck/CesiumMilkTruck.gltf").string();
	desc.bakeAllSubmeshes = false;


	auto scenegraph = commandInterface->meshCache()->getMeshScenegraph(desc);

	for (auto& node : scenegraph->nodes) {
		node.reset();
	}

	commandInterface->insertAssetScenegraph(*scenegraph, desc.absPath, nullptr);

	ASSERT_TRUE(application.activeRaCoProject().project()->links().empty());
}

TEST_F(RaCoApplicationFixture, importglTFScenegraphDeselectedAnimationChannelsDoNotGetImported) {
	auto* commandInterface = application.activeRaCoProject().commandInterface();
	commandInterface->deleteObjects(application.activeRaCoProject().project()->instances());

	raco::core::MeshDescriptor desc;
	desc.absPath = cwd_path().append("meshes/CesiumMilkTruck/CesiumMilkTruck.gltf").string();
	desc.bakeAllSubmeshes = false;


	auto scenegraph = commandInterface->meshCache()->getMeshScenegraph(desc);

	for (auto& samplers : scenegraph->animationSamplers) {
		for (auto& sampler : samplers) {
			sampler.reset();
		}
	}

	commandInterface->insertAssetScenegraph(*scenegraph, desc.absPath, nullptr);

	auto allAnimChannels = raco::core::Queries::filterByTypeName(application.activeRaCoProject().project()->instances(), {raco::user_types::AnimationChannel::typeDescription.typeName});
	ASSERT_TRUE(allAnimChannels.empty());

	auto allAnims = raco::core::Queries::filterByTypeName(application.activeRaCoProject().project()->instances(), {raco::user_types::Animation::typeDescription.typeName});
	ASSERT_EQ(allAnims.size(), 1);

	auto& channels = allAnims.front()->get("animationChannels")->asTable();
	for (auto i = 0; i < channels.size(); ++i) {
		ASSERT_EQ(channels[i]->asRef(), nullptr);
	}
	ASSERT_TRUE(application.activeRaCoProject().project()->links().empty());
}

TEST_F(RaCoApplicationFixture, importglTFScenegraphWrongFileReturnsEmptyScenegraph) {
	auto* commandInterface = application.activeRaCoProject().commandInterface();
	commandInterface->deleteObjects(application.activeRaCoProject().project()->instances());

	raco::core::MeshDescriptor desc;
	desc.absPath = cwd_path().append("nonexistentFile.gltf").string();
	desc.bakeAllSubmeshes = false;

	auto scenegraph = commandInterface->meshCache()->getMeshScenegraph(desc);

	ASSERT_EQ(scenegraph, nullptr);
}

TEST_F(RaCoApplicationFixture, importglTFScenegraphWrongFileReturnsEmptyAnimSampler) {
	auto* commandInterface = application.activeRaCoProject().commandInterface();
	commandInterface->deleteObjects(application.activeRaCoProject().project()->instances());

	auto animSampler = commandInterface->meshCache()->getAnimationSamplerData(cwd_path().append("nonexistentFile.gltf").string(), 0, 0);

	ASSERT_EQ(animSampler, nullptr);
}

TEST_F(RaCoApplicationFixture, importglTFScenegraphMeshNodesDontReferenceDeselectedMeshes) {
	auto* commandInterface = application.activeRaCoProject().commandInterface();
	commandInterface->deleteObjects(application.activeRaCoProject().project()->instances());

	raco::core::MeshDescriptor desc;
	desc.absPath = cwd_path().append("meshes/CesiumMilkTruck/CesiumMilkTruck.gltf").string();
	desc.bakeAllSubmeshes = false;

	auto scenegraph = commandInterface->meshCache()->getMeshScenegraph(desc);

	auto meshToRemoveIndex = std::find_if(scenegraph->meshes.begin(), scenegraph->meshes.end(), [](const auto& meshName) { return meshName.value() == "Cesium_Milk_Truck.0"; }) - scenegraph->meshes.begin();
	scenegraph->meshes[meshToRemoveIndex].reset();

	for (auto& node : scenegraph->nodes) {
		for (auto& index : node->subMeshIndeces) {
			if (index == meshToRemoveIndex) {
				index = -1;
			}
		}
	}

	commandInterface->insertAssetScenegraph(*scenegraph, desc.absPath, nullptr);

	auto allMeshNodes = raco::core::Queries::filterByTypeName(application.activeRaCoProject().project()->instances(), {raco::user_types::MeshNode::typeDescription.typeName});
	ASSERT_EQ(allMeshNodes.size(), 5);

	for (const auto& meshNode : allMeshNodes) {
		auto referencingMesh = raco::core::ValueHandle(meshNode, {"mesh"}).asRef();
		if (meshNode->objectName() == "Cesium_Milk_Truck_meshnode_0") {
			ASSERT_EQ(referencingMesh, nullptr);
		} else {
			ASSERT_NE(referencingMesh, nullptr);
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