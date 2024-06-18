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

#include "object_tree_view_model/ObjectTreeViewRenderViewModel.h"

#include "user_types/BlitPass.h"
#include "user_types/MeshNode.h"
#include "user_types/RenderLayer.h"
#include "user_types/RenderPass.h"
#include "user_types/RenderTarget.h"

using namespace raco::user_types;
using namespace raco::object_tree::model;

class ObjectTreeViewRenderViewModelTest : public ObjectTreeViewDefaultModelTest {
public:
	ObjectTreeViewRenderViewModelTest() : ObjectTreeViewDefaultModelTest() {
		viewModel_.reset(new object_tree::model::ObjectTreeViewRenderViewModel(&commandInterface(), application.dataChangeDispatcher(), nullptr));

		node = create<Node>("root", nullptr, {"render_main"});
		mesh = create_mesh("mesh", "meshes/Duck.glb");
		material = create_material("material", "shaders/simple_texture.vert", "shaders/simple_texture.frag", {});
		material_def = create<Material>("material_def", nullptr, {"mat_default"});
		material_alt = create<Material>("material_alt", nullptr, {"mat_alt"});
		meshnode = create_meshnode("meshnode", mesh, material, node);
		meshnode_def = create_meshnode("meshnode_def", mesh, material_def, node);
		meshnode_alt = create_meshnode("meshnode_alt", mesh, material_alt, node);
		buffer_1 = create<RenderBuffer>("buffer_1");
		buffer_2 = create<RenderBuffer>("buffer_2");
		target = create_rendertarget("render_target", {buffer_1});
		layer = create_layer("layer", {}, {{"render_main", 7}});
		renderpass = create_renderpass("render_pass", {}, {layer}, 2);
		dispatch(true);
	}

	void dispatch(bool rebuildTree = false) {
		application.dataChangeDispatcher()->dispatch(recorder().release());
		if (rebuildTree) {
			viewModel_->buildObjectTree();
		}
	}

	void checkTree(QModelIndex index, ObjectTreeNode* node) {
		auto modelNode = viewModel_->indexToTreeNode(index);
		std::map<ObjectTreeNodeType, std::string> nodeTypeNames = {
			{ObjectTreeNodeType::EditorObject, "EditorObject"},
			{ObjectTreeNodeType::Root, "Root"},
			{ObjectTreeNodeType::Tag, "Tag"}};
		std::string desc = fmt::format("Node type = {},  Object type = {},  Name = {}",
			nodeTypeNames.at(node->getType()), node->getDisplayType(), node->getDisplayName());

		EXPECT_EQ(modelNode->getRepresentedObject(), node->getRepresentedObject()) << desc;
		EXPECT_EQ(modelNode->getType(), node->getType()) << desc;
		EXPECT_EQ(modelNode->getTypeName(), node->getTypeName()) << desc;
		EXPECT_EQ(modelNode->getRenderOrder(), node->getRenderOrder()) << desc;
		EXPECT_EQ(modelNode->getInputBuffers(), node->getInputBuffers()) << desc;
		EXPECT_EQ(modelNode->getOutputBuffers(), node->getOutputBuffers()) << desc;
		EXPECT_EQ(modelNode->childCount(), node->childCount()) << desc;
		for (size_t row = 0; row < node->childCount(); row++) {
			checkTree(viewModel_->index(row, 0, index), node->getChild(row));
		}
	}

	ObjectTreeNode* makeNode(core::SEditorObject obj, const std::vector<ObjectTreeNode*>& children = {}) {
		ObjectTreeNode* node = obj ? new ObjectTreeNode(obj, {}) : new ObjectTreeNode(ObjectTreeNodeType::Root, {});
		for (auto child : children) {
			node->addChild(child);
		}
		return node;
	}

	ObjectTreeNode* makeRenderPassNode(core::SEditorObject obj, int renderOrder, const std::string& inputBuffers, const std::string& outputBuffers, const std::vector<ObjectTreeNode*>& children = {}) {
		ObjectTreeNode* node = new ObjectTreeNode(obj, {});
		node->setRenderOrder(renderOrder);
		node->setBuffers(inputBuffers, outputBuffers);
		for (auto child : children) {
			node->addChild(child);
		}
		return node;
	}

	ObjectTreeNode* makeTagNode(const std::string& tag, int renderOrder, const std::vector<ObjectTreeNode*>& children = {}) {
		auto node = new ObjectTreeNode(ObjectTreeNodeType::Tag, tag, {});
		node->setRenderOrder(renderOrder);
		for (auto child : children) {
			node->addChild(child);
		}
		return node;
	}

	SNode node;
	SMesh mesh;
	SMaterial material;
	SMaterial material_def;
	SMaterial material_alt;
	SMeshNode meshnode;
	SMeshNode meshnode_def;
	SMeshNode meshnode_alt;
	SRenderBuffer buffer_1;
	SRenderBuffer buffer_2;
	SRenderTarget target;
	SRenderLayer layer;
	SRenderPass renderpass;
};

TEST_F(ObjectTreeViewRenderViewModelTest, build_top_level) {
	commandInterface().set(ValueHandle(renderpass, &RenderPass::target_), target);
	commandInterface().set(ValueHandle(renderpass, &RenderPass::layers_)[0], SEditorObject());
	auto blitpass = create_blitpass("blit_pass", 1, buffer_1, buffer_2);
	dispatch(true);

	checkTree({}, makeNode({}, {makeRenderPassNode(renderpass, 2, {}, "buffer_1"), makeRenderPassNode(blitpass, 1, "buffer_1", "buffer_2")}));
}

TEST_F(ObjectTreeViewRenderViewModelTest, build_layer_sortorder_tag) {
	checkTree(viewModel_->index(0, 0),
		makeRenderPassNode(renderpass, 2, {}, "<Framebuffer>", {makeNode(layer, {makeTagNode("render_main", 7, {makeNode(node, {makeNode(meshnode), makeNode(meshnode_def), makeNode(meshnode_alt)})})})}));
}

TEST_F(ObjectTreeViewRenderViewModelTest, build_layer_sortorder_scenegraph) {
	commandInterface().set({layer, &RenderLayer::sortOrder_}, static_cast<int>(ERenderLayerOrder::SceneGraph));
	dispatch(true);

	checkTree(viewModel_->index(0, 0),
		makeRenderPassNode(renderpass, 2, {}, "<Framebuffer>", {makeNode(layer, {makeNode(node, {makeNode(meshnode), makeNode(meshnode_def), makeNode(meshnode_alt)})})}));
}

TEST_F(ObjectTreeViewRenderViewModelTest, build_material_filter_inclusive) {
	commandInterface().setTags({layer, &user_types::RenderLayer::materialFilterTags_}, {"mat_default"});
	commandInterface().set({layer, &user_types::RenderLayer::materialFilterMode_}, static_cast<int>(user_types::ERenderLayerMaterialFilterMode::Inclusive));
	dispatch(true);

	checkTree(viewModel_->index(0, 0),
		makeRenderPassNode(renderpass, 2, {}, "<Framebuffer>", {makeNode(layer, {makeTagNode("render_main", 7, {makeNode(node, {makeNode(meshnode_def)})})})}));
}

TEST_F(ObjectTreeViewRenderViewModelTest, build_material_filter_exclusive) {
	commandInterface().setTags({layer, &user_types::RenderLayer::materialFilterTags_}, {"mat_default"});
	commandInterface().set({layer, &user_types::RenderLayer::materialFilterMode_}, static_cast<int>(user_types::ERenderLayerMaterialFilterMode::Exclusive));
	dispatch(true);

	checkTree(viewModel_->index(0, 0),
		makeRenderPassNode(renderpass, 2, {}, "<Framebuffer>", {makeNode(layer, {makeTagNode("render_main", 7, {makeNode(node, {makeNode(meshnode), makeNode(meshnode_alt)})})})}));
}

TEST_F(ObjectTreeViewRenderViewModelTest, build_input_buffer_shared_material) {
	commandInterface().set(ValueHandle(material, &Material::uniforms_).get("u_Tex"), buffer_1);
	dispatch(true);

	checkTree(viewModel_->index(0, 0),
		makeRenderPassNode(renderpass, 2, "buffer_1", "<Framebuffer>", {makeNode(layer, {makeTagNode("render_main", 7, {makeNode(node, {makeNode(meshnode), makeNode(meshnode_def), makeNode(meshnode_alt)})})})}));
}

TEST_F(ObjectTreeViewRenderViewModelTest, build_input_buffer_private_material_private_uniform) {
	commandInterface().set(meshnode->getMaterialPrivateHandle(0), true);
	commandInterface().set(meshnode->getUniformContainerHandle(0).get("u_Tex"), buffer_1);
	dispatch(true);

	checkTree(viewModel_->index(0, 0),
		makeRenderPassNode(renderpass, 2, "buffer_1", "<Framebuffer>", {makeNode(layer, {makeTagNode("render_main", 7, {makeNode(node, {makeNode(meshnode), makeNode(meshnode_def), makeNode(meshnode_alt)})})})}));
}

TEST_F(ObjectTreeViewRenderViewModelTest, build_input_buffer_private_material_shared_uniform) {
	commandInterface().set(meshnode->getMaterialPrivateHandle(0), true);
	commandInterface().set(ValueHandle(material, &Material::uniforms_).get("u_Tex"), buffer_1);
	dispatch(true);

	checkTree(viewModel_->index(0, 0),
		makeRenderPassNode(renderpass, 2, {}, "<Framebuffer>", {makeNode(layer, {makeTagNode("render_main", 7, {makeNode(node, {makeNode(meshnode), makeNode(meshnode_def), makeNode(meshnode_alt)})})})}));
}

TEST_F(ObjectTreeViewRenderViewModelTest, update_create_blitpass) {
	commandInterface().set(ValueHandle(renderpass, &RenderPass::target_), target);
	commandInterface().set(ValueHandle(renderpass, &RenderPass::layers_)[0], SEditorObject());
	auto blitpass = create_blitpass("blit_pass", 1, buffer_1, buffer_2);
	dispatch(true);

	checkTree({}, makeNode({}, {makeRenderPassNode(renderpass, 2, {}, "buffer_1"), makeRenderPassNode(blitpass, 1, "buffer_1", "buffer_2")}));

	auto blitpass_2 = create_blitpass("blit_pass_2", 3, buffer_2, buffer_1);
	dispatch();

	checkTree({}, makeNode({}, {makeRenderPassNode(renderpass, 2, {}, "buffer_1"),
								   makeRenderPassNode(blitpass, 1, "buffer_1", "buffer_2"),
								   makeRenderPassNode(blitpass_2, 3, "buffer_2", "buffer_1")}));
}

TEST_F(ObjectTreeViewRenderViewModelTest, update_delete_renderpass) {
	commandInterface().set(ValueHandle(renderpass, &RenderPass::target_), target);
	commandInterface().set(ValueHandle(renderpass, &RenderPass::layers_)[0], SEditorObject());
	auto blitpass = create_blitpass("blit_pass", 1, buffer_1, buffer_2);
	dispatch(true);

	checkTree({}, makeNode({}, {makeRenderPassNode(renderpass, 2, {}, "buffer_1"), makeRenderPassNode(blitpass, 1, "buffer_1", "buffer_2")}));

	commandInterface().deleteObjects({renderpass});
	dispatch();

	checkTree({}, makeNode({}, {makeRenderPassNode(blitpass, 1, "buffer_1", "buffer_2")}));
}

TEST_F(ObjectTreeViewRenderViewModelTest, update_delete_buffer) {
	commandInterface().set(ValueHandle(renderpass, &RenderPass::target_), target);
	commandInterface().set(ValueHandle(renderpass, &RenderPass::layers_)[0], SEditorObject());
	auto blitpass = create_blitpass("blit_pass", 1, buffer_1, buffer_2);
	dispatch(true);

	checkTree({}, makeNode({}, {makeRenderPassNode(renderpass, 2, {}, "buffer_1"), makeRenderPassNode(blitpass, 1, "buffer_1", "buffer_2")}));

	commandInterface().deleteObjects({buffer_1});
	dispatch();

	checkTree({}, makeNode({}, {makeRenderPassNode(renderpass, 2, {}, {}), makeRenderPassNode(blitpass, 1, {}, "buffer_2")}));
}

TEST_F(ObjectTreeViewRenderViewModelTest, update_create_meshnode_root) {
	checkTree(viewModel_->index(0, 0),
		makeRenderPassNode(renderpass, 2, {}, "<Framebuffer>", {makeNode(layer, {makeTagNode("render_main", 7, {makeNode(node, {makeNode(meshnode), makeNode(meshnode_def), makeNode(meshnode_alt)})})})}));

	auto meshnode_new = create_meshnode("meshnode_new", mesh, material, {}, {"render_main"});
	dispatch(false);

	checkTree(viewModel_->index(0, 0),
		makeRenderPassNode(renderpass, 2, {}, "<Framebuffer>", {makeNode(layer, {makeTagNode("render_main", 7, {makeNode(node, {makeNode(meshnode), makeNode(meshnode_def), makeNode(meshnode_alt)}), makeNode(meshnode_new)})})}));
}

TEST_F(ObjectTreeViewRenderViewModelTest, update_delete_meshnode_root) {
	auto meshnode_new = create_meshnode("meshnode_new", mesh, material, {}, {"render_main"});
	dispatch(true);

	checkTree(viewModel_->index(0, 0),
		makeRenderPassNode(renderpass, 2, {}, "<Framebuffer>", {makeNode(layer, {makeTagNode("render_main", 7, {makeNode(node, {makeNode(meshnode), makeNode(meshnode_def), makeNode(meshnode_alt)}), makeNode(meshnode_new)})})}));

	commandInterface().deleteObjects({meshnode_new});
	dispatch();

	checkTree(viewModel_->index(0, 0),
		makeRenderPassNode(renderpass, 2, {}, "<Framebuffer>", {makeNode(layer, {makeTagNode("render_main", 7, {makeNode(node, {makeNode(meshnode), makeNode(meshnode_def), makeNode(meshnode_alt)})})})}));
}

TEST_F(ObjectTreeViewRenderViewModelTest, update_blitpass_change_render_order) {
	commandInterface().deleteObjects({renderpass});
	auto blitpass = create_blitpass("blit_pass", 1, buffer_1, buffer_2);
	dispatch(true);

	checkTree({}, makeNode({}, {makeRenderPassNode(blitpass, 1, "buffer_1", "buffer_2")}));

	commandInterface().set(ValueHandle(blitpass, &BlitPass::renderOrder_), 5);
	dispatch();

	checkTree({}, makeNode({}, {makeRenderPassNode(blitpass, 5, "buffer_1", "buffer_2")}));
}


TEST_F(ObjectTreeViewRenderViewModelTest, update_blitpass_change_source_buffer) {
	commandInterface().deleteObjects({renderpass});
	auto blitpass = create_blitpass("blit_pass", 1, buffer_1, buffer_2);
	dispatch(true);

	checkTree({}, makeNode({}, {makeRenderPassNode(blitpass, 1, "buffer_1", "buffer_2")}));

	commandInterface().set(ValueHandle(blitpass, &BlitPass::sourceRenderBuffer_), SEditorObject());
	dispatch();

	checkTree({}, makeNode({}, {makeRenderPassNode(blitpass, 1, {}, "buffer_2")}));

	commandInterface().set(ValueHandle(blitpass, &BlitPass::sourceRenderBuffer_), buffer_1);
	dispatch();

	checkTree({}, makeNode({}, {makeRenderPassNode(blitpass, 1, "buffer_1", "buffer_2")}));
}

TEST_F(ObjectTreeViewRenderViewModelTest, update_blitpass_change_target_buffer) {
	commandInterface().deleteObjects({renderpass});
	auto blitpass = create_blitpass("blit_pass", 1, buffer_1, buffer_2);
	dispatch(true);

	checkTree({}, makeNode({}, {makeRenderPassNode(blitpass, 1, "buffer_1", "buffer_2")}));

	commandInterface().set(ValueHandle(blitpass, &BlitPass::targetRenderBuffer_), SEditorObject());
	dispatch();

	checkTree({}, makeNode({}, {makeRenderPassNode(blitpass, 1, "buffer_1", {})}));

	commandInterface().set(ValueHandle(blitpass, &BlitPass::targetRenderBuffer_), buffer_2);
	dispatch();

	checkTree({}, makeNode({}, {makeRenderPassNode(blitpass, 1, "buffer_1", "buffer_2")}));
}

TEST_F(ObjectTreeViewRenderViewModelTest, update_renderpass_change_target) {
	checkTree(viewModel_->index(0, 0),
		makeRenderPassNode(renderpass, 2, {}, "<Framebuffer>", {makeNode(layer, {makeTagNode("render_main", 7, {makeNode(node, {makeNode(meshnode), makeNode(meshnode_def), makeNode(meshnode_alt)})})})}));

	commandInterface().set(ValueHandle(renderpass, &RenderPass::target_), target);
	dispatch();

	checkTree(viewModel_->index(0, 0),
		makeRenderPassNode(renderpass, 2, {}, "buffer_1", {makeNode(layer, {makeTagNode("render_main", 7, {makeNode(node, {makeNode(meshnode), makeNode(meshnode_def), makeNode(meshnode_alt)})})})}));

	commandInterface().set(ValueHandle(renderpass, &RenderPass::target_), SEditorObject());
	dispatch();

	checkTree(viewModel_->index(0, 0),
		makeRenderPassNode(renderpass, 2, {}, "<Framebuffer>", {makeNode(layer, {makeTagNode("render_main", 7, {makeNode(node, {makeNode(meshnode), makeNode(meshnode_def), makeNode(meshnode_alt)})})})}));
}

TEST_F(ObjectTreeViewRenderViewModelTest, update_renderpass_change_render_order) {
	checkTree(viewModel_->index(0, 0),
		makeRenderPassNode(renderpass, 2, {}, "<Framebuffer>", {makeNode(layer, {makeTagNode("render_main", 7, {makeNode(node, {makeNode(meshnode), makeNode(meshnode_def), makeNode(meshnode_alt)})})})}));

	commandInterface().set(ValueHandle(renderpass, &RenderPass::renderOrder_), 13);
	dispatch();

	checkTree(viewModel_->index(0, 0),
		makeRenderPassNode(renderpass, 13, {}, "<Framebuffer>", {makeNode(layer, {makeTagNode("render_main", 7, {makeNode(node, {makeNode(meshnode), makeNode(meshnode_def), makeNode(meshnode_alt)})})})}));
}

TEST_F(ObjectTreeViewRenderViewModelTest, update_layer_change_sortorder) {
	checkTree(viewModel_->index(0, 0),
		makeRenderPassNode(renderpass, 2, {}, "<Framebuffer>", {makeNode(layer, {makeTagNode("render_main", 7, {makeNode(node, {makeNode(meshnode), makeNode(meshnode_def), makeNode(meshnode_alt)})})})}));

	commandInterface().set({layer, &RenderLayer::sortOrder_}, static_cast<int>(ERenderLayerOrder::SceneGraph));
	dispatch();

	checkTree(viewModel_->index(0, 0),
		makeRenderPassNode(renderpass, 2, {}, "<Framebuffer>", {makeNode(layer, {makeNode(node, {makeNode(meshnode), makeNode(meshnode_def), makeNode(meshnode_alt)})})}));
}

TEST_F(ObjectTreeViewRenderViewModelTest, update_layer_change_material_filter_mode) {
	commandInterface().setTags({layer, &user_types::RenderLayer::materialFilterTags_}, {"mat_default"});
	dispatch(true);

	checkTree(viewModel_->index(0, 0),
		makeRenderPassNode(renderpass, 2, {}, "<Framebuffer>", {makeNode(layer, {makeTagNode("render_main", 7, {makeNode(node, {makeNode(meshnode), makeNode(meshnode_alt)})})})}));

	commandInterface().set({layer, &user_types::RenderLayer::materialFilterMode_}, static_cast<int>(user_types::ERenderLayerMaterialFilterMode::Inclusive));
	dispatch();

	checkTree(viewModel_->index(0, 0),
		makeRenderPassNode(renderpass, 2, {}, "<Framebuffer>", {makeNode(layer, {makeTagNode("render_main", 7, {makeNode(node, {makeNode(meshnode_def)})})})}));
}

TEST_F(ObjectTreeViewRenderViewModelTest, update_layer_change_material_filter_tags) {
	checkTree(viewModel_->index(0, 0),
		makeRenderPassNode(renderpass, 2, {}, "<Framebuffer>", {makeNode(layer, {makeTagNode("render_main", 7, {makeNode(node, {makeNode(meshnode), makeNode(meshnode_def), makeNode(meshnode_alt)})})})}));

	commandInterface().setTags({layer, &user_types::RenderLayer::materialFilterTags_}, {"mat_default"});
	dispatch();

	checkTree(viewModel_->index(0, 0),
		makeRenderPassNode(renderpass, 2, {}, "<Framebuffer>", {makeNode(layer, {makeTagNode("render_main", 7, {makeNode(node, {makeNode(meshnode), makeNode(meshnode_alt)})})})}));
}

TEST_F(ObjectTreeViewRenderViewModelTest, update_layer_change_renderable_tags_replace) {
	checkTree(viewModel_->index(0, 0),
		makeRenderPassNode(renderpass, 2, {}, "<Framebuffer>", {makeNode(layer, {makeTagNode("render_main", 7, {makeNode(node, {makeNode(meshnode), makeNode(meshnode_def), makeNode(meshnode_alt)})})})}));

	commandInterface().setRenderableTags({layer, &RenderLayer::renderableTags_}, {{"render_main", 2}});
	dispatch();

	checkTree(viewModel_->index(0, 0),
		makeRenderPassNode(renderpass, 2, {}, "<Framebuffer>", {makeNode(layer, {makeTagNode("render_main", 2, {makeNode(node, {makeNode(meshnode), makeNode(meshnode_def), makeNode(meshnode_alt)})})})}));

	commandInterface().setRenderableTags({layer, &RenderLayer::renderableTags_}, {{"render_alt", 3}});
	dispatch();

	checkTree(viewModel_->index(0, 0),
		makeRenderPassNode(renderpass, 2, {}, "<Framebuffer>", {makeNode(layer, {makeTagNode("render_alt", 3, {})})}));
}

TEST_F(ObjectTreeViewRenderViewModelTest, update_layer_change_renderable_tags_priority) {
	checkTree(viewModel_->index(0, 0),
		makeRenderPassNode(renderpass, 2, {}, "<Framebuffer>", {makeNode(layer, {makeTagNode("render_main", 7, {makeNode(node, {makeNode(meshnode), makeNode(meshnode_def), makeNode(meshnode_alt)})})})}));

	commandInterface().set(ValueHandle{layer, &RenderLayer::renderableTags_}.get("render_main"), 2);
	dispatch();

	checkTree(viewModel_->index(0, 0),
		makeRenderPassNode(renderpass, 2, {}, "<Framebuffer>", {makeNode(layer, {makeTagNode("render_main", 2, {makeNode(node, {makeNode(meshnode), makeNode(meshnode_def), makeNode(meshnode_alt)})})})}));
}

TEST_F(ObjectTreeViewRenderViewModelTest, update_renderpass_change_layers) {
	checkTree(viewModel_->index(0, 0),
		makeRenderPassNode(renderpass, 2, {}, "<Framebuffer>", {makeNode(layer, {makeTagNode("render_main", 7, {makeNode(node, {makeNode(meshnode), makeNode(meshnode_def), makeNode(meshnode_alt)})})})}));

	commandInterface().set(ValueHandle(renderpass, &user_types::RenderPass::layers_)[0], SEditorObject());
	dispatch();

	checkTree(viewModel_->index(0, 0),
		makeRenderPassNode(renderpass, 2, {}, "<Framebuffer>", {}));
}

TEST_F(ObjectTreeViewRenderViewModelTest, update_meshnode_change_material) {
	commandInterface().setTags({layer, &user_types::RenderLayer::materialFilterTags_}, {"mat_default"});
	commandInterface().set({layer, &user_types::RenderLayer::materialFilterMode_}, static_cast<int>(user_types::ERenderLayerMaterialFilterMode::Inclusive));
	dispatch(true);

	checkTree(viewModel_->index(0, 0),
		makeRenderPassNode(renderpass, 2, {}, "<Framebuffer>", {makeNode(layer, {makeTagNode("render_main", 7, {makeNode(node, {makeNode(meshnode_def)})})})}));

	commandInterface().set(meshnode->getMaterialHandle(0), material_def);
	dispatch();

	checkTree(viewModel_->index(0, 0),
		makeRenderPassNode(renderpass, 2, {}, "<Framebuffer>", {makeNode(layer, {makeTagNode("render_main", 7, {makeNode(node, {makeNode(meshnode), makeNode(meshnode_def)})})})}));
}

TEST_F(ObjectTreeViewRenderViewModelTest, update_meshnode_change_private) {
	commandInterface().set(ValueHandle(material, &Material::uniforms_).get("u_Tex"), buffer_1);
	commandInterface().set(meshnode->getMaterialPrivateHandle(0), true);
	commandInterface().set(meshnode->getUniformContainerHandle(0).get("u_Tex"), buffer_2);
	dispatch(true);

	checkTree(viewModel_->index(0, 0),
		makeRenderPassNode(renderpass, 2, "buffer_2", "<Framebuffer>", {makeNode(layer, {makeTagNode("render_main", 7, {makeNode(node, {makeNode(meshnode), makeNode(meshnode_def), makeNode(meshnode_alt)})})})}));

	commandInterface().set(meshnode->getMaterialPrivateHandle(0), false);

	dispatch();
	checkTree(viewModel_->index(0, 0),
		makeRenderPassNode(renderpass, 2, "buffer_1", "<Framebuffer>", {makeNode(layer, {makeTagNode("render_main", 7, {makeNode(node, {makeNode(meshnode), makeNode(meshnode_def), makeNode(meshnode_alt)})})})}));
}

TEST_F(ObjectTreeViewRenderViewModelTest, update_input_buffer_shared_material) {
	checkTree(viewModel_->index(0, 0),
		makeRenderPassNode(renderpass, 2, {}, "<Framebuffer>", {makeNode(layer, {makeTagNode("render_main", 7, {makeNode(node, {makeNode(meshnode), makeNode(meshnode_def), makeNode(meshnode_alt)})})})}));

	commandInterface().set(ValueHandle(material, &Material::uniforms_).get("u_Tex"), buffer_1);
	dispatch();

	checkTree(viewModel_->index(0, 0),
		makeRenderPassNode(renderpass, 2, "buffer_1", "<Framebuffer>", {makeNode(layer, {makeTagNode("render_main", 7, {makeNode(node, {makeNode(meshnode), makeNode(meshnode_def), makeNode(meshnode_alt)})})})}));
}

TEST_F(ObjectTreeViewRenderViewModelTest, update_input_buffer_private_material_private_uniform) {
	commandInterface().set(meshnode->getMaterialPrivateHandle(0), true);
	dispatch(true);

	checkTree(viewModel_->index(0, 0),
		makeRenderPassNode(renderpass, 2, {}, "<Framebuffer>", {makeNode(layer, {makeTagNode("render_main", 7, {makeNode(node, {makeNode(meshnode), makeNode(meshnode_def), makeNode(meshnode_alt)})})})}));

	commandInterface().set(meshnode->getUniformContainerHandle(0).get("u_Tex"), buffer_1);
	dispatch();

	checkTree(viewModel_->index(0, 0),
		makeRenderPassNode(renderpass, 2, "buffer_1", "<Framebuffer>", {makeNode(layer, {makeTagNode("render_main", 7, {makeNode(node, {makeNode(meshnode), makeNode(meshnode_def), makeNode(meshnode_alt)})})})}));
}

TEST_F(ObjectTreeViewRenderViewModelTest, update_node_change_tags) {
	checkTree(viewModel_->index(0, 0),
		makeRenderPassNode(renderpass, 2, {}, "<Framebuffer>", {makeNode(layer, {makeTagNode("render_main", 7, {makeNode(node, {makeNode(meshnode), makeNode(meshnode_def), makeNode(meshnode_alt)})})})}));

	commandInterface().setTags({node, &user_types::Node::tags_}, {"render_alt"});
	dispatch();

	checkTree(viewModel_->index(0, 0),
		makeRenderPassNode(renderpass, 2, {}, "<Framebuffer>", {makeNode(layer, {makeTagNode("render_main", 7, {})})}));
}

