/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include <gtest/gtest.h>

#include "RamsesBaseFixture.h"
#include "user_types/MeshNode.h"
#include "user_types/Node.h"
#include "user_types/RenderLayer.h"
#include "user_types/Enumerations.h"

using raco::user_types::Material;
using raco::user_types::MeshNode;
using raco::user_types::Node;
using raco::user_types::RenderLayer;

class RenderLayerAdaptorTest : public RamsesBaseFixture<> {
protected:
	int32_t getSortOrder(const ramses::RenderGroup& group, const ramses::MeshNode& meshnode) {
		int32_t order;
		auto status = group.getMeshNodeOrder(meshnode, order);
		EXPECT_EQ(status, ramses::StatusOK);
		return order;
	}

	void set_renderables(raco::user_types::SRenderLayer layer, const std::vector<std::pair<std::string,int>>& renderables) {
		context.removeAllProperties({layer, {"renderableTags"}});
		for (int index = 0; index < renderables.size(); index++) {
			context.addProperty({layer, {"renderableTags"}}, renderables[index].first, std::make_unique<raco::data_storage::Value<int>>(renderables[index].second));
		}
	}

};

TEST_F(RenderLayerAdaptorTest, renderables_meshnode_root) {
	auto meshnode = create<MeshNode>("meshnode", nullptr, {"render_main"});
	auto layer = create_layer("layer", {}, {{"render_main",0}});

	dispatch();

	auto engineMeshNode = select<ramses::MeshNode>(*sceneContext.scene(), "meshnode");
	auto engineGroup = select<ramses::RenderGroup>(*sceneContext.scene(), "layer");

	ASSERT_TRUE(engineGroup->containsMeshNode(*engineMeshNode));
}

TEST_F(RenderLayerAdaptorTest, renderables_meshnode_child) {
	auto root = create<Node>("root", nullptr, {"render_main"});
	auto meshnode = create<MeshNode>("meshnode", root);
	auto layer = create_layer("layer", {}, {{"render_main",0}});

	dispatch();

	auto engineMeshNode = select<ramses::MeshNode>(*sceneContext.scene(), "meshnode");
	auto engineGroup = select<ramses::RenderGroup>(*sceneContext.scene(), "layer");

	ASSERT_TRUE(engineGroup->containsMeshNode(*engineMeshNode));
}

TEST_F(RenderLayerAdaptorTest, renderables_meshnode_multi) {
	auto meshnode1 = create<MeshNode>("meshnode1", nullptr, {"render_main"});
	auto meshnode2 = create<MeshNode>("meshnode2");
	auto layer = create_layer("layer", {}, {{"render_main",0}});

	dispatch();

	auto engineMeshNode1 = select<ramses::MeshNode>(*sceneContext.scene(), "meshnode1");
	auto engineMeshNode2 = select<ramses::MeshNode>(*sceneContext.scene(), "meshnode2");
	auto engineGroup = select<ramses::RenderGroup>(*sceneContext.scene(), "layer");

	ASSERT_TRUE(engineGroup->containsMeshNode(*engineMeshNode1));
	ASSERT_FALSE(engineGroup->containsMeshNode(*engineMeshNode2));
}

TEST_F(RenderLayerAdaptorTest, renderables_meshnode_root_add_node_tag) {
	auto meshnode = create<MeshNode>("meshnode");
	auto layer = create_layer("layer", {}, {{"render_main",0}});

	dispatch();

	auto engineMeshNode = select<ramses::MeshNode>(*sceneContext.scene(), "meshnode");
	auto engineGroup = select<ramses::RenderGroup>(*sceneContext.scene(), "layer");

	ASSERT_FALSE(engineGroup->containsMeshNode(*engineMeshNode));

	context.set({meshnode, {"tags"}}, std::vector<std::string>({"render_main"}));
	dispatch();
	ASSERT_TRUE(engineGroup->containsMeshNode(*engineMeshNode));
}

TEST_F(RenderLayerAdaptorTest, renderables_meshnode_root_add_layer_renderable) {
	auto meshnode = create<MeshNode>("meshnode", nullptr, {"render_main"});
	auto layer = create<RenderLayer>("layer");

	dispatch();

	auto engineMeshNode = select<ramses::MeshNode>(*sceneContext.scene(), "meshnode");
	auto engineGroup = select<ramses::RenderGroup>(*sceneContext.scene(), "layer");

	ASSERT_FALSE(engineGroup->containsMeshNode(*engineMeshNode));

	context.addProperty({layer, {"renderableTags"}}, "render_main", std::make_unique<raco::data_storage::Value<int>>(0));
	dispatch();
	ASSERT_TRUE(engineGroup->containsMeshNode(*engineMeshNode));
}

TEST_F(RenderLayerAdaptorTest, renderables_meshnode_move_scenegraph_child) {
	auto root = create<Node>("root", nullptr, {"render_main"});
	auto meshNode = create<MeshNode>("meshnode", root);
	auto layer = create_layer("layer", {}, {{"render_main", 0}});

	dispatch();

	auto engineMeshNode = select<ramses::MeshNode>(*sceneContext.scene(), "meshnode");
	auto engineGroup = select<ramses::RenderGroup>(*sceneContext.scene(), "layer");

	ASSERT_TRUE(engineGroup->containsMeshNode(*engineMeshNode));

	context.moveScenegraphChildren({meshNode}, nullptr);

	dispatch();
	ASSERT_FALSE(engineGroup->containsMeshNode(*engineMeshNode));

	context.moveScenegraphChildren({meshNode}, root);

	dispatch();
	ASSERT_TRUE(engineGroup->containsMeshNode(*engineMeshNode));
}

TEST_F(RenderLayerAdaptorTest, matfilter_toggle_invert) {
	auto root = create<Node>("root", nullptr, {"render_main"});
	auto mesh = create_mesh("mesh", "meshes/Duck.glb");
	auto material = create<Material>("material", nullptr, {});
	auto material_def = create<Material>("material_def", nullptr, {"mat_default"});
	auto material_alt = create<Material>("material_alt", nullptr, {"mat_alt"});
	auto meshnode = create_meshnode("meshnode", mesh, material, root);
	auto meshnode_def = create_meshnode("meshnode_def", mesh, material_def, root);
	auto meshnode_alt = create_meshnode("meshnode_alt", mesh, material_alt, root);
	auto layer = create_layer("layer", {}, {{"render_main",0}}, {"mat_default"}, false);

	dispatch();

	auto engineMeshNode = select<ramses::MeshNode>(*sceneContext.scene(), "meshnode");
	auto engineMeshNode_def = select<ramses::MeshNode>(*sceneContext.scene(), "meshnode_def");
	auto engineMeshNode_alt = select<ramses::MeshNode>(*sceneContext.scene(), "meshnode_alt");
	auto engineGroup = select<ramses::RenderGroup>(*sceneContext.scene(), "layer");

	ASSERT_FALSE(engineGroup->containsMeshNode(*engineMeshNode));
	ASSERT_TRUE(engineGroup->containsMeshNode(*engineMeshNode_def));
	ASSERT_FALSE(engineGroup->containsMeshNode(*engineMeshNode_alt));

	context.set({layer, {"invertMaterialFilter"}}, true);
	dispatch();

	ASSERT_TRUE(engineGroup->containsMeshNode(*engineMeshNode));
	ASSERT_FALSE(engineGroup->containsMeshNode(*engineMeshNode_def));
	ASSERT_TRUE(engineGroup->containsMeshNode(*engineMeshNode_alt));
}

TEST_F(RenderLayerAdaptorTest, matfilter_include_change_mat_tags) {
	auto root = create<Node>("root", nullptr, {"render_main"});
	auto mesh = create_mesh("mesh", "meshes/Duck.glb");
	auto material = create<Material>("material", nullptr, {});
	auto material_def = create<Material>("material_def", nullptr, {"mat_default"});
	auto material_alt = create<Material>("material_alt", nullptr, {"mat_alt"});
	auto meshnode = create_meshnode("meshnode", mesh, material, root);
	auto meshnode_def = create_meshnode("meshnode_def", mesh, material_def, root);
	auto meshnode_alt = create_meshnode("meshnode_alt", mesh, material_alt, root);
	auto layer = create_layer("layer", {}, {{"render_main",0}}, {"mat_default"}, false);

	dispatch();

	auto engineMeshNode = select<ramses::MeshNode>(*sceneContext.scene(), "meshnode");
	auto engineMeshNode_def = select<ramses::MeshNode>(*sceneContext.scene(), "meshnode_def");
	auto engineMeshNode_alt = select<ramses::MeshNode>(*sceneContext.scene(), "meshnode_alt");
	auto engineGroup = select<ramses::RenderGroup>(*sceneContext.scene(), "layer");

	ASSERT_FALSE(engineGroup->containsMeshNode(*engineMeshNode));
	ASSERT_TRUE(engineGroup->containsMeshNode(*engineMeshNode_def));
	ASSERT_FALSE(engineGroup->containsMeshNode(*engineMeshNode_alt));

	context.set({material, {"tags"}}, std::vector<std::string>({"mat_default"}));
	context.set({material_def, {"tags"}}, std::vector<std::string>({"mat_alt"}));
	context.set({material_alt, {"tags"}}, std::vector<std::string>({"mat_alt", "mat_default"}));
	dispatch();

	ASSERT_TRUE(engineGroup->containsMeshNode(*engineMeshNode));
	ASSERT_FALSE(engineGroup->containsMeshNode(*engineMeshNode_def));
	ASSERT_TRUE(engineGroup->containsMeshNode(*engineMeshNode_alt));
}

TEST_F(RenderLayerAdaptorTest, matfilter_include_change_layer_matfilter) {
	auto root = create<Node>("root", nullptr, {"render_main"});
	auto mesh = create_mesh("mesh", "meshes/Duck.glb");
	auto material = create<Material>("material", nullptr, {});
	auto material_def = create<Material>("material_def", nullptr, {"mat_default"});
	auto material_alt = create<Material>("material_alt", nullptr, {"mat_alt"});
	auto meshnode = create_meshnode("meshnode", mesh, material, root);
	auto meshnode_def = create_meshnode("meshnode_def", mesh, material_def, root);
	auto meshnode_alt = create_meshnode("meshnode_alt", mesh, material_alt, root);
	auto layer = create_layer("layer", {}, {{"render_main",0}}, {"mat_default"}, false);

	dispatch();

	auto engineMeshNode = select<ramses::MeshNode>(*sceneContext.scene(), "meshnode");
	auto engineMeshNode_def = select<ramses::MeshNode>(*sceneContext.scene(), "meshnode_def");
	auto engineMeshNode_alt = select<ramses::MeshNode>(*sceneContext.scene(), "meshnode_alt");
	auto engineGroup = select<ramses::RenderGroup>(*sceneContext.scene(), "layer");

	ASSERT_FALSE(engineGroup->containsMeshNode(*engineMeshNode));
	ASSERT_TRUE(engineGroup->containsMeshNode(*engineMeshNode_def));
	ASSERT_FALSE(engineGroup->containsMeshNode(*engineMeshNode_alt));

	context.set({layer, {"materialFilterTags"}}, std::vector<std::string>({"mat_alt"}));
	dispatch();

	ASSERT_FALSE(engineGroup->containsMeshNode(*engineMeshNode));
	ASSERT_FALSE(engineGroup->containsMeshNode(*engineMeshNode_def));
	ASSERT_TRUE(engineGroup->containsMeshNode(*engineMeshNode_alt));
}

TEST_F(RenderLayerAdaptorTest, matfilter_exclude_change_mat_tags) {
	auto root = create<Node>("root", nullptr, {"render_main"});
	auto mesh = create_mesh("mesh", "meshes/Duck.glb");
	auto material = create<Material>("material", nullptr, {});
	auto material_def = create<Material>("material_def", nullptr, {"mat_default"});
	auto material_alt = create<Material>("material_alt", nullptr, {"mat_alt"});
	auto meshnode = create_meshnode("meshnode", mesh, material, root);
	auto meshnode_def = create_meshnode("meshnode_def", mesh, material_def, root);
	auto meshnode_alt = create_meshnode("meshnode_alt", mesh, material_alt, root);
	auto layer = create_layer("layer", {}, {{"render_main",0}}, {"mat_default"}, true);

	dispatch();

	auto engineMeshNode = select<ramses::MeshNode>(*sceneContext.scene(), "meshnode");
	auto engineMeshNode_def = select<ramses::MeshNode>(*sceneContext.scene(), "meshnode_def");
	auto engineMeshNode_alt = select<ramses::MeshNode>(*sceneContext.scene(), "meshnode_alt");
	auto engineGroup = select<ramses::RenderGroup>(*sceneContext.scene(), "layer");

	ASSERT_TRUE(engineGroup->containsMeshNode(*engineMeshNode));
	ASSERT_FALSE(engineGroup->containsMeshNode(*engineMeshNode_def));
	ASSERT_TRUE(engineGroup->containsMeshNode(*engineMeshNode_alt));

	context.set({material, {"tags"}}, std::vector<std::string>({"mat_default"}));
	context.set({material_def, {"tags"}}, std::vector<std::string>({"mat_alt"}));
	context.set({material_alt, {"tags"}}, std::vector<std::string>({"mat_alt", "mat_default"}));
	dispatch();

	ASSERT_FALSE(engineGroup->containsMeshNode(*engineMeshNode));
	ASSERT_TRUE(engineGroup->containsMeshNode(*engineMeshNode_def));
	ASSERT_FALSE(engineGroup->containsMeshNode(*engineMeshNode_alt));
}

TEST_F(RenderLayerAdaptorTest, matfilter_exclude_change_layer_matfilter) {
	auto root = create<Node>("root", nullptr, {"render_main"});
	auto mesh = create_mesh("mesh", "meshes/Duck.glb");
	auto material = create<Material>("material", nullptr, {});
	auto material_def = create<Material>("material_def", nullptr, {"mat_default"});
	auto material_alt = create<Material>("material_alt", nullptr, {"mat_alt"});
	auto meshnode = create_meshnode("meshnode", mesh, material, root);
	auto meshnode_def = create_meshnode("meshnode_def", mesh, material_def, root);
	auto meshnode_alt = create_meshnode("meshnode_alt", mesh, material_alt, root);
	auto layer = create_layer("layer", {}, {{"render_main",0}}, {"mat_default"}, true);

	dispatch();

	auto engineMeshNode = select<ramses::MeshNode>(*sceneContext.scene(), "meshnode");
	auto engineMeshNode_def = select<ramses::MeshNode>(*sceneContext.scene(), "meshnode_def");
	auto engineMeshNode_alt = select<ramses::MeshNode>(*sceneContext.scene(), "meshnode_alt");
	auto engineGroup = select<ramses::RenderGroup>(*sceneContext.scene(), "layer");

	ASSERT_TRUE(engineGroup->containsMeshNode(*engineMeshNode));
	ASSERT_FALSE(engineGroup->containsMeshNode(*engineMeshNode_def));
	ASSERT_TRUE(engineGroup->containsMeshNode(*engineMeshNode_alt));

	context.set({layer, {"materialFilterTags"}}, std::vector<std::string>({"mat_alt"}));
	dispatch();

	ASSERT_TRUE(engineGroup->containsMeshNode(*engineMeshNode));
	ASSERT_TRUE(engineGroup->containsMeshNode(*engineMeshNode_def));
	ASSERT_FALSE(engineGroup->containsMeshNode(*engineMeshNode_alt));
}

TEST_F(RenderLayerAdaptorTest, matfilter_nested_toggle_invert) {
	auto root = create<Node>("root", nullptr, {"render_main"});
	auto mesh = create_mesh("mesh", "meshes/Duck.glb");
	auto material = create<Material>("material", nullptr, {});
	auto material_def = create<Material>("material_def", nullptr, {"mat_default"});
	auto material_alt = create<Material>("material_alt", nullptr, {"mat_alt"});
	auto meshnode = create_meshnode("meshnode", mesh, material, root);
	auto meshnode_def = create_meshnode("meshnode_def", mesh, material_def, meshnode);
	auto meshnode_alt = create_meshnode("meshnode_alt", mesh, material_alt, meshnode_def);
	auto layer = create_layer("layer", {}, {{"render_main",0}}, {"mat_default"}, true);

	dispatch();

	auto engineMeshNode = select<ramses::MeshNode>(*sceneContext.scene(), "meshnode");
	auto engineMeshNode_def = select<ramses::MeshNode>(*sceneContext.scene(), "meshnode_def");
	auto engineMeshNode_alt = select<ramses::MeshNode>(*sceneContext.scene(), "meshnode_alt");
	auto engineGroup = select<ramses::RenderGroup>(*sceneContext.scene(), "layer");

	ASSERT_TRUE(engineGroup->containsMeshNode(*engineMeshNode));
	ASSERT_FALSE(engineGroup->containsMeshNode(*engineMeshNode_def));
	ASSERT_TRUE(engineGroup->containsMeshNode(*engineMeshNode_alt));

	context.set({layer, {"invertMaterialFilter"}}, true);
	dispatch();

	ASSERT_TRUE(engineGroup->containsMeshNode(*engineMeshNode));
	ASSERT_FALSE(engineGroup->containsMeshNode(*engineMeshNode_def));
	ASSERT_TRUE(engineGroup->containsMeshNode(*engineMeshNode_alt));
}

TEST_F(RenderLayerAdaptorTest, nested_simple) {
	auto root = create<Node>("root", nullptr, {"render_main"});
	auto meshnode = create<MeshNode>("meshnode", root);
	auto layer = create_layer("layer", {}, {{"render_main",0}});
	auto layer_n = create_layer("layer_n", {"render_main"}, {{"render_nested",0}});

	dispatch();

	auto engineMeshNode = select<ramses::MeshNode>(*sceneContext.scene(), "meshnode");
	auto engineGroup = select<ramses::RenderGroup>(*sceneContext.scene(), "layer");
	auto engineGroup_n = select<ramses::RenderGroup>(*sceneContext.scene(), "layer_n");

	ASSERT_TRUE(engineGroup->containsMeshNode(*engineMeshNode));
	ASSERT_TRUE(engineGroup->containsRenderGroup(*engineGroup_n));
}

TEST_F(RenderLayerAdaptorTest, nested_fail_self_loop) {
	auto root = create<Node>("root", nullptr, {"render_main"});
	auto meshnode = create<MeshNode>("meshnode", root);
	auto layer = create_layer("layer", {"render_main"}, {{"render_main",0}});

	dispatch();

	auto engineMeshNode = select<ramses::MeshNode>(*sceneContext.scene(), "meshnode");
	auto engineGroup = select<ramses::RenderGroup>(*sceneContext.scene(), "layer");

	ASSERT_TRUE(engineGroup->containsMeshNode(*engineMeshNode));
	ASSERT_FALSE(engineGroup->containsRenderGroup(*engineGroup));
}

TEST_F(RenderLayerAdaptorTest, nested_fail_child_direct_loop) {
	auto root = create<Node>("root", nullptr, {"render_main"});
	auto meshnode = create<MeshNode>("meshnode", root);
	auto layer = create_layer("layer", {"LOOP"}, {{"render_main",0}});
	auto layer_n = create_layer("layer_n", {"render_main"}, {{"LOOP",0}});

	dispatch();

	auto engineMeshNode = select<ramses::MeshNode>(*sceneContext.scene(), "meshnode");
	auto engineGroup = select<ramses::RenderGroup>(*sceneContext.scene(), "layer");
	auto engineGroup_n = select<ramses::RenderGroup>(*sceneContext.scene(), "layer_n");

	ASSERT_TRUE(engineGroup->containsMeshNode(*engineMeshNode));
	ASSERT_FALSE(engineGroup->containsRenderGroup(*engineGroup_n));

	set_renderables(layer_n, {{"FOO", 0}});
	dispatch();

	ASSERT_TRUE(engineGroup->containsMeshNode(*engineMeshNode));
	ASSERT_TRUE(engineGroup->containsRenderGroup(*engineGroup_n));
}

TEST_F(RenderLayerAdaptorTest, nested_fail_child_indirect_loop) {
	auto root = create<Node>("root", nullptr, {"render_main"});
	auto meshnode = create<MeshNode>("meshnode", root);
	auto layer = create_layer("layer", {"LOOP"}, {{"render_main",0}});
	auto layer_a = create_layer("layer_a", {"render_main"}, {{"render_nest",0}});
	auto layer_b = create_layer("layer_b", {"render_nest"}, {{"LOOP",0}});

	dispatch();

	auto engineMeshNode = select<ramses::MeshNode>(*sceneContext.scene(), "meshnode");
	auto engineGroup = select<ramses::RenderGroup>(*sceneContext.scene(), "layer");
	auto engineGroup_a = select<ramses::RenderGroup>(*sceneContext.scene(), "layer_a");
	auto engineGroup_b = select<ramses::RenderGroup>(*sceneContext.scene(), "layer_b");

	ASSERT_TRUE(engineGroup->containsMeshNode(*engineMeshNode));
	ASSERT_FALSE(engineGroup->containsRenderGroup(*engineGroup_a));
	ASSERT_FALSE(engineGroup->containsRenderGroup(*engineGroup_b));

	set_renderables(layer_b, {{"FOO", 0}});
	dispatch();

	ASSERT_TRUE(engineGroup->containsMeshNode(*engineMeshNode));
	ASSERT_TRUE(engineGroup->containsRenderGroup(*engineGroup_a));
	ASSERT_FALSE(engineGroup->containsRenderGroup(*engineGroup_b));
}

TEST_F(RenderLayerAdaptorTest, sortorder_manual) {
	auto root = create<Node>("root", nullptr, {"render_main"});
	auto meshnode1 = create<MeshNode>("meshnode1", root);
	auto meshnode2 = create<MeshNode>("meshnode2", nullptr, {"render_alt"});
	auto meshnode3 = create<MeshNode>("meshnode3", root);
	auto layer = create_layer("layer", {}, {{"render_main", 0}, {"render_alt", 1}});
	context.set({layer, {"sortOrder"}}, static_cast<int>(raco::user_types::ERenderLayerOrder::Manual));

	dispatch();

	auto engineMeshNode1 = select<ramses::MeshNode>(*sceneContext.scene(), "meshnode1");
	auto engineMeshNode2 = select<ramses::MeshNode>(*sceneContext.scene(), "meshnode2");
	auto engineMeshNode3 = select<ramses::MeshNode>(*sceneContext.scene(), "meshnode3");
	auto engineGroup = select<ramses::RenderGroup>(*sceneContext.scene(), "layer");

	ASSERT_TRUE(engineGroup->containsMeshNode(*engineMeshNode1));
	ASSERT_TRUE(engineGroup->containsMeshNode(*engineMeshNode2));
	ASSERT_TRUE(engineGroup->containsMeshNode(*engineMeshNode3));
	ASSERT_EQ(getSortOrder(*engineGroup, *engineMeshNode1), 0);
	ASSERT_EQ(getSortOrder(*engineGroup, *engineMeshNode2), 1);
	ASSERT_EQ(getSortOrder(*engineGroup, *engineMeshNode3), 0);

	set_renderables(layer, {{"render_alt", 0}, {"render_main", 1}});
	dispatch();

	ASSERT_TRUE(engineGroup->containsMeshNode(*engineMeshNode1));
	ASSERT_TRUE(engineGroup->containsMeshNode(*engineMeshNode2));
	ASSERT_TRUE(engineGroup->containsMeshNode(*engineMeshNode3));
	ASSERT_EQ(getSortOrder(*engineGroup, *engineMeshNode1), 1);
	ASSERT_EQ(getSortOrder(*engineGroup, *engineMeshNode2), 0);
	ASSERT_EQ(getSortOrder(*engineGroup, *engineMeshNode3), 1);
}

TEST_F(RenderLayerAdaptorTest, sortorder_scenegraph) {
	auto root = create<Node>("root", nullptr, {"render_main"});
	auto meshnode1 = create<MeshNode>("meshnode1", root);
	auto meshnode2 = create<MeshNode>("meshnode2", nullptr, {"render_alt"});
	auto meshnode3 = create<MeshNode>("meshnode3", root);
	auto layer = create_layer("layer", {}, {{"render_main", 0}, {"render_alt", 1}});
	context.set({layer, {"sortOrder"}}, static_cast<int>(raco::user_types::ERenderLayerOrder::SceneGraph));

	dispatch();

	auto engineMeshNode1 = select<ramses::MeshNode>(*sceneContext.scene(), "meshnode1");
	auto engineMeshNode2 = select<ramses::MeshNode>(*sceneContext.scene(), "meshnode2");
	auto engineMeshNode3 = select<ramses::MeshNode>(*sceneContext.scene(), "meshnode3");
	auto engineGroup = select<ramses::RenderGroup>(*sceneContext.scene(), "layer");

	ASSERT_TRUE(engineGroup->containsMeshNode(*engineMeshNode1));
	ASSERT_TRUE(engineGroup->containsMeshNode(*engineMeshNode2));
	ASSERT_TRUE(engineGroup->containsMeshNode(*engineMeshNode3));
	ASSERT_EQ(getSortOrder(*engineGroup, *engineMeshNode1), 1);
	ASSERT_EQ(getSortOrder(*engineGroup, *engineMeshNode2), 3);
	ASSERT_EQ(getSortOrder(*engineGroup, *engineMeshNode3), 2);

	set_renderables(layer, {{"render_alt", 0}, {"render_main", 1}});
	dispatch();

	// Priorities are ignored for scene graph sorted render layers.
	ASSERT_TRUE(engineGroup->containsMeshNode(*engineMeshNode1));
	ASSERT_TRUE(engineGroup->containsMeshNode(*engineMeshNode2));
	ASSERT_TRUE(engineGroup->containsMeshNode(*engineMeshNode3));
	ASSERT_EQ(getSortOrder(*engineGroup, *engineMeshNode1), 1);
	ASSERT_EQ(getSortOrder(*engineGroup, *engineMeshNode2), 3);
	ASSERT_EQ(getSortOrder(*engineGroup, *engineMeshNode3), 2);

	context.set({layer, {"sortOrder"}}, static_cast<int>(raco::user_types::ERenderLayerOrder::Manual));
	dispatch();

	// Priorities are no longer ignored for manual sorted render layers.
	ASSERT_TRUE(engineGroup->containsMeshNode(*engineMeshNode1));
	ASSERT_TRUE(engineGroup->containsMeshNode(*engineMeshNode2));
	ASSERT_TRUE(engineGroup->containsMeshNode(*engineMeshNode3));
	auto s1 = getSortOrder(*engineGroup, *engineMeshNode1);
	auto s2 = getSortOrder(*engineGroup, *engineMeshNode2);
	auto s3 = getSortOrder(*engineGroup, *engineMeshNode3);
	ASSERT_EQ(getSortOrder(*engineGroup, *engineMeshNode1), 1);
	ASSERT_EQ(getSortOrder(*engineGroup, *engineMeshNode2), 0);
	ASSERT_EQ(getSortOrder(*engineGroup, *engineMeshNode3), 1);
}
