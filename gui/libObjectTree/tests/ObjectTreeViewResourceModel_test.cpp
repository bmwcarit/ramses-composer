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
#include "object_tree_view_model/ObjectTreeViewResourceModel.h"
#include "core/SerializationFunctions.h"
#include "user_types/AnimationChannel.h"
#include "user_types/RenderBuffer.h"
#include "user_types/RenderLayer.h"
#include "user_types/RenderTarget.h"
#include "user_types/RenderPass.h"

class ObjectTreeViewResourceModelTest : public ObjectTreeViewDefaultModelTest {
public:
	ObjectTreeViewResourceModelTest() : ObjectTreeViewDefaultModelTest() {
		viewModel_.reset(new raco::object_tree::model::ObjectTreeViewResourceModel(&commandInterface, dataChangeDispatcher_, nullptr,
			{
				raco::user_types::AnimationChannel::typeDescription.typeName,
				raco::user_types::CubeMap::typeDescription.typeName,
				raco::user_types::Material::typeDescription.typeName,
				raco::user_types::Mesh::typeDescription.typeName,
				raco::user_types::Texture::typeDescription.typeName,
				raco::user_types::RenderBuffer::typeDescription.typeName,
				raco::user_types::RenderTarget::typeDescription.typeName,
				raco::user_types::RenderLayer::typeDescription.typeName,
				raco::user_types::RenderPass::typeDescription.typeName}));
	}
};


TEST_F(ObjectTreeViewResourceModelTest, AllowedObjsResourcesAreAllowedOnTopLevel) {
	for (const auto &[typeName, typeInfo] : viewModel_->objectFactory()->getTypes()) {
		auto newObj = viewModel_->objectFactory()->createObject(typeName);
		if (raco::core::Queries::isResource(newObj)) {
			ASSERT_TRUE(viewModel_->objectsAreAllowedInModel({newObj}, viewModel_->getInvisibleRootIndex()));
		}
	}
}

TEST_F(ObjectTreeViewResourceModelTest, AllowedObjsResourcesAreAllowedOnTopLevelAsExtRefs) {
	for (const auto &[typeName, typeInfo] : viewModel_->objectFactory()->getTypes()) {
		auto newObj = viewModel_->objectFactory()->createObject(typeName);
		if (raco::core::Queries::isResource(newObj)) {
			newObj->addAnnotation(std::make_shared<raco::core::ExternalReferenceAnnotation>("differentProject"));
			auto copiedObjs = commandInterface.copyObjects({newObj}, true);
			dataChangeDispatcher_->dispatch(recorder.release());

			ASSERT_EQ(viewModel_->canPasteInto(viewModel_->getInvisibleRootIndex(), copiedObjs, true),
				&newObj->getTypeDescription() != &raco::user_types::RenderPass::typeDescription);
		}
	}
}

TEST_F(ObjectTreeViewResourceModelTest, AllowedObjsResourcesAreAllowedWithResourceParentSelected) {
	auto allResources = createAllResources();
	for (const auto &[typeName, typeInfo] : viewModel_->objectFactory()->getTypes()) {
		auto newObj = viewModel_->objectFactory()->createObject(typeName);
		if (raco::core::Queries::isResource(newObj)) {
			for (const auto &resourceInScene : allResources) {
				ASSERT_TRUE(viewModel_->objectsAreAllowedInModel({newObj}, viewModel_->indexFromObjectID(resourceInScene->objectID())));
			}
		}
	}
}

TEST_F(ObjectTreeViewResourceModelTest, AllowedObjsResourcesAreAllowedAsExtRefWithResourceParentSelected) {
	auto allResources = createAllResources();
	for (const auto &[typeName, typeInfo] : viewModel_->objectFactory()->getTypes()) {
		auto newObj = viewModel_->objectFactory()->createObject(typeName);
		if (raco::core::Queries::isResource(newObj)) {
			newObj->addAnnotation(std::make_shared<raco::core::ExternalReferenceAnnotation>("differentProject"));
			auto copyObjs = commandInterface.copyObjects({newObj}, true);
			dataChangeDispatcher_->dispatch(recorder.release());

			for (const auto &resourceInScene : allResources) {
				ASSERT_EQ(viewModel_->canPasteInto(viewModel_->indexFromObjectID(resourceInScene->objectID()), copyObjs, true),
					&newObj->getTypeDescription() != &raco::user_types::RenderPass::typeDescription);
			}
		}
	}
}

TEST_F(ObjectTreeViewResourceModelTest, AllowedObjsSceneGraphObjectsAreNotAllowed) {
	auto allResources = createAllResources();
	for (const auto &[typeName, typeInfo] : viewModel_->objectFactory()->getTypes()) {
		auto newObj = viewModel_->objectFactory()->createObject(typeName);
		if (!raco::core::Queries::isResource(newObj)) {
			ASSERT_FALSE(viewModel_->objectsAreAllowedInModel({newObj}, viewModel_->getInvisibleRootIndex()));
		}
	}
}

TEST_F(ObjectTreeViewResourceModelTest, PastePastingMaterialsUnderMaterialCreatesMaterialOnTopLevel) {
	createNodes(raco::user_types::Material::typeDescription.typeName, {raco::user_types::Material::typeDescription.typeName}).front();
	auto materialIndex = viewModel_->indexFromObjectID(raco::user_types::Material::typeDescription.typeName + raco::user_types::Material::typeDescription.typeName);

	auto resourceChild = createNodes(raco::user_types::Mesh::typeDescription.typeName, {raco::user_types::Mesh::typeDescription.typeName}).front();
	auto cutObjs = commandInterface.cutObjects({resourceChild}, false);
	dataChangeDispatcher_->dispatch(recorder.release());

	ASSERT_TRUE(viewModel_->canPasteInto(materialIndex, cutObjs));

	viewModel_->pasteObjectAtIndex(materialIndex, false, nullptr, cutObjs);
	dataChangeDispatcher_->dispatch(recorder.release());

	materialIndex = viewModel_->indexFromObjectID(raco::user_types::Material::typeDescription.typeName + raco::user_types::Material::typeDescription.typeName);
	ASSERT_TRUE(materialIndex.isValid());
	ASSERT_EQ(viewModel_->indexToTreeNode(materialIndex)->childCount(), 0);
	ASSERT_EQ(viewModel_->project()->instances().size(), 2);
}

TEST_F(ObjectTreeViewResourceModelTest, AllowedObjsDeepCopiedSceneGraphWithResourcesIsNotAllowed) {
	auto meshNode = createNodes(raco::user_types::MeshNode::typeDescription.typeName, {raco::user_types::MeshNode::typeDescription.typeName}).front();
	auto mesh = createNodes(raco::user_types::Mesh::typeDescription.typeName, {raco::user_types::Mesh::typeDescription.typeName}).front();

	commandInterface.set(raco::core::ValueHandle{meshNode, {"mesh"}}, mesh);
	dataChangeDispatcher_->dispatch(recorder.release());

	auto cutObjs = commandInterface.cutObjects({meshNode}, true);
	dataChangeDispatcher_->dispatch(recorder.release());

	ASSERT_FALSE(viewModel_->canPasteInto(viewModel_->getInvisibleRootIndex(), cutObjs));
}

TEST_F(ObjectTreeViewResourceModelTest, AllowedObjsDeepCopiedPrefabInstanceWithPrefabIsNotAllowed) {
	auto prefabInstance = createNodes(raco::user_types::PrefabInstance::typeDescription.typeName, {raco::user_types::PrefabInstance::typeDescription.typeName}).front();
	auto prefab = createNodes(raco::user_types::Prefab::typeDescription.typeName, {raco::user_types::Prefab::typeDescription.typeName}).front();

	commandInterface.set(raco::core::ValueHandle{prefabInstance, {"template"}}, prefab);
	dataChangeDispatcher_->dispatch(recorder.release());

	auto cutObjs = commandInterface.cutObjects({prefabInstance}, true);
	dataChangeDispatcher_->dispatch(recorder.release());

	ASSERT_FALSE(viewModel_->canPasteInto(viewModel_->getInvisibleRootIndex(), cutObjs));
}

TEST_F(ObjectTreeViewResourceModelTest, AllowedObjsNoSceneGraphObjectsAreAllowedUnderExtRef) {
	auto extRefMesh = createNodes(raco::user_types::Mesh::typeDescription.typeName, {raco::user_types::Mesh::typeDescription.typeName}).front();
	extRefMesh->addAnnotation(std::make_shared<raco::core::ExternalReferenceAnnotation>("differentProject"));
	auto extRefMeshIndex = viewModel_->indexFromObjectID(raco::user_types::Mesh::typeDescription.typeName + raco::user_types::Mesh::typeDescription.typeName);

	for (const auto &[typeName, typeInfo] : viewModel_->objectFactory()->getTypes()) {
		auto newObj = viewModel_->objectFactory()->createObject(typeName);
		if (!raco::core::Queries::isResource(newObj)) {
			ASSERT_FALSE(viewModel_->objectsAreAllowedInModel({newObj}, extRefMeshIndex));
		}
	}
}