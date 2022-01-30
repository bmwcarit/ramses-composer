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

#include "testing/TestUtil.h"

#include "core/Queries_Tags.h"

#include "user_types/MeshNode.h"
#include "user_types/PrefabInstance.h"
#include "user_types/RenderLayer.h"
#include "user_types/Texture.h"

#include <gtest/gtest.h>

using namespace raco::core;
using namespace raco::user_types;


class QueriesTagTest : public TestEnvironmentCore {
public:
	void SetUp() override {
		TestEnvironmentCore::SetUp();
		rootnode_ = commandInterface.createObject(Node::typeDescription.typeName)->as<Node>();
		renderLayer_ = commandInterface.createObject(RenderLayer::typeDescription.typeName)->as<RenderLayer>();
		material_ = commandInterface.createObject(Material::typeDescription.typeName)->as<Material>();
		meshnode_ = commandInterface.createObject(MeshNode::typeDescription.typeName)->as<MeshNode>();
		mesh_ = commandInterface.createObject(Mesh::typeDescription.typeName)->as<Mesh>();
		prefab_ = commandInterface.createObject(Prefab::typeDescription.typeName)->as<Prefab>();
		prefabInstance_ = commandInterface.createObject(PrefabInstance::typeDescription.typeName)->as<PrefabInstance>();
		meshnodeInPrefab_ = commandInterface.createObject(MeshNode::typeDescription.typeName)->as<MeshNode>();

		commandInterface.set({mesh_, {"uri"}}, (test_path() / "meshes" / "Duck.glb").string());
		commandInterface.set({meshnode_, {"mesh"}}, mesh_);
		commandInterface.set({meshnodeInPrefab_, {"mesh"}}, mesh_);

		commandInterface.moveScenegraphChildren({meshnode_}, rootnode_);
		commandInterface.moveScenegraphChildren({meshnodeInPrefab_}, prefab_);

		commandInterface.set(ValueHandle{rootnode_, &Node::tags_}, std::vector<std::string>{"rntag1", "rntag2"});
		commandInterface.set(ValueHandle{meshnode_, &MeshNode::tags_}, std::vector<std::string>{"tag1", "tag2"});
		commandInterface.set(ValueHandle{renderLayer_, &RenderLayer::tags_}, std::vector<std::string>{"tag1", "tag2"});
		commandInterface.set(ValueHandle{material_, &Material::tags_}, std::vector<std::string>{"mtag1"});
		commandInterface.set(ValueHandle{prefabInstance_, &PrefabInstance::template_}, prefab_);
		commandInterface.set(ValueHandle{prefabInstance_, &PrefabInstance::tags_}, std::vector<std::string>{"pitag1", "pitag2"});
		commandInterface.set(ValueHandle{meshnodeInPrefab_, &MeshNode::tags_}, std::vector<std::string>{"mnptag1", "mnptag2"});
	}

	SNode rootnode_;
	SRenderLayer renderLayer_;
	SMaterial material_;
	SMeshNode meshnode_;
	SMesh mesh_;
	SPrefab prefab_;
	SPrefabInstance prefabInstance_;
	SMeshNode meshnodeInPrefab_;
};

TEST_F(QueriesTagTest, isUserTypeInTypeList) {
	EXPECT_EQ(Queries::isUserTypeInTypeList(meshnode_, Queries::UserTypesWithRenderableTags{}), true);
	EXPECT_EQ(Queries::isUserTypeInTypeList(mesh_, Queries::UserTypesWithRenderableTags{}), false);
	EXPECT_EQ(Queries::isUserTypeInTypeList(nullptr, Queries::UserTypesWithRenderableTags{}), false);
}

TEST_F(QueriesTagTest, isTagProperty) {
	EXPECT_EQ(Queries::isTagProperty(ValueHandle{meshnode_, &MeshNode::tags_}), true);
	EXPECT_EQ(Queries::isTagProperty(ValueHandle{meshnode_, &MeshNode::materials_}), false);
	EXPECT_EQ(Queries::isTagProperty(ValueHandle{renderLayer_, &RenderLayer::renderableTags_}), false);
	EXPECT_EQ(Queries::isTagProperty(ValueHandle{renderLayer_, &RenderLayer::materialFilterTags_}), false);
	EXPECT_EQ(Queries::isTagProperty(ValueHandle{material_, &Material::tags_}), true);
}

TEST_F(QueriesTagTest, renderableTags) {
	EXPECT_EQ(Queries::renderableTags(rootnode_), std::set<std::string>({"rntag1", "rntag2"}));
	EXPECT_EQ(Queries::renderableTags(meshnode_), std::set<std::string>({"tag1", "tag2"}));
	EXPECT_EQ(Queries::renderableTagsWithParentTags(meshnode_), std::set<std::string>({"rntag1", "rntag2", "tag1", "tag2"}));
	EXPECT_EQ(Queries::renderableTags(renderLayer_), std::set<std::string>({"tag1", "tag2"}));
	EXPECT_EQ(Queries::renderableTags(material_).size(), 0);
	EXPECT_EQ(Queries::renderableTags(nullptr).size(), 0);
	
	EXPECT_EQ(Queries::renderableTagsWithParentTags(prefabInstance_), std::set<std::string>({"pitag1", "pitag2"}));
	EXPECT_EQ(Queries::renderableTagsWithParentTags(meshnodeInPrefab_), std::set<std::string>({"mnptag1", "mnptag2"}));
	const auto meshnodeInPrefabInstance = raco::select<MeshNode>(prefabInstance_->children_->asVector<SEditorObject>());
	EXPECT_EQ(Queries::renderableTagsWithParentTags(meshnodeInPrefabInstance), std::set<std::string>({"mnptag1", "mnptag2", "pitag1", "pitag2"}));
}

TEST_F(QueriesTagTest, materialTags) {
	EXPECT_EQ(Queries::materialTags(material_), std::set<std::string>({"mtag1"}));
	EXPECT_EQ(Queries::materialTags(meshnode_).size(), 0);
	EXPECT_EQ(Queries::materialTags(nullptr).size(), 0);
}
	
TEST_F(QueriesTagTest, isMeshNodeInMaterialFilter) {
	EXPECT_EQ(Queries::isMeshNodeInMaterialFilter(meshnode_, {}, false), false);
	EXPECT_EQ(Queries::isMeshNodeInMaterialFilter(meshnode_, {}, true), true);
	commandInterface.set(ValueHandle{meshnode_, {"materials", "material", "material"}}, material_);
	EXPECT_EQ(Queries::isMeshNodeInMaterialFilter(meshnode_, {}, false), false);
	EXPECT_EQ(Queries::isMeshNodeInMaterialFilter(meshnode_, {}, true), true);
	EXPECT_EQ(Queries::isMeshNodeInMaterialFilter(meshnode_, {"mtag1"}, true), false);
	EXPECT_EQ(Queries::isMeshNodeInMaterialFilter(meshnode_, {"mtag1"}, false), true);
	EXPECT_EQ(Queries::isMeshNodeInMaterialFilter(meshnode_, {"nomtag1"}, true), true);
	EXPECT_EQ(Queries::isMeshNodeInMaterialFilter(meshnode_, {"nomtag1"}, false), false);
}

TEST_F(QueriesTagTest, hasObjectTag) {
	EXPECT_EQ(Queries::hasObjectTag(meshnode_, "tag1"), true);
	EXPECT_EQ(Queries::hasObjectTag(meshnode_, "notag1"), false);
	EXPECT_EQ(Queries::hasObjectTag(SMeshNode{}, "notag1"), false);
}

TEST_F(QueriesTagTest, hasObjectAnyTag) {
	EXPECT_EQ(Queries::hasObjectAnyTag(meshnode_, {"tag1"}), true);
	EXPECT_EQ(Queries::hasObjectAnyTag(meshnode_, {"tag3", "tag1"}), true);
	EXPECT_EQ(Queries::hasObjectAnyTag(meshnode_, {"tag4"}), false);
	EXPECT_EQ(Queries::hasObjectAnyTag(meshnode_, {}), false);
	EXPECT_EQ(Queries::hasObjectAnyTag(SMeshNode{}, {}), false);
}
