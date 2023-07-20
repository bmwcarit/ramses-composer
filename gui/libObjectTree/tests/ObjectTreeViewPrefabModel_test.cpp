/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "gtest/gtest.h"

#include "ObjectTreeViewDefaultModel_test.h"
#include "core/ExternalReferenceAnnotation.h"
#include "core/Queries.h"
#include "object_tree_view_model/ObjectTreeViewPrefabModel.h"
#include "user_types/LuaInterface.h"
#include "user_types/Skin.h"

using namespace raco::user_types;

class ObjectTreeViewPrefabModelTest : public ObjectTreeViewDefaultModelTest {
public:
	ObjectTreeViewPrefabModelTest() : ObjectTreeViewDefaultModelTest() {
		viewModel_.reset(new raco::object_tree::model::ObjectTreeViewPrefabModel(&commandInterface(), application.dataChangeDispatcher(), externalProjectStore(),
			{
				Prefab::typeDescription.typeName
			}));
	}
};

TEST_F(ObjectTreeViewPrefabModelTest, TypesAllowedIntoIndexEmptyIndex) {
	auto allowedTypes = viewModel_->typesAllowedIntoIndex({});
	std::vector<std::string> allowedTypesAssert{Prefab::typeDescription.typeName};

	EXPECT_EQ(allowedTypes, allowedTypesAssert);
}

TEST_F(ObjectTreeViewPrefabModelTest, TypesAllowedIntoIndexInvalidParent) {
	auto prefab = createNodes(Prefab::typeDescription.typeName, {"prefab"}).front();
	auto prefabInstance = createNodes(PrefabInstance::typeDescription.typeName, {"prefabInstance"}).front();
	auto luaScript = createNodes(LuaScript::typeDescription.typeName, {"luaScript"}).front();
	auto animation = createNodes(Animation::typeDescription.typeName, {"animation"}).front();
	moveScenegraphChildren({prefabInstance}, prefab);
	moveScenegraphChildren({luaScript}, prefab);
	moveScenegraphChildren({animation}, prefab);

	ASSERT_TRUE(viewModel_->typesAllowedIntoIndex(viewModel_->indexFromTreeNodeID(prefabInstance->objectID())).empty());
	ASSERT_TRUE(viewModel_->typesAllowedIntoIndex(viewModel_->indexFromTreeNodeID(luaScript->objectID())).empty());
	ASSERT_TRUE(viewModel_->typesAllowedIntoIndex(viewModel_->indexFromTreeNodeID(animation->objectID())).empty());
}

TEST_F(ObjectTreeViewPrefabModelTest, TypesAllowedIntoIndexNode) {
	auto prefab = createNodes(Prefab::typeDescription.typeName, {"prefab"}).front();
	auto node = createNodes(Node::typeDescription.typeName, {"node"}).front();
	moveScenegraphChildren({node}, prefab);

	auto allowedTypes = viewModel_->typesAllowedIntoIndex(viewModel_->indexFromTreeNodeID(node->objectID()));
	std::vector<std::string> allowedTypesAssert{Animation::typeDescription.typeName,
		LuaInterface::typeDescription.typeName,
		LuaScript::typeDescription.typeName,
		MeshNode::typeDescription.typeName,
		Node::typeDescription.typeName,
		OrthographicCamera::typeDescription.typeName,
		PerspectiveCamera::typeDescription.typeName,
		PrefabInstance::typeDescription.typeName,
		Skin::typeDescription.typeName};

	EXPECT_EQ(allowedTypes, allowedTypesAssert);
}

TEST_F(ObjectTreeViewPrefabModelTest, AllowedObjsResourcesAreNotAllowedOnTopLevel) {
	for (const auto &typeName : getTypes()) {
		auto newObj = commandInterface().createObject(typeName);
		if (raco::core::Queries::isResource(newObj)) {
			ASSERT_FALSE(viewModel_->isObjectAllowedIntoIndex({}, newObj));
		}
	}
}

TEST_F(ObjectTreeViewPrefabModelTest, AllowedObjsResourcesAreNotAllowedUnderPrefab) {
	auto prefab = createNodes(Prefab::typeDescription.typeName, {Prefab::typeDescription.typeName}).front();

	for (const auto &typeName : getTypes()) {
		auto newObj = commandInterface().createObject(typeName);
		if (raco::core::Queries::isResource(newObj)) {
			ASSERT_FALSE(viewModel_->isObjectAllowedIntoIndex(viewModel_->indexFromTreeNodeID(prefab->objectID()), newObj));
		}
	}
}

TEST_F(ObjectTreeViewPrefabModelTest, AllowedObjsCheckSceneGraphObjectsOnTopLevel) {
	for (const auto &typeName : getTypes()) {
		auto newObj = commandInterface().createObject(typeName);
		if (!raco::core::Queries::isResource(newObj)) {
			if (typeName == Prefab::typeDescription.typeName) {
				ASSERT_TRUE(viewModel_->isObjectAllowedIntoIndex({}, newObj));
			} else {
				ASSERT_FALSE(viewModel_->isObjectAllowedIntoIndex({}, newObj));
			}
		}
	}
}

TEST_F(ObjectTreeViewPrefabModelTest, AllowedObjsCheckExternalSceneGraphObjectsUnderPrefab) {
	auto prefab = createNodes(Prefab::typeDescription.typeName, {Prefab::typeDescription.typeName}).front();
	auto prefabIndex = viewModel_->indexFromTreeNodeID(prefab->objectID());

	for (const auto &typeName : getTypes()) {
		auto newObj = commandInterface().createObject(typeName);
		if (!raco::core::Queries::isResource(newObj) && !raco::core::Queries::isProjectSettings(newObj)) {
			if (newObj->as<Prefab>()) {
				ASSERT_FALSE(viewModel_->isObjectAllowedIntoIndex(prefabIndex, newObj));
			} else {
				ASSERT_TRUE(viewModel_->isObjectAllowedIntoIndex(prefabIndex, newObj));
			}
		}
	}
}

TEST_F(ObjectTreeViewPrefabModelTest, AllowedObjsPrefabInTreeViewIsNotMovable) {
	auto prefabs = createNodes(Prefab::typeDescription.typeName, {"prefab1", "prefab2"});
	auto prefabIndex = viewModel_->indexFromTreeNodeID(prefabs[0]->objectID());
	ASSERT_TRUE(prefabIndex.isValid());

	ASSERT_FALSE(viewModel_->isObjectAllowedIntoIndex(prefabIndex, prefabs[1]));
}

TEST_F(ObjectTreeViewPrefabModelTest, AllowedObjsDeepCopiedSceneGraphWithResourcesIsNotAllowed) {
	auto meshNode = createNodes(MeshNode::typeDescription.typeName, {MeshNode::typeDescription.typeName}).front();
	auto mesh = createNodes(Mesh::typeDescription.typeName, {Mesh::typeDescription.typeName}).front();

	commandInterface().set(raco::core::ValueHandle{meshNode, {"mesh"}}, mesh);
	dispatch();

	auto cutObjs = commandInterface().cutObjects({meshNode}, true);
	dispatch();

	auto [parsedObjs, sourceProjectTopLevelObjectIds] = viewModel_->getObjectsAndRootIdsFromClipboardString(cutObjs);
	ASSERT_FALSE(viewModel_->canPasteIntoIndex({}, parsedObjs, sourceProjectTopLevelObjectIds));
}

TEST_F(ObjectTreeViewPrefabModelTest, AllowedObjsDeepCopiedSceneGraphWithResourcesAllowedUnderPrefab) {
	auto meshNode = createNodes(MeshNode::typeDescription.typeName, {MeshNode::typeDescription.typeName}).front();
	auto mesh = createNodes(Mesh::typeDescription.typeName, {Mesh::typeDescription.typeName}).front();
	auto prefab = createNodes(Prefab::typeDescription.typeName, {Prefab::typeDescription.typeName}).front();

	commandInterface().set(raco::core::ValueHandle{meshNode, {"mesh"}}, mesh);
	dispatch();
	
	auto copiedObjs = commandInterface().copyObjects({meshNode}, true);
	dispatch();

	auto [parsedObjs, sourceProjectTopLevelObjectIds] = viewModel_->getObjectsAndRootIdsFromClipboardString(copiedObjs);
	ASSERT_TRUE(viewModel_->canPasteIntoIndex(viewModel_->indexFromTreeNodeID(prefab->objectID()), parsedObjs, sourceProjectTopLevelObjectIds));
}

TEST_F(ObjectTreeViewPrefabModelTest, AllowedObjsDeepCopiedPrefabInstanceWithPrefabIsAllowed) {
	auto prefabInstance = createNodes(PrefabInstance::typeDescription.typeName, {PrefabInstance::typeDescription.typeName}).front();
	auto prefab = createNodes(Prefab::typeDescription.typeName, {Prefab::typeDescription.typeName}).front();

	commandInterface().set(raco::core::ValueHandle{prefabInstance, {"template"}}, prefab);
	dispatch();

	auto cutObjs = commandInterface().cutObjects({prefabInstance}, true);
	dispatch();

	auto [parsedObjs, sourceProjectTopLevelObjectIds] = viewModel_->getObjectsAndRootIdsFromClipboardString(cutObjs);
	ASSERT_TRUE(viewModel_->canPasteIntoIndex({}, parsedObjs, sourceProjectTopLevelObjectIds));
}

TEST_F(ObjectTreeViewPrefabModelTest, AllowedObjsDeepCopiedPrefabIsAllowedInEmptySpace) {
	auto meshNode = createNodes(MeshNode::typeDescription.typeName, {MeshNode::typeDescription.typeName}).front();
	auto mesh = createNodes(Mesh::typeDescription.typeName, {Mesh::typeDescription.typeName}).front();
	auto prefab = createNodes(Prefab::typeDescription.typeName, {Prefab::typeDescription.typeName}).front();

	commandInterface().set(raco::core::ValueHandle{meshNode, {"mesh"}}, mesh);
	dispatch();

	commandInterface().moveScenegraphChildren({meshNode}, prefab);
	dispatch();

	auto copiedObjs = commandInterface().copyObjects({prefab}, true);
	dispatch();

	auto [parsedObjs, sourceProjectTopLevelObjectIds] = viewModel_->getObjectsAndRootIdsFromClipboardString(copiedObjs);
	ASSERT_TRUE(viewModel_->canPasteIntoIndex({}, parsedObjs, sourceProjectTopLevelObjectIds));
}

TEST_F(ObjectTreeViewPrefabModelTest, AllowedObjsDeepCopiedPrefabIsNotAllowedUnderPrefab) {
	auto meshNode = createNodes(MeshNode::typeDescription.typeName, {MeshNode::typeDescription.typeName}).front();
	auto mesh = createNodes(Mesh::typeDescription.typeName, {Mesh::typeDescription.typeName}).front();
	auto prefabs = createNodes(Prefab::typeDescription.typeName, {"prefab", "prefab2"});

	commandInterface().set(raco::core::ValueHandle{meshNode, {"mesh"}}, mesh);
	dispatch();

	commandInterface().moveScenegraphChildren({meshNode}, prefabs.front());
	dispatch();

	auto copiedObjs = commandInterface().copyObjects({prefabs.front()}, true);
	dispatch();

	auto [parsedObjs, sourceProjectTopLevelObjectIds] = viewModel_->getObjectsAndRootIdsFromClipboardString(copiedObjs);
	ASSERT_FALSE(viewModel_->canPasteIntoIndex(viewModel_->indexFromTreeNodeID(prefabs[1]->objectID()), parsedObjs, sourceProjectTopLevelObjectIds));
}

TEST_F(ObjectTreeViewPrefabModelTest, AllowedObjsNothingIsAllowedUnderExtRef) {
	auto extRefPrefab = createNodes(Prefab::typeDescription.typeName, {Prefab::typeDescription.typeName}).front();
	extRefPrefab->addAnnotation(std::make_shared<raco::core::ExternalReferenceAnnotation>("differentProject"));

	viewModel_->buildObjectTree();
	auto extRefPrefabIndex = viewModel_->indexFromTreeNodeID(extRefPrefab->objectID());

	for (const auto &typeName : getTypes()) {
		auto newObj = commandInterface().createObject(typeName);
		ASSERT_FALSE(viewModel_->isObjectAllowedIntoIndex(extRefPrefabIndex, newObj));
	}

	auto extRefGroupIndex = viewModel_->index(0, 0);
	ASSERT_EQ(viewModel_->indexToTreeNode(extRefGroupIndex)->getType(), raco::object_tree::model::ObjectTreeNodeType::ExtRefGroup);
	ASSERT_FALSE(viewModel_->isObjectAllowedIntoIndex(extRefGroupIndex, commandInterface().createObject(Node::typeDescription.typeName)));
	ASSERT_TRUE(viewModel_->isObjectAllowedIntoIndex(extRefGroupIndex, commandInterface().createObject(Prefab::typeDescription.typeName)));
}

TEST_F(ObjectTreeViewPrefabModelTest, CanNotDoAnythingButPasteWithExtRefGroup) {
	auto extRefPrefab = createNodes(Prefab::typeDescription.typeName, {Prefab::typeDescription.typeName}).front();
	extRefPrefab->addAnnotation(std::make_shared<raco::core::ExternalReferenceAnnotation>("differentProject"));

	viewModel_->buildObjectTree();
	auto extRefGroupIndex = viewModel_->index(0, 0);
	ASSERT_EQ(viewModel_->indexToTreeNode(extRefGroupIndex)->getType(), raco::object_tree::model::ObjectTreeNodeType::ExtRefGroup);

	ASSERT_FALSE(viewModel_->canDeleteAtIndices({extRefGroupIndex}));
	ASSERT_FALSE(viewModel_->canCopyAtIndices({extRefGroupIndex}));
	ASSERT_FALSE(viewModel_->canDuplicateAtIndices({extRefGroupIndex}));

	dispatch();
	auto copiedObjs = commandInterface().copyObjects({extRefPrefab});
	dispatch();

	auto [parsedObjs, sourceProjectTopLevelObjectIds] = viewModel_->getObjectsAndRootIdsFromClipboardString(copiedObjs);
	ASSERT_TRUE(viewModel_->canPasteIntoIndex({}, parsedObjs, sourceProjectTopLevelObjectIds));
	ASSERT_TRUE(viewModel_->canPasteIntoIndex(extRefGroupIndex, parsedObjs, sourceProjectTopLevelObjectIds));
}

TEST_F(ObjectTreeViewPrefabModelTest, DuplicationPrefabCanBeDuplicated) {
	auto prefab = createNodes(Prefab::typeDescription.typeName, {Prefab::typeDescription.typeName}).front();

	ASSERT_TRUE(viewModel_->canDuplicateAtIndices({viewModel_->indexFromTreeNodeID(prefab->objectID())}));
}

TEST_F(ObjectTreeViewPrefabModelTest, DuplicationPrefabChildrenCanBeDuplicated) {
	auto prefab = createNodes(Prefab::typeDescription.typeName, {Prefab::typeDescription.typeName}).front();
	auto allSceneGraphNodes = createAllSceneGraphObjects();

	moveScenegraphChildren(allSceneGraphNodes, prefab);

	for (const auto &sceneGraphNodeInScene : allSceneGraphNodes) {
		auto sceneObjIndex = viewModel_->indexFromTreeNodeID(sceneGraphNodeInScene->objectID());
		ASSERT_TRUE(viewModel_->canDuplicateAtIndices({sceneObjIndex}));
	}
}

TEST_F(ObjectTreeViewPrefabModelTest, DuplicationExtrefPrefabCanNotBeDuplicated) {
	auto extRefPrefab = createNodes(Prefab::typeDescription.typeName, {Prefab::typeDescription.typeName}).front();
	auto allSceneGraphNodes = createAllSceneGraphObjects();

	moveScenegraphChildren(allSceneGraphNodes, extRefPrefab);

	extRefPrefab->addAnnotation(std::make_shared<raco::core::ExternalReferenceAnnotation>("differentProject"));
	for (auto &node : allSceneGraphNodes) {
		node->addAnnotation(std::make_shared<raco::core::ExternalReferenceAnnotation>("differentProject"));
	}

	ASSERT_FALSE(viewModel_->canDuplicateAtIndices({viewModel_->indexFromTreeNodeID(extRefPrefab->objectID())}));
	for (const auto &sceneGraphNodeInScene : allSceneGraphNodes) {
		auto sceneObjIndex = viewModel_->indexFromTreeNodeID(sceneGraphNodeInScene->objectID());
		ASSERT_FALSE(viewModel_->canDuplicateAtIndices({sceneObjIndex}));
	}
}

TEST_F(ObjectTreeViewPrefabModelTest, DuplicationPrefabInstanceChildrenInPrefabCanNotBeDuplicated) {
	auto prefabs = createNodes(Prefab::typeDescription.typeName, {"p1", "p2"});
	auto prefabInstance = createNodes(PrefabInstance::typeDescription.typeName, {PrefabInstance::typeDescription.typeName}).front();
	auto allSceneGraphNodes = createAllSceneGraphObjects();

	moveScenegraphChildren(allSceneGraphNodes, prefabs[0]);
	moveScenegraphChildren({prefabInstance}, prefabs[1]);

	commandInterface().set({prefabInstance, &PrefabInstance::template_}, prefabs[0]);

	viewModel_->buildObjectTree();

	for (const auto &instChild : prefabs[1]->children_->asVector<SEditorObject>()[0]->children_->asVector<SEditorObject>()) {
		auto sceneObjIndex = viewModel_->indexFromTreeNodeID(instChild->objectID());
		ASSERT_FALSE(viewModel_->canDuplicateAtIndices({sceneObjIndex}));
	}
}

TEST_F(ObjectTreeViewPrefabModelTest, DuplicationDifferentNodesInDifferentPrefabsCanNotBeDuplicated) {
	auto prefabs = createNodes(Prefab::typeDescription.typeName, {"p1", "p2"});
	auto nodes = createNodes(Node::typeDescription.typeName, {"n11", "n12", "n21"});

	moveScenegraphChildren({nodes[0]}, prefabs[0]);
	moveScenegraphChildren({nodes[1]}, nodes[0]);
	moveScenegraphChildren({nodes[2]}, prefabs[1]);

	viewModel_->buildObjectTree();

	auto prefab1Child = viewModel_->indexFromTreeNodeID(nodes[1]->objectID());
	auto prefab2Child = viewModel_->indexFromTreeNodeID(nodes[2]->objectID());

	ASSERT_FALSE(viewModel_->canDuplicateAtIndices({prefab1Child, prefab2Child}));
}

TEST_F(ObjectTreeViewPrefabModelTest, DuplicationCanNotDuplicateNothing) {
	ASSERT_FALSE(viewModel_->canDuplicateAtIndices({}));
}


TEST_F(ObjectTreeViewPrefabModelTest, move_prefab_toplevel) {
	auto prefab = create<Prefab>("prefab");
	dispatch();

	auto mimeData = viewModel_->mimeData({viewModel_->indexFromTreeNodeID(prefab->objectID())});
	ASSERT_FALSE(viewModel_->canDropMimeData(mimeData, Qt::DropAction::MoveAction, 0, 0, {}));
}

TEST_F(ObjectTreeViewPrefabModelTest, move_prefab_into_prefab) {
	auto prefab = create<Prefab>("prefab");
	auto prefab2 = create<Prefab>("prefab2");
	dispatch();

	auto prefabIndex = viewModel_->indexFromTreeNodeID(prefab->objectID());
	auto mimeData = viewModel_->mimeData({viewModel_->indexFromTreeNodeID(prefab2->objectID())});
	ASSERT_FALSE(viewModel_->canDropMimeData(mimeData, Qt::DropAction::MoveAction, 0, 0, prefabIndex));
}
