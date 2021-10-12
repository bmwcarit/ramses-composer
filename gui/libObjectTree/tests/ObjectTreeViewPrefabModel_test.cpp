/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "gtest/gtest.h"

#include "ObjectTreeViewDefaultModel_test.h"
#include "core/ExternalReferenceAnnotation.h"
#include "core/Queries.h"
#include "object_tree_view_model/ObjectTreeViewPrefabModel.h"

class ObjectTreeViewPrefabModelTest : public ObjectTreeViewDefaultModelTest {
public:
	ObjectTreeViewPrefabModelTest() : ObjectTreeViewDefaultModelTest() {
		viewModel_.reset(new raco::object_tree::model::ObjectTreeViewPrefabModel(&commandInterface, dataChangeDispatcher_, nullptr,
			{raco::user_types::Node::typeDescription.typeName,
				raco::user_types::MeshNode::typeDescription.typeName,
				raco::user_types::Prefab::typeDescription.typeName,
				raco::user_types::PrefabInstance::typeDescription.typeName,
				raco::user_types::OrthographicCamera::typeDescription.typeName,
				raco::user_types::PerspectiveCamera::typeDescription.typeName,
				raco::user_types::LuaScript::typeDescription.typeName}));
	}
};


TEST_F(ObjectTreeViewPrefabModelTest, AllowedObjsResourcesAreNotAllowedOnTopLevel) {
	for (const auto &[typeName, typeInfo] : viewModel_->objectFactory()->getTypes()) {
		auto newObj = viewModel_->objectFactory()->createObject(typeName);
		if (raco::core::Queries::isResource(newObj)) {
			ASSERT_FALSE(viewModel_->objectsAreAllowedInModel({newObj}, viewModel_->getInvisibleRootIndex()));
		}
	}
}


TEST_F(ObjectTreeViewPrefabModelTest, AllowedObjsResourcesAreNotAllowedUnderPrefab) {
	auto prefab = createNodes(raco::user_types::Prefab::typeDescription.typeName, {raco::user_types::Prefab::typeDescription.typeName});

	for (const auto &[typeName, typeInfo] : viewModel_->objectFactory()->getTypes()) {
		auto newObj = viewModel_->objectFactory()->createObject(typeName);
		if (raco::core::Queries::isResource(newObj)) {
			ASSERT_FALSE(viewModel_->objectsAreAllowedInModel({newObj}, viewModel_->indexFromObjectID(raco::user_types::Prefab::typeDescription.typeName + raco::user_types::Prefab::typeDescription.typeName)));
		}
	}
}

TEST_F(ObjectTreeViewPrefabModelTest, AllowedObjsCheckSceneGraphObjectsOnTopLevel) {
	for (const auto &[typeName, typeInfo] : viewModel_->objectFactory()->getTypes()) {
		auto newObj = viewModel_->objectFactory()->createObject(typeName);
		if (!raco::core::Queries::isResource(newObj)) {
			if (typeName == raco::user_types::Prefab::typeDescription.typeName) {
				ASSERT_TRUE(viewModel_->objectsAreAllowedInModel({newObj}, viewModel_->getInvisibleRootIndex()));
			} else {
				ASSERT_FALSE(viewModel_->objectsAreAllowedInModel({newObj}, viewModel_->getInvisibleRootIndex()));
			}
		}
	}
}

TEST_F(ObjectTreeViewPrefabModelTest, AllowedObjsCheckExternalSceneGraphObjectsUnderPrefab) {
	auto prefab = createNodes(raco::user_types::Prefab::typeDescription.typeName, {raco::user_types::Prefab::typeDescription.typeName});
	auto prefabIndex = viewModel_->indexFromObjectID(raco::user_types::Prefab::typeDescription.typeName + raco::user_types::Prefab::typeDescription.typeName);

	for (const auto &[typeName, typeInfo] : viewModel_->objectFactory()->getTypes()) {
		auto newObj = viewModel_->objectFactory()->createObject(typeName);
		if (!raco::core::Queries::isResource(newObj) && !raco::core::Queries::isProjectSettings(newObj)) {
			if (newObj->as<raco::user_types::Prefab>()) {
				ASSERT_FALSE(viewModel_->objectsAreAllowedInModel({newObj}, prefabIndex));
			} else {
				ASSERT_TRUE(viewModel_->objectsAreAllowedInModel({newObj}, prefabIndex));
			}
		}
	}
}

TEST_F(ObjectTreeViewPrefabModelTest, AllowedObjsPrefabInTreeViewIsNotMovable) {
	auto prefabs = createNodes(raco::user_types::Prefab::typeDescription.typeName, {"prefab1", "prefab2"});
	auto prefabIndex = viewModel_->indexFromObjectID(raco::user_types::Prefab::typeDescription.typeName + "prefab1");

	ASSERT_FALSE(viewModel_->objectsAreAllowedInModel({prefabs[1]}, prefabIndex));
}

TEST_F(ObjectTreeViewPrefabModelTest, AllowedObjsDeepCopiedSceneGraphWithResourcesIsNotAllowed) {
	auto meshNode = createNodes(raco::user_types::MeshNode::typeDescription.typeName, {raco::user_types::MeshNode::typeDescription.typeName}).front();
	auto mesh = createNodes(raco::user_types::Mesh::typeDescription.typeName, {raco::user_types::Mesh::typeDescription.typeName}).front();

	commandInterface.set(raco::core::ValueHandle{meshNode, {"mesh"}}, mesh);
	dataChangeDispatcher_->dispatch(recorder.release());

	auto cutObjs = commandInterface.cutObjects({meshNode}, true);
	dataChangeDispatcher_->dispatch(recorder.release());

	ASSERT_FALSE(viewModel_->canPasteInto(viewModel_->getInvisibleRootIndex(), cutObjs));
}

TEST_F(ObjectTreeViewPrefabModelTest, AllowedObjsDeepCopiedSceneGraphWithResourcesAllowedUnderPrefab) {
	auto meshNode = createNodes(raco::user_types::MeshNode::typeDescription.typeName, {raco::user_types::MeshNode::typeDescription.typeName}).front();
	auto mesh = createNodes(raco::user_types::Mesh::typeDescription.typeName, {raco::user_types::Mesh::typeDescription.typeName}).front();
	auto prefab = createNodes(raco::user_types::Prefab::typeDescription.typeName, {raco::user_types::Prefab::typeDescription.typeName}).front();

	commandInterface.set(raco::core::ValueHandle{meshNode, {"mesh"}}, mesh);
	dataChangeDispatcher_->dispatch(recorder.release());

	auto copiedObjs = commandInterface.copyObjects({meshNode}, true);
	dataChangeDispatcher_->dispatch(recorder.release());

	ASSERT_TRUE(viewModel_->canPasteInto(viewModel_->indexFromObjectID(prefab->objectID()), copiedObjs));
}

TEST_F(ObjectTreeViewPrefabModelTest, AllowedObjsDeepCopiedPrefabInstanceWithPrefabIsNotAllowed) {
	auto prefabInstance = createNodes(raco::user_types::PrefabInstance::typeDescription.typeName, {raco::user_types::PrefabInstance::typeDescription.typeName}).front();
	auto prefab = createNodes(raco::user_types::Prefab::typeDescription.typeName, {raco::user_types::Prefab::typeDescription.typeName}).front();

	commandInterface.set(raco::core::ValueHandle{prefabInstance, {"template"}}, prefab);
	dataChangeDispatcher_->dispatch(recorder.release());

	auto cutObjs = commandInterface.cutObjects({prefabInstance}, true);
	dataChangeDispatcher_->dispatch(recorder.release());

	ASSERT_FALSE(viewModel_->canPasteInto(viewModel_->getInvisibleRootIndex(), cutObjs));
}

TEST_F(ObjectTreeViewPrefabModelTest, AllowedObjsDeepCopiedPrefabIsAllowedInEmptySpace) {
	auto meshNode = createNodes(raco::user_types::MeshNode::typeDescription.typeName, {raco::user_types::MeshNode::typeDescription.typeName}).front();
	auto mesh = createNodes(raco::user_types::Mesh::typeDescription.typeName, {raco::user_types::Mesh::typeDescription.typeName}).front();
	auto prefab = createNodes(raco::user_types::Prefab::typeDescription.typeName, {raco::user_types::Prefab::typeDescription.typeName}).front();

	commandInterface.set(raco::core::ValueHandle{meshNode, {"mesh"}}, mesh);
	dataChangeDispatcher_->dispatch(recorder.release());

	commandInterface.moveScenegraphChild(meshNode, prefab);
	dataChangeDispatcher_->dispatch(recorder.release());

	auto copiedObjs = commandInterface.copyObjects({prefab}, true);
	dataChangeDispatcher_->dispatch(recorder.release());

	ASSERT_TRUE(viewModel_->canPasteInto(viewModel_->getInvisibleRootIndex(), copiedObjs));
}

TEST_F(ObjectTreeViewPrefabModelTest, AllowedObjsDeepCopiedPrefabIsAllowedUnderPrefab) {
	auto meshNode = createNodes(raco::user_types::MeshNode::typeDescription.typeName, {raco::user_types::MeshNode::typeDescription.typeName}).front();
	auto mesh = createNodes(raco::user_types::Mesh::typeDescription.typeName, {raco::user_types::Mesh::typeDescription.typeName}).front();
	auto prefabs = createNodes(raco::user_types::Prefab::typeDescription.typeName, {"prefab", "prefab2"});

	commandInterface.set(raco::core::ValueHandle{meshNode, {"mesh"}}, mesh);
	dataChangeDispatcher_->dispatch(recorder.release());

	commandInterface.moveScenegraphChild(meshNode, prefabs.front());
	dataChangeDispatcher_->dispatch(recorder.release());

	auto copiedObjs = commandInterface.copyObjects({prefabs.front()}, true);
	dataChangeDispatcher_->dispatch(recorder.release());

	ASSERT_TRUE(viewModel_->canPasteInto(viewModel_->indexFromObjectID("prefab2"), copiedObjs));
}

TEST_F(ObjectTreeViewPrefabModelTest, AllowedObjsNothingIsAllowedUnderExtRef) {
	auto extRefPrefab = createNodes(raco::user_types::Prefab::typeDescription.typeName, {raco::user_types::Prefab::typeDescription.typeName}).front();
	extRefPrefab->addAnnotation(std::make_shared<raco::core::ExternalReferenceAnnotation>("differentProject"));
	auto extRefPrefabIndex = viewModel_->indexFromObjectID(raco::user_types::Prefab::typeDescription.typeName + raco::user_types::Prefab::typeDescription.typeName);

	for (const auto &[typeName, typeInfo] : viewModel_->objectFactory()->getTypes()) {
		auto newObj = viewModel_->objectFactory()->createObject(typeName);
		ASSERT_FALSE(viewModel_->objectsAreAllowedInModel({newObj}, extRefPrefabIndex));
	}
}
