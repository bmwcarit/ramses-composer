/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "object_tree_view_model/ObjectTreeViewExternalProjectModel.h"

#include "ObjectTreeViewDefaultModel_test.h"

#include "application/RaCoApplication.h"

#include "ramses_adaptor/SceneBackend.h"
#include "ramses_base/BaseEngineBackend.h"

#include "user_types/Node.h"

#include "core/Queries.h"


using namespace raco::object_tree;

class ExposedObjectTreeViewExternalProjectModel : public model::ObjectTreeViewExternalProjectModel {
public:
	void triggerObjectTreeRebuilding() {
		buildObjectTree();
	}

	ExposedObjectTreeViewExternalProjectModel(raco::application::RaCoApplication &app)
		: ObjectTreeViewExternalProjectModel(app.activeRaCoProject().commandInterface(), app.activeRaCoProject().fileChangeMonitor(), app.dataChangeDispatcher(), app.externalProjects()) {}

	model::ObjectTreeNode *getInvisibleRootNode() {
		return invisibleRootNode_.get();
	}

	std::unordered_map<std::string, QModelIndex> indexes() {
		return indexes_;
	}
};


class ObjectTreeViewExternalProjectModelTest : public ObjectTreeViewDefaultModelTest {
protected:
	static inline const char* PROJECT_PATH = "projectPath.rca";

	static inline int NEW_PROJECT_NODE_AMOUNT{0};

	raco::ramses_base::HeadlessEngineBackend backend{};
	raco::ramses_base::HeadlessEngineBackend otherBackend{};
	raco::application::RaCoApplication application{backend};
	raco::application::RaCoApplication otherApplication{otherBackend};
	ExposedObjectTreeViewExternalProjectModel externalProjectModel{application};

	void generateExternalProject(const std::vector<raco::core::SEditorObject> &instances) {
		application.externalProjectsStore_.externalProjects_[PROJECT_PATH] = raco::application::RaCoProject::createNew(&otherApplication);
		NEW_PROJECT_NODE_AMOUNT = raco::core::Queries::filterForVisibleObjects(application.externalProjectsStore_.externalProjects_[PROJECT_PATH]->project()->instances()).size();

		for (const auto &instance : instances) {
			application.externalProjectsStore_.externalProjects_[PROJECT_PATH]->project()->addInstance(instance);
		}
	}
};


TEST_F(ObjectTreeViewExternalProjectModelTest, InstantiationNoLocalInstancesInModel) {
	commandInterface.createObject(raco::user_types::Node::typeDescription.typeName);
	externalProjectModel.triggerObjectTreeRebuilding();
	ASSERT_TRUE(externalProjectModel.indexes().empty());
}

TEST_F(ObjectTreeViewExternalProjectModelTest, LoadingProjectEmpty) {
	generateExternalProject({});

	externalProjectModel.triggerObjectTreeRebuilding();

	auto rootNode = externalProjectModel.getInvisibleRootNode();
	ASSERT_EQ(rootNode->childCount(), 1);
	ASSERT_EQ(rootNode->getChildren().front()->getRepresentedObject()->objectName(), PROJECT_PATH);
	ASSERT_EQ(rootNode->getChildren().front()->childCount(), NEW_PROJECT_NODE_AMOUNT);
}

TEST_F(ObjectTreeViewExternalProjectModelTest, LoadingProjectTenTopLevelNodes) {
	constexpr size_t CHILD_NODE_AMOUNT = 10;

	std::vector<raco::core::SEditorObject> instances;
	for (size_t i{0}; i < CHILD_NODE_AMOUNT; ++i) {
		instances.emplace_back(std::make_shared<raco::user_types::Node>());
	}

	generateExternalProject(instances);
	externalProjectModel.triggerObjectTreeRebuilding();

	auto rootNode = externalProjectModel.getInvisibleRootNode();
	ASSERT_EQ(rootNode->childCount(), 1);
	ASSERT_EQ(rootNode->getChildren().front()->childCount(), CHILD_NODE_AMOUNT + NEW_PROJECT_NODE_AMOUNT);
}

TEST_F(ObjectTreeViewExternalProjectModelTest, LoadingProjectHierarchyParentsCreatedFirst) {
	constexpr size_t NODE_AMOUNT = 5;
	std::vector<raco::core::SEditorObject> instances;

	for (size_t i{0}; i < NODE_AMOUNT; ++i) {
		instances.emplace_back(otherApplication.activeRaCoProject().commandInterface()->createObject(raco::user_types::Node::typeDescription.typeName, "node_" + std::to_string(i)));
	}

	for (size_t i{0}; i < NODE_AMOUNT - 1; ++i) {
		otherApplication.activeRaCoProject().commandInterface()->moveScenegraphChild(instances[i + 1], instances[i]);
	}

	generateExternalProject(instances);
	externalProjectModel.triggerObjectTreeRebuilding();

	auto externalProjectRootNode = externalProjectModel.getInvisibleRootNode()->getChild(0);
	auto hierarchyLevel = 0;

	for (auto treeItem = externalProjectRootNode->getChildren().back(); treeItem->childCount() > 0; treeItem = treeItem->getChild(0)) {
		ASSERT_EQ(treeItem->getRepresentedObject()->objectName(), "node_" + std::to_string(hierarchyLevel));
		++hierarchyLevel;
	}
	ASSERT_EQ(hierarchyLevel, NODE_AMOUNT - 1);
}

TEST_F(ObjectTreeViewExternalProjectModelTest, LoadingProjectHierarchyChildrenCreatedFirst) {
	constexpr size_t NODE_AMOUNT = 5;
	std::vector<raco::core::SEditorObject> instances;

	for (int i{NODE_AMOUNT - 1}; i >= 0; --i) {
		instances.emplace_back(otherApplication.activeRaCoProject().commandInterface()->createObject(raco::user_types::Node::typeDescription.typeName, "node_" + std::to_string(i)));
	}

	for (size_t i{1}; i < NODE_AMOUNT; ++i) {
		otherApplication.activeRaCoProject().commandInterface()->moveScenegraphChild(instances[i - 1], instances[i]);
	}
	
	generateExternalProject(instances);
	externalProjectModel.triggerObjectTreeRebuilding();

	auto externalProjectRootNode = externalProjectModel.getInvisibleRootNode()->getChild(0);
	auto hierarchyLevel = 0;

	for (auto treeItem = externalProjectRootNode->getChildren().back(); treeItem->childCount() > 0; treeItem = treeItem->getChild(0)) {
		ASSERT_EQ(treeItem->getRepresentedObject()->objectName(), "node_" + std::to_string(hierarchyLevel));
		++hierarchyLevel;
	}
	ASSERT_EQ(hierarchyLevel, NODE_AMOUNT - 1);
}

TEST_F(ObjectTreeViewExternalProjectModelTest, LoadingProjectHierarchyRetainChildrenOrder) {
	constexpr size_t CHILD_NODE_AMOUNT = 3;
	std::vector<raco::core::SEditorObject> instances;

	auto rootNode = instances.emplace_back(otherApplication.activeRaCoProject().commandInterface()->createObject(raco::user_types::Node::typeDescription.typeName, "myRoot"));

	for (size_t i{0}; i < CHILD_NODE_AMOUNT; ++i) {
		auto childNode = instances.emplace_back(otherApplication.activeRaCoProject().commandInterface()->createObject(raco::user_types::Node::typeDescription.typeName, "node_" + std::to_string(i)));
		otherApplication.activeRaCoProject().commandInterface()->moveScenegraphChild(childNode, rootNode, 0);
	}

	generateExternalProject(instances);
	externalProjectModel.triggerObjectTreeRebuilding();

	auto externalProjectRootNode = externalProjectModel.getInvisibleRootNode()->getChild(0);
	auto ourRootNode = externalProjectRootNode->getChildren().back();
	ASSERT_EQ(ourRootNode->childCount(), CHILD_NODE_AMOUNT);
	ASSERT_EQ(ourRootNode->getChild(0)->getRepresentedObject()->objectName(), "node_2");
	ASSERT_EQ(ourRootNode->getChild(1)->getRepresentedObject()->objectName(), "node_1");
	ASSERT_EQ(ourRootNode->getChild(2)->getRepresentedObject()->objectName(), "node_0");
}