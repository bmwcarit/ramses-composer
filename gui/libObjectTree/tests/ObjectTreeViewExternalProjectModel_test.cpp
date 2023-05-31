/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
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
		: ObjectTreeViewExternalProjectModel(app.activeRaCoProject().commandInterface(), app.dataChangeDispatcher(), app.externalProjects()) {}

	model::ObjectTreeNode *getInvisibleRootNode() {
		return invisibleRootNode_.get();
	}

	std::unordered_map<std::string, QModelIndex> indexes() {
		return indexes_;
	}
};


class ObjectTreeViewExternalProjectModelTest : public ObjectTreeViewDefaultModelTest {
protected:

	raco::ramses_base::HeadlessEngineBackend otherBackend{raco::ramses_base::BaseEngineBackend::maxFeatureLevel};
	raco::application::RaCoApplication otherApplication{otherBackend};
	ExposedObjectTreeViewExternalProjectModel externalProjectModel{application};

	void generateExternalProject(const std::vector<raco::core::SEditorObject> &instances, std::string projectPath = "projectPath.rca") {
		application.externalProjectsStore_.externalProjects_[projectPath] = raco::application::RaCoProject::createNew(&otherApplication, true, static_cast<int>(raco::ramses_base::BaseEngineBackend::maxFeatureLevel));

		auto project = application.externalProjectsStore_.externalProjects_[projectPath]->project();
		for (const auto &instance : instances) {
			project->addInstance(instance);
		}
	}
};


TEST_F(ObjectTreeViewExternalProjectModelTest, InstantiationNoLocalInstancesInModel) {
	commandInterface().createObject(raco::user_types::Node::typeDescription.typeName);
	externalProjectModel.triggerObjectTreeRebuilding();
	ASSERT_TRUE(externalProjectModel.indexes().empty());
}

TEST_F(ObjectTreeViewExternalProjectModelTest, LoadingProjectEmpty) {
	auto projectPath = "projectPath.rca";
	generateExternalProject({}, projectPath);

	externalProjectModel.triggerObjectTreeRebuilding();

	auto rootNode = externalProjectModel.getInvisibleRootNode();
	ASSERT_EQ(rootNode->childCount(), 1);
	ASSERT_EQ(rootNode->getChildren().front()->getExternalProjectPath(), projectPath);
	ASSERT_EQ(rootNode->getChildren().front()->childCount(), 4);
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
	ASSERT_EQ(rootNode->getChildren().front()->childCount(), CHILD_NODE_AMOUNT + 4);
}

TEST_F(ObjectTreeViewExternalProjectModelTest, LoadingProjectHierarchyParentsCreatedFirst) {
	constexpr size_t NODE_AMOUNT = 5;
	std::vector<raco::core::SEditorObject> instances;

	for (size_t i{0}; i < NODE_AMOUNT; ++i) {
		instances.emplace_back(otherApplication.activeRaCoProject().commandInterface()->createObject(raco::user_types::Node::typeDescription.typeName, "node_" + std::to_string(i)));
	}

	for (size_t i{0}; i < NODE_AMOUNT - 1; ++i) {
		otherApplication.activeRaCoProject().commandInterface()->moveScenegraphChildren({instances[i + 1]}, instances[i]);
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
		otherApplication.activeRaCoProject().commandInterface()->moveScenegraphChildren({instances[i - 1]}, instances[i]);
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
		otherApplication.activeRaCoProject().commandInterface()->moveScenegraphChildren({childNode}, rootNode, 0);
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

TEST_F(ObjectTreeViewExternalProjectModelTest, CanCopyAtIndicesMultiSelection) {
	std::string project1Path = "project1Path.rca";
	auto project1Node = std::make_shared<raco::user_types::Node>();
	generateExternalProject({project1Node}, project1Path);

	auto project2Node = std::make_shared<raco::user_types::Node>();
	std::string project2Path = "project2Path.rca";
	generateExternalProject({project2Node}, project2Path);

	externalProjectModel.triggerObjectTreeRebuilding();

	auto project1Index = externalProjectModel.indexFromTreeNodeID(project1Path);
	auto project1NodeIndex = externalProjectModel.indexFromTreeNodeID(project1Node->objectID());
	auto project2Index = externalProjectModel.indexFromTreeNodeID(project2Path);
	auto project2NodeIndex = externalProjectModel.indexFromTreeNodeID(project2Node->objectID());

	ASSERT_TRUE(project1Index.isValid());
	ASSERT_TRUE(project1NodeIndex.isValid());
	ASSERT_TRUE(project2Index.isValid());
	ASSERT_TRUE(project2NodeIndex.isValid());

	ASSERT_FALSE(externalProjectModel.canCopyAtIndices({}));
	ASSERT_FALSE(externalProjectModel.canCopyAtIndices({project1Index}));
	ASSERT_FALSE(externalProjectModel.canCopyAtIndices({project2Index}));
	ASSERT_FALSE(externalProjectModel.canCopyAtIndices({project1Index, project2Index}));

	ASSERT_TRUE(externalProjectModel.canCopyAtIndices({project1NodeIndex}));
	ASSERT_TRUE(externalProjectModel.canCopyAtIndices({project1Index, project1NodeIndex}));
	ASSERT_TRUE(externalProjectModel.canCopyAtIndices({project1NodeIndex, {}}));
	ASSERT_TRUE(externalProjectModel.canCopyAtIndices({project2NodeIndex}));
	ASSERT_TRUE(externalProjectModel.canCopyAtIndices({project2Index, project2NodeIndex}));
	ASSERT_TRUE(externalProjectModel.canCopyAtIndices({project2NodeIndex, {}}));

	ASSERT_FALSE(externalProjectModel.canCopyAtIndices({project1NodeIndex, project2NodeIndex}));
}

TEST_F(ObjectTreeViewExternalProjectModelTest, CanDeleteAtIndicesNever) {
	std::string project1Path = "project1Path.rca";
	auto project1Node = std::make_shared<raco::user_types::Node>();
	generateExternalProject({project1Node}, project1Path);

	externalProjectModel.triggerObjectTreeRebuilding();

	auto project1Index = externalProjectModel.indexFromTreeNodeID(project1Path);
	auto project1NodeIndex = externalProjectModel.indexFromTreeNodeID(project1Node->objectID());

	ASSERT_FALSE(externalProjectModel.canDeleteAtIndices({}));
	ASSERT_FALSE(externalProjectModel.canDeleteAtIndices({{}}));
	ASSERT_FALSE(externalProjectModel.canDeleteAtIndices({project1Index}));
	ASSERT_FALSE(externalProjectModel.canDeleteAtIndices({project1NodeIndex}));
	ASSERT_FALSE(externalProjectModel.canDeleteAtIndices({project1Index, project1NodeIndex, {}}));
}

TEST_F(ObjectTreeViewExternalProjectModelTest, CanNeverDuplicate) {
	std::string project1Path = "project1Path.rca";
	auto project1Node = std::make_shared<raco::user_types::Node>();
	generateExternalProject({project1Node}, project1Path);

	externalProjectModel.triggerObjectTreeRebuilding();

	auto project1Index = externalProjectModel.indexFromTreeNodeID(project1Path);
	auto project1NodeIndex = externalProjectModel.indexFromTreeNodeID(project1Node->objectID());

	ASSERT_FALSE(externalProjectModel.canDuplicateAtIndices({}));
	ASSERT_FALSE(externalProjectModel.canDuplicateAtIndices({{}}));
	ASSERT_FALSE(externalProjectModel.canDuplicateAtIndices({project1Index}));
	ASSERT_FALSE(externalProjectModel.canDuplicateAtIndices({project1NodeIndex}));
	ASSERT_FALSE(externalProjectModel.canDuplicateAtIndices({project1Index, project1NodeIndex, {}}));
}
