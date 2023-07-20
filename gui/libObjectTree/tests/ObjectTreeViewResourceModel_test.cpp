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
#include "object_tree_view_model/ObjectTreeViewResourceModel.h"
#include "user_types/AnchorPoint.h"
#include "user_types/AnimationChannel.h"
#include "user_types/BlitPass.h"
#include "user_types/LuaScriptModule.h"
#include "user_types/MeshNode.h"
#include "user_types/RenderBuffer.h"
#include "user_types/RenderBufferMS.h"
#include "user_types/RenderLayer.h"
#include "user_types/RenderTarget.h"
#include "user_types/RenderPass.h"
#include "user_types/TextureExternal.h"
#include "user_types/Timer.h"

using namespace raco::user_types;

class ObjectTreeViewResourceModelTest : public ObjectTreeViewDefaultModelTest {
public:
	ObjectTreeViewResourceModelTest() : ObjectTreeViewDefaultModelTest() {
		viewModel_.reset(new raco::object_tree::model::ObjectTreeViewResourceModel(&commandInterface(), application.dataChangeDispatcher(), nullptr,
			{
				AnchorPoint::typeDescription.typeName,
				AnimationChannel::typeDescription.typeName,
				BlitPass::typeDescription.typeName,
				CubeMap::typeDescription.typeName,
				Material::typeDescription.typeName,
				Mesh::typeDescription.typeName,
				LuaScriptModule::typeDescription.typeName,
				Texture::typeDescription.typeName,
				TextureExternal::typeDescription.typeName,
				Timer::typeDescription.typeName,
				RenderBuffer::typeDescription.typeName,
				RenderBufferMS::typeDescription.typeName,
				RenderTarget::typeDescription.typeName,
				RenderLayer::typeDescription.typeName,
				RenderPass::typeDescription.typeName}));
	}
};

TEST_F(ObjectTreeViewResourceModelTest, TypesAllowedIntoIndexEmptyIndex) {
	auto allowedTypes = viewModel_->typesAllowedIntoIndex({});
	std::vector<std::string> allowedTypesAssert {
		AnchorPoint::typeDescription.typeName,
		AnimationChannel::typeDescription.typeName,
		BlitPass::typeDescription.typeName,
		CubeMap::typeDescription.typeName,
		LuaScriptModule::typeDescription.typeName,
		Material::typeDescription.typeName,
		Mesh::typeDescription.typeName,
		RenderBuffer::typeDescription.typeName,
		RenderBufferMS::typeDescription.typeName,
		RenderLayer::typeDescription.typeName,
		RenderPass::typeDescription.typeName,
		RenderTarget::typeDescription.typeName,
		Texture::typeDescription.typeName,
		TextureExternal::typeDescription.typeName,
		Timer::typeDescription.typeName};

	EXPECT_EQ(allowedTypes, allowedTypesAssert);
}

TEST_F(ObjectTreeViewResourceModelTest, TypesAllowedIntoIndexAnyTypeBehavesLikeEmptyIndex) {
	auto allowedTypesEmptyIndex = viewModel_->typesAllowedIntoIndex({});

	for (const auto &typeName : getTypes()) {
		auto item = createNodes(typeName, {typeName}).front();
		auto allowedTypes = viewModel_->typesAllowedIntoIndex(viewModel_->indexFromTreeNodeID(item->objectID()));

		ASSERT_EQ(allowedTypes.size(), allowedTypesEmptyIndex.size());
		for (int i = 0; i < allowedTypes.size(); ++i) {
			ASSERT_EQ(allowedTypes[i], allowedTypesEmptyIndex[i]);
		}
	}
}

TEST_F(ObjectTreeViewResourceModelTest, AllowedObjsResourcesAreAllowedOnTopLevel) {
	for (const auto &typeName : getTypes()) {
		auto newObj = commandInterface().createObject(typeName);
		if (raco::core::Queries::isResource(newObj)) {
			ASSERT_TRUE(viewModel_->isObjectAllowedIntoIndex({}, newObj));
		}
	}
}

TEST_F(ObjectTreeViewResourceModelTest, DuplicationCanNotDuplicateNothing) {
	ASSERT_FALSE(viewModel_->canDuplicateAtIndices({}));
}

TEST_F(ObjectTreeViewResourceModelTest, DuplicationResourcesCanBeDuplicated) {
	for (const auto &typeName : getTypes()) {
		auto newObj = commandInterface().createObject(typeName);
		if (raco::core::Queries::isResource(newObj)) {
			auto newObj = createNodes(typeName, {typeName}).front();
			ASSERT_TRUE(viewModel_->canDuplicateAtIndices({viewModel_->indexFromTreeNodeID(newObj->objectID())}));
		}
	}
}

TEST_F(ObjectTreeViewResourceModelTest, DuplicationExtRefResourcesCanNotBeDuplicated) {
	for (const auto &typeName : getTypes()) {
		auto newObj = commandInterface().createObject(typeName);
		if (raco::core::Queries::isResource(newObj)) {
			auto newObj = createNodes(typeName, {typeName}).front();
			newObj->addAnnotation(std::make_shared<raco::core::ExternalReferenceAnnotation>("differentProject"));

			ASSERT_FALSE(viewModel_->canDuplicateAtIndices({viewModel_->indexFromTreeNodeID(newObj->objectID())}));
		}
	}
}

TEST_F(ObjectTreeViewResourceModelTest, AllowedObjsResourcesAreAllowedOnTopLevelAsExtRefs) {
	for (const auto &typeName : getTypes()) {
		auto newObj = commandInterface().createObject(typeName);
		if (raco::core::Queries::isResource(newObj)) {
			newObj->addAnnotation(std::make_shared<raco::core::ExternalReferenceAnnotation>("differentProject"));
			auto copiedObjs = commandInterface().copyObjects({newObj}, true);
			dispatch();

			auto [parsedObjs, sourceProjectTopLevelObjectIds] = viewModel_->getObjectsAndRootIdsFromClipboardString(copiedObjs);
			ASSERT_EQ(viewModel_->canPasteIntoIndex({}, parsedObjs, sourceProjectTopLevelObjectIds, true),
				!(newObj->isType<RenderPass>() || newObj->isType<AnchorPoint>()));
		}
	}
}

TEST_F(ObjectTreeViewResourceModelTest, AllowedObjsResourcesAreAllowedWithResourceParentSelected) {
	auto allResources = createAllResources();
	for (const auto &typeName : getTypes()) {
		auto newObj = commandInterface().createObject(typeName);
		if (raco::core::Queries::isResource(newObj)) {
			for (const auto &resourceInScene : allResources) {
				ASSERT_FALSE(viewModel_->isObjectAllowedIntoIndex(viewModel_->indexFromTreeNodeID(resourceInScene->objectID()), newObj));
				ASSERT_TRUE(viewModel_->isObjectAllowedIntoIndex({}, newObj));
			}
		}
	}
}

TEST_F(ObjectTreeViewResourceModelTest, AllowedObjsResourcesAreAllowedAsExtRef) {
	auto allResources = createAllResources();
	for (const auto &typeName : getTypes()) {
		auto newObj = commandInterface().createObject(typeName);
		if (raco::core::Queries::isResource(newObj)) {
			newObj->addAnnotation(std::make_shared<raco::core::ExternalReferenceAnnotation>("differentProject"));
			auto copyObjs = commandInterface().copyObjects({newObj}, true);
			dispatch();

			auto [parsedObjs, sourceProjectTopLevelObjectIds] = viewModel_->getObjectsAndRootIdsFromClipboardString(copyObjs);
			ASSERT_EQ(viewModel_->canPasteIntoIndex({}, parsedObjs, sourceProjectTopLevelObjectIds, true),
				!(newObj->isType<RenderPass>() || newObj->isType<AnchorPoint>()));
		}
	}
}

TEST_F(ObjectTreeViewResourceModelTest, AllowedObjsSceneGraphObjectsAreNotAllowed) {
	auto allResources = createAllResources();
	for (const auto &typeName : getTypes()) {
		auto newObj = commandInterface().createObject(typeName);
		if (!raco::core::Queries::isResource(newObj)) {
			ASSERT_FALSE(viewModel_->isObjectAllowedIntoIndex({}, newObj));
		}
	}
}

TEST_F(ObjectTreeViewResourceModelTest, PastePastingMaterialsUnderMaterialCreatesMaterialOnTopLevel) {
	auto material = createNodes(Material::typeDescription.typeName, {Material::typeDescription.typeName}).front();
	auto materialIndex = viewModel_->indexFromTreeNodeID(material->objectID());

	auto resourceChild = createNodes(Mesh::typeDescription.typeName, {Mesh::typeDescription.typeName}).front();
	auto cutObjs = commandInterface().cutObjects({resourceChild}, false);
	dispatch(); 

	materialIndex = viewModel_->indexFromTreeNodeID(material->objectID());

	auto [parsedObjs, sourceProjectTopLevelObjectIds] = viewModel_->getObjectsAndRootIdsFromClipboardString(cutObjs);
	ASSERT_FALSE(viewModel_->canPasteIntoIndex(materialIndex, parsedObjs, sourceProjectTopLevelObjectIds));
	ASSERT_TRUE(viewModel_->canPasteIntoIndex({}, parsedObjs, sourceProjectTopLevelObjectIds));

	viewModel_->pasteObjectAtIndex(materialIndex, false, nullptr, cutObjs);
	dispatch();

	materialIndex = viewModel_->indexFromTreeNodeID(material->objectID());
	ASSERT_TRUE(materialIndex.isValid());
	ASSERT_EQ(viewModel_->indexToTreeNode(materialIndex)->childCount(), 0);
	ASSERT_EQ(viewModel_->project()->instances().size(), 3);
}

TEST_F(ObjectTreeViewResourceModelTest, AllowedObjsDeepCopiedSceneGraphWithResourcesIsAllowed) {
	auto meshNode = createNodes(MeshNode::typeDescription.typeName, {MeshNode::typeDescription.typeName}).front();
	auto mesh = createNodes(Mesh::typeDescription.typeName, {Mesh::typeDescription.typeName}).front();

	commandInterface().set(raco::core::ValueHandle{meshNode, {"mesh"}}, mesh);
	dispatch();

	auto cutObjs = commandInterface().cutObjects({meshNode}, true);
	dispatch();

	auto [parsedObjs, sourceProjectTopLevelObjectIds] = viewModel_->getObjectsAndRootIdsFromClipboardString(cutObjs);
	ASSERT_TRUE(viewModel_->canPasteIntoIndex({}, parsedObjs, sourceProjectTopLevelObjectIds));
}

TEST_F(ObjectTreeViewResourceModelTest, AllowedObjsDeepCopiedPrefabInstanceWithPrefabIsNotAllowed) {
	auto prefabInstance = createNodes(PrefabInstance::typeDescription.typeName, {PrefabInstance::typeDescription.typeName}).front();
	auto prefab = createNodes(Prefab::typeDescription.typeName, {Prefab::typeDescription.typeName}).front();

	commandInterface().set(raco::core::ValueHandle{prefabInstance, {"template"}}, prefab);
	dispatch();

	auto cutObjs = commandInterface().cutObjects({prefabInstance}, true);
	dispatch();

	auto [parsedObjs, sourceProjectTopLevelObjectIds] = viewModel_->getObjectsAndRootIdsFromClipboardString(cutObjs);
	ASSERT_FALSE(viewModel_->canPasteIntoIndex({}, parsedObjs, sourceProjectTopLevelObjectIds));
}

TEST_F(ObjectTreeViewResourceModelTest, AllowedObjsNoSceneGraphObjectsAreAllowedUnderExtRef) {
	auto extRefMesh = createNodes(Mesh::typeDescription.typeName, {Mesh::typeDescription.typeName}).front();
	extRefMesh->addAnnotation(std::make_shared<raco::core::ExternalReferenceAnnotation>("differentProject"));

	viewModel_->buildObjectTree();
	auto extRefMeshIndex = viewModel_->indexFromTreeNodeID(extRefMesh->objectID());
	auto extRefGroupIndex = viewModel_->index(0, 0);
	ASSERT_EQ(viewModel_->indexToTreeNode(extRefGroupIndex)->getType(), raco::object_tree::model::ObjectTreeNodeType::ExtRefGroup);

	for (const auto &typeName : getTypes()) {
		auto newObj = commandInterface().createObject(typeName);
		ASSERT_FALSE(viewModel_->isObjectAllowedIntoIndex(extRefMeshIndex, newObj));	
		ASSERT_EQ(viewModel_->isObjectAllowedIntoIndex(extRefGroupIndex, newObj), raco::core::Queries::isResource(newObj));	
	}
}

TEST_F(ObjectTreeViewResourceModelTest, CanNotDoAnythingButPasteWithExtRefGroup) {
	auto extRefMesh = createNodes(Mesh::typeDescription.typeName, {Mesh::typeDescription.typeName}).front();
	extRefMesh->addAnnotation(std::make_shared<raco::core::ExternalReferenceAnnotation>("differentProject"));

	viewModel_->buildObjectTree();
	auto extRefGroupIndex = viewModel_->index(0, 0);
	ASSERT_EQ(viewModel_->indexToTreeNode(extRefGroupIndex)->getType(), raco::object_tree::model::ObjectTreeNodeType::ExtRefGroup);

	ASSERT_FALSE(viewModel_->canDeleteAtIndices({extRefGroupIndex}));
	ASSERT_FALSE(viewModel_->canCopyAtIndices({extRefGroupIndex}));
	ASSERT_FALSE(viewModel_->canDuplicateAtIndices({extRefGroupIndex}));

	dispatch();
	auto copiedObjs = commandInterface().copyObjects({extRefMesh});
	dispatch();

	auto [parsedObjs, sourceProjectTopLevelObjectIds] = viewModel_->getObjectsAndRootIdsFromClipboardString(copiedObjs);
	ASSERT_TRUE(viewModel_->canPasteIntoIndex({}, parsedObjs, sourceProjectTopLevelObjectIds));
	ASSERT_TRUE(viewModel_->canPasteIntoIndex(extRefGroupIndex, parsedObjs, sourceProjectTopLevelObjectIds));
}