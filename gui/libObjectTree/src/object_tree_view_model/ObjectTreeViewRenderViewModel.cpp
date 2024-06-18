/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "object_tree_view_model/ObjectTreeViewRenderViewModel.h"

#include "core/Iterators.h"
#include "core/Queries.h"
#include "core/Queries_Tags.h"

#include "user_types/BlitPass.h"
#include "user_types/Material.h"
#include "user_types/MeshNode.h"
#include "user_types/RenderLayer.h"
#include "user_types/RenderPass.h"

namespace raco::object_tree::model {

using namespace raco::core;

ObjectTreeViewRenderViewModel::ObjectTreeViewRenderViewModel(core::CommandInterface* commandInterface, components::SDataChangeDispatcher dispatcher, core::ExternalProjectsStoreInterface* externalProjectsStore)
	: ObjectTreeViewDefaultModel(commandInterface, dispatcher, externalProjectsStore,
		  {user_types::BlitPass::typeDescription.typeName, user_types::RenderPass::typeDescription.typeName}, true) {
}

Qt::ItemFlags ObjectTreeViewRenderViewModel::flags(const QModelIndex& index) const {
	return QAbstractItemModel::flags(index);
}

std::vector<std::string> ObjectTreeViewRenderViewModel::creatableTypes(const QModelIndex& index) const {
	return {};
}

bool ObjectTreeViewRenderViewModel::canProgramaticallyGotoObject() const {
	return false;
}

std::vector<ObjectTreeViewDefaultModel::ColumnIndex> ObjectTreeViewRenderViewModel::hiddenColumns() const {
	return {COLUMNINDEX_PREVIEW_VISIBILITY, COLUMNINDEX_ABSTRACT_VIEW_VISIBILITY, COLUMNINDEX_PROJECT};
}

ObjectTreeViewDefaultModel::ColumnIndex ObjectTreeViewRenderViewModel::defaultSortColumn() const {
	return COLUMNINDEX_RENDER_ORDER;
}

void ObjectTreeViewRenderViewModel::updateSubscriptions() {
	subscriptions_.clear();

	auto setDirtyAction = [this](ValueHandle handle) {
		dirty_ = true;
	};

	// Subscription needed:
	// BlitPass -> buffers, renderOrder
	// RenderPass -> target, layers, renderOrder
	// RenderLayer -> renderableTags, materialFilterTags, materialFilterMode, sortOrder
	// Node -> tags, children
	// MeshNode -> private, material
	// any texture uniform in a MeshNode or Material

	// BlitPass
	subscriptions_.emplace_back(dispatcher_->registerOnPropertyChange("sourceRenderBuffer", setDirtyAction));
	subscriptions_.emplace_back(dispatcher_->registerOnPropertyChange("targetRenderBuffer", setDirtyAction));
	subscriptions_.emplace_back(dispatcher_->registerOnPropertyChange("sourceRenderBufferMS", setDirtyAction));
	subscriptions_.emplace_back(dispatcher_->registerOnPropertyChange("targetRenderBufferMS", setDirtyAction));
	// RenderPass
	subscriptions_.emplace_back(dispatcher_->registerOnPropertyChange("target", setDirtyAction));
	subscriptions_.emplace_back(dispatcher_->registerOnPropertyChange("renderOrder", setDirtyAction));
	// RenderLayer
	subscriptions_.emplace_back(dispatcher_->registerOnPropertyChange("materialFilterMode", setDirtyAction));
	subscriptions_.emplace_back(dispatcher_->registerOnPropertyChange("sortOrder", setDirtyAction));
	// MeshNode
	subscriptions_.emplace_back(dispatcher_->registerOnPropertyChange("private", setDirtyAction));
	subscriptions_.emplace_back(dispatcher_->registerOnPropertyChange("material", setDirtyAction));
	// Node
	subscriptions_.emplace_back(dispatcher_->registerOnPropertyChange("tags", setDirtyAction));

	for (const auto& obj : project()->instances()) {
		if (auto meshnode = obj->as<user_types::MeshNode>()) {
			if (meshnode->materialPrivate(0)) {
				for (auto const& prop : ValueTreeIteratorAdaptor(meshnode->getUniformContainerHandle(0))) {
					if (prop.type() == PrimitiveType::Ref) {
						subscriptions_.emplace_back(dispatcher_->registerOn(prop, [this]() { dirty_ = true; }));
					}
				}
			} else if (auto mat = meshnode->getMaterial(0)) {
				for (auto const& prop : ValueTreeIteratorAdaptor(ValueHandle(meshnode->getMaterial(0), &user_types::Material::uniforms_))) {
					if (prop.type() == PrimitiveType::Ref) {
						subscriptions_.emplace_back(dispatcher_->registerOn(prop, [this]() { dirty_ = true; }));
					}
				}
			}
		}

		if (auto rp = obj->as<user_types::RenderPass>()) {
			subscriptions_.emplace_back(dispatcher_->registerOnChildren({rp, &user_types::RenderPass::layers_}, setDirtyAction));
		}
		if (auto rl = obj->as<user_types::RenderLayer>()) {
			subscriptions_.emplace_back(dispatcher_->registerOnChildren({rl, &user_types::RenderLayer::renderableTags_}, setDirtyAction));
			subscriptions_.emplace_back(dispatcher_->registerOnChildren({rl, &user_types::RenderLayer::materialFilterTags_}, setDirtyAction));
		}
	}
}

void ObjectTreeViewRenderViewModel::buildObjectTree() {
	dirty_ = false;
	if (!commandInterface_) {
		return;
	}

	auto filteredEditorObjects = filterForTopLevelObjects(project()->instances());

	beginResetModel();

	resetInvisibleRootNode();
	buildRenderPasses(invisibleRootNode_.get(), filteredEditorObjects);
	updateTreeIndexes();
	updateSubscriptions();

	endResetModel();
}

std::vector<ObjectTreeNode*> ObjectTreeViewRenderViewModel::buildTaggedNodeSubtree(const std::vector<SEditorObject>& objects, const std::set<std::string>& tags, bool parentActive, const std::set<std::string>& materialFilterTags, bool materialFilterExclusive, const std::map<core::SEditorObject, core::SEditorObjectSet>& renderBufferRefs, SEditorObjectSet& inputBuffers) {
	std::vector<ObjectTreeNode*> nodes;

	for (const auto obj : objects) {
		bool objectTagged = core::Queries::hasObjectAnyTag(obj->as<user_types::Node>(), tags);
		bool active = parentActive || objectTagged;

		auto currentChildNodes = buildTaggedNodeSubtree(obj->children_->asVector<core::SEditorObject>(), tags, active, materialFilterTags, materialFilterExclusive, renderBufferRefs, inputBuffers);

		bool addObject = active && obj->isType<user_types::MeshNode>() && core::Queries::isMeshNodeInMaterialFilter(obj->as<user_types::MeshNode>(), materialFilterTags, materialFilterExclusive);

		if (addObject || !currentChildNodes.empty()) {
			if (addObject) {
				auto meshNode = obj->as<user_types::MeshNode>();
				auto it = renderBufferRefs.find(meshNode->materialPrivate(0) ? obj : meshNode->getMaterial(0));
				if (it != renderBufferRefs.end()) {
					for (const auto obj : it->second) {
						inputBuffers.insert(obj);
					}
				}
			}

			auto node = new ObjectTreeNode(obj, nullptr);
			for (auto childNode : currentChildNodes) {
				node->addChild(childNode);
			}
			nodes.emplace_back(node);
		}
	}
	return nodes;
}

void ObjectTreeViewRenderViewModel::buildRenderLayerSubtree(
	ObjectTreeNode* layerNode,
	core::SEditorObject obj,
	const std::map<core::SEditorObject, core::SEditorObjectSet>& renderBufferRefs,
	const std::vector<core::SEditorObject>& topLevelNodes,
	const std::vector<user_types::SRenderLayer>& renderLayers,
	core::SEditorObjectSet& inputBuffers) {
	auto layer = obj->as<user_types::RenderLayer>();

	std::set<std::string> materialFilterTags{layer->materialFilterTags()};
	bool materialFilterExclusive = *layer->materialFilterMode_ == static_cast<int>(user_types::ERenderLayerMaterialFilterMode::Exclusive);

	bool sceneGraphOrder = static_cast<int>(user_types::ERenderLayerOrder::SceneGraph) == *layer->sortOrder_;

	if (sceneGraphOrder) {
		for (auto node : buildTaggedNodeSubtree(topLevelNodes, layer->renderableTagSet(), false, materialFilterTags, materialFilterExclusive, renderBufferRefs, inputBuffers)) {
			layerNode->addChild(node);
		}
	} else {
		for (auto tag : layer->renderableTags()) {
			auto tagNode = new ObjectTreeNode(ObjectTreeNodeType::Tag, tag, layerNode);
			tagNode->setRenderOrder(layer->renderableTags_->get(tag)->asInt());

			for (auto node : buildTaggedNodeSubtree(topLevelNodes, {tag}, false, materialFilterTags, materialFilterExclusive, renderBufferRefs, inputBuffers)) {
				tagNode->addChild(node);
			}

			for (auto childLayer : renderLayers) {
				if (core::Queries::hasObjectTag(childLayer, tag)) {
					auto childLayerNode = new ObjectTreeNode(childLayer, tagNode);
					buildRenderLayerSubtree(childLayerNode, childLayer, renderBufferRefs, topLevelNodes, renderLayers, inputBuffers);
				}
			}
		}
	}
}

void ObjectTreeViewRenderViewModel::buildRenderPassSubtree(ObjectTreeNode* node, core::SEditorObject obj, const std::map<core::SEditorObject, core::SEditorObjectSet>& renderBufferRefs, const std::vector<core::SEditorObject>& topLevelNodes, const std::vector<user_types::SRenderLayer>& renderLayers) {
	core::SEditorObjectSet inputBuffers;

	auto renderPass = obj->as<user_types::RenderPass>();
	std::vector<SEditorObject> layers;
	for (const auto& layer : renderPass->layers_->asVector<SEditorObject>()) {
		if (layer) {
			auto layerNode = new ObjectTreeNode(layer, node);
			buildRenderLayerSubtree(layerNode, layer, renderBufferRefs, topLevelNodes, renderLayers, inputBuffers);
		}
	}

	std::string inputBufferDesc;
	for (auto buffer : inputBuffers) {
		if (!inputBufferDesc.empty()) {
			inputBufferDesc += "\n";
		}
		inputBufferDesc += buffer->objectName();
	}

	std::string outputBufferDesc;
	auto target = *renderPass->target_;
	if (target) {
		if (auto targetNormal = target->as<user_types::RenderTarget>()) {
			for (auto buffer : targetNormal->buffers_->asVector<core::SEditorObject>()) {
				if (buffer) {
					if (!outputBufferDesc.empty()) {
						outputBufferDesc += "\n";
					}
					outputBufferDesc += buffer->objectName();
				}
			}
		} else {
			auto targetMS = target->as<user_types::RenderTargetMS>();
			for (auto buffer : targetMS->buffers_->asVector<core::SEditorObject>()) {
				if (buffer) {
					if (!outputBufferDesc.empty()) {
						outputBufferDesc += "\n";
					}
					outputBufferDesc += buffer->objectName();
				}
			}
		}
	} else {
		outputBufferDesc = "<Framebuffer>";
	}
	node->setBuffers(inputBufferDesc, outputBufferDesc);

}

std::map<core::SEditorObject, core::SEditorObjectSet> findRenderBufferReferences(const Project& project) {
	std::map<core::SEditorObject, core::SEditorObjectSet> refs;

	for (auto obj : project.instances()) {
		if (obj->isType<user_types::RenderBuffer>()) {
			for (const auto& weakSrcObj : obj->referencesToThis()) {
				auto srcObj = weakSrcObj.lock();
				if (srcObj) {
					refs[srcObj].insert(obj);
				}
			}
		}
	}
	return refs;
}

void ObjectTreeViewRenderViewModel::buildRenderPasses(ObjectTreeNode* rootNode, const std::vector<core::SEditorObject>& children) {
	auto renderBufferRefs = findRenderBufferReferences(*project());

	std::vector<SEditorObject> topLevelNodes;
	std::vector<user_types::SRenderLayer> renderLayers;
	for (auto const& child : project()->instances()) {
		if (!child->getParent() && child->as<user_types::Node>()) {
			topLevelNodes.emplace_back(child);
		}
		if (auto layer = child->as<user_types::RenderLayer>()) {
			renderLayers.emplace_back(layer);
		}
	}

	for (auto obj : children) {
		if (auto rp = obj->as<user_types::RenderPass>()) {
			auto node = new ObjectTreeNode(rp, rootNode);
			node->setRenderOrder(*rp->renderOrder_);
			buildRenderPassSubtree(node, rp, renderBufferRefs, topLevelNodes, renderLayers);
		}
		if (auto blitpass = obj->as<user_types::BlitPass>()) {
			auto node = new ObjectTreeNode(blitpass, rootNode);
			node->setRenderOrder(*blitpass->renderOrder_);
			
			std::string inputBuffers;
			std::string outputBuffers;
			if (*blitpass->sourceRenderBuffer_) {
				inputBuffers = (*blitpass->sourceRenderBuffer_)->objectName();
			} else if (*blitpass->sourceRenderBufferMS_) {
				inputBuffers = (*blitpass->sourceRenderBufferMS_)->objectName();
			}

			if (*blitpass->targetRenderBuffer_) {
				outputBuffers = (*blitpass->targetRenderBuffer_)->objectName();
			} else if (*blitpass->targetRenderBufferMS_) {
				outputBuffers = (*blitpass->targetRenderBufferMS_)->objectName();
			}
			node->setBuffers(inputBuffers, outputBuffers);
		}
	}
}

}  // namespace raco::object_tree::model