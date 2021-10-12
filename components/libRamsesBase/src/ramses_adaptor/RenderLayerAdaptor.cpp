/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "ramses_adaptor/RenderLayerAdaptor.h"

#include "ramses_adaptor/MeshNodeAdaptor.h"

#include "ramses_base/RamsesHandles.h"

#include "user_types/Enumerations.h"
#include "user_types/MeshNode.h"
#include "user_types/Prefab.h"

#include "core/Queries_Tags.h"

namespace raco::ramses_adaptor {

RenderLayerAdaptor::RenderLayerAdaptor(SceneAdaptor* sceneAdaptor, std::shared_ptr<user_types::RenderLayer> editorObject)
	: TypedObjectAdaptor(sceneAdaptor, editorObject, raco::ramses_base::ramsesRenderGroup(sceneAdaptor->scene())),
	  subscriptions_{sceneAdaptor->dispatcher()->registerOnObjectsLifeCycle([this](SEditorObject obj) { tagDirty(); }, [this](SEditorObject obj) { tagDirty(); }),
		  sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{editorObject, &user_types::RenderLayer::objectName_}, [this]() { tagDirty(); }),
		  sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{editorObject, &user_types::RenderLayer::renderableTags_}, [this]() { tagDirty(); }),
		  sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{editorObject, &user_types::RenderLayer::materialFilterTags_}, [this]() { tagDirty(); }),
		  sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{editorObject, &user_types::RenderLayer::invertMaterialFilter_}, [this]() { tagDirty(); }),
		  sceneAdaptor->dispatcher()->registerOn(core::ValueHandle{editorObject, &user_types::RenderLayer::sortOrder_}, [this]() { tagDirty(); }),
		  sceneAdaptor->dispatcher()->registerOnPropertyChange("children", [this](core::ValueHandle handle) { tagDirty(); }),
		  sceneAdaptor->dispatcher()->registerOnPropertyChange("tags", [this](core::ValueHandle handle) { tagDirty(); }),
		  sceneAdaptor->dispatcher()->registerOnPropertyChange("renderableTags", [this](core::ValueHandle handle) { tagDirty(); })} {
}

void RenderLayerAdaptor::buildRenderGroup(core::Errors* errors) {
	errors->removeAll(editorObject());
	
	ramsesObject().removeAllRenderables();
	ramsesObject().removeAllRenderGroups();

	ramsesObject().setName(editorObject()->objectName().c_str());


	std::vector<SEditorObject> topLevelNodes;
	std::vector<user_types::SRenderLayer> layers;
	for (auto const& child : sceneAdaptor_->project().instances()) {
		if (!child->getParent() && child->getTypeDescription().typeName != raco::user_types::Prefab::typeDescription.typeName) {
			topLevelNodes.emplace_back(child);
		}
		if (auto layer = child->as<user_types::RenderLayer>()) {
			layers.emplace_back(layer);
		}
	}

	std::set<std::string> materialTags{editorObject()->materialFilterTags()};
	
	raco::user_types::ERenderLayerOrder sortOrder = static_cast<raco::user_types::ERenderLayerOrder>(*editorObject()->sortOrder_);

	int32_t orderIndex = 0;
	for (size_t index = 0; index < editorObject()->renderableTags_->size(); index++) {
		auto const& renderableTag = editorObject()->renderableTags_->name(index);

		if (sortOrder == raco::user_types::ERenderLayerOrder::Manual) {
			orderIndex = editorObject()->renderableTags_->get(index)->asInt();
		} else if (sortOrder == raco::user_types::ERenderLayerOrder::SceneGraph) {
			orderIndex = 0; // we iterate the scene graph several times, make sure the mesh nodes are sorted by scene graph even if they are from different tags.
		}

		bool sceneGraphOrder = sortOrder == raco::user_types::ERenderLayerOrder::SceneGraph;
		buildRenderableOrder(errors, topLevelNodes, renderableTag, false, materialTags, *editorObject()->invertMaterialFilter_, orderIndex, sceneGraphOrder);
		addNestedLayers(errors, layers, renderableTag, orderIndex, sceneGraphOrder);
	}
}

void RenderLayerAdaptor::buildRenderableOrder(core::Errors* errors, std::vector<SEditorObject>& objs, const std::string& tag, bool parentActive, const std::set<std::string>& materialFilterTags, bool invertMaterialFilter, int32_t& orderIndex, bool sceneGraphOrder) {

	for (const auto& obj : objs) {
		bool currentActive = parentActive || core::Queries::hasObjectTag(obj->as<user_types::Node>(), tag);

		if (currentActive) {
			if (obj->getTypeDescription().typeName == raco::user_types::MeshNode::typeDescription.typeName) {
				if (core::Queries::isMeshNodeInMaterialFilter(obj->as<user_types::MeshNode>(), materialFilterTags, invertMaterialFilter)) {
					auto adaptor = sceneAdaptor_->lookup<MeshNodeAdaptor>(obj);
					if (ramsesObject().containsMeshNode(adaptor->getRamsesObjectPointer())) {
						if (ramsesObject().getMeshNodeOrder(adaptor->getRamsesObjectPointer()) != orderIndex) {
							const auto errorMsg = fmt::format("Mesh node '{}' has been added to render layer '{}' more than once with different priorities.", obj->objectName(), editorObject()->objectName());
							errors->addError(core::ErrorCategory::GENERAL, core::ErrorLevel::WARNING, {editorObject()->shared_from_this(), &user_types::RenderLayer::renderableTags_}, errorMsg);
							LOG_WARNING(raco::log_system::RAMSES_ADAPTOR, errorMsg);
						}
					} else {
						ramsesObject().addMeshNode(adaptor->getRamsesObjectPointer(), orderIndex);
					}
				}
			}
		}

		if (sceneGraphOrder) {
			// Make sure the orderIndex leaves all mesh nodes in scene graph order, even if they belong to different tags.
			++orderIndex;
		}

		if (obj->children_->size() > 0) {
			auto vec = obj->children_->asVector<SEditorObject>();
			buildRenderableOrder(errors, vec, tag, currentActive, materialFilterTags, invertMaterialFilter, orderIndex, sceneGraphOrder);
		}
	}
}

namespace {
bool containsLayer(const std::vector<user_types::SRenderLayer>& allLayers, user_types::SRenderLayer rootLayer, user_types::SRenderLayer queriedChild) {
	for (size_t index = 0; index < rootLayer->renderableTags_->size(); index++) {
		auto const& rootTag = rootLayer->renderableTags_->name(index);
		if (core::Queries::hasObjectTag(queriedChild, rootTag)) {
			return true;
		}
	}
	for (size_t index = 0; index < rootLayer->renderableTags_->size(); index++) {
		auto const& rootTag = rootLayer->renderableTags_->name(index);
		for (auto layer : allLayers) {
			if (core::Queries::hasObjectTag(layer, rootTag)) {
				if (containsLayer(allLayers, layer, queriedChild)) {
					return true;
				}
			}
		}
	}

	return false;
}
}

void RenderLayerAdaptor::addNestedLayers(core::Errors* errors, const std::vector<user_types::SRenderLayer>& layers, const std::string& tag, int32_t orderIndex, bool sceneGraphOrder) {
	for (auto layer : layers) {
		if (core::Queries::hasObjectTag(layer, tag)) {
			if (sceneGraphOrder) {
				const auto errorMsg = fmt::format("Render layer '{}' is using ordering by 'Scene Graph' but contains render layers. The render layers will be ignored.", editorObject()->objectName());
				errors->addError(core::ErrorCategory::GENERAL, core::ErrorLevel::ERROR, {editorObject()->shared_from_this(), &user_types::RenderLayer::sortOrder_}, errorMsg);
				LOG_ERROR(raco::log_system::RAMSES_ADAPTOR, errorMsg);
				return;
			}
			if (containsLayer(layers, layer, editorObject())) {
				const auto errorMsg = fmt::format("Render layer '{}' contains itself.", editorObject()->objectName());
				errors->addError(core::ErrorCategory::GENERAL, core::ErrorLevel::WARNING, {editorObject()->shared_from_this(), &user_types::RenderLayer::renderableTags_}, errorMsg);
				LOG_WARNING(raco::log_system::RAMSES_ADAPTOR, errorMsg);
			} else {
				if (auto adaptor = sceneAdaptor_->lookup<RenderLayerAdaptor>(layer); adaptor != nullptr) {
					if (ramsesObject().containsRenderGroup(adaptor->getRamsesObjectPointer())) {
						if (ramsesObject().getRenderGroupOrder(adaptor->getRamsesObjectPointer()) != orderIndex) {
							const auto errorMsg = fmt::format("Render layer '{}' has been added to render layer '{}' more than once with different priorities.", layer->objectName(), editorObject()->objectName());
							errors->addError(core::ErrorCategory::GENERAL, core::ErrorLevel::WARNING, {editorObject()->shared_from_this(), &user_types::RenderLayer::renderableTags_}, errorMsg);
							LOG_WARNING(raco::log_system::RAMSES_ADAPTOR, errorMsg);
						}
					} else {
						ramsesObject().addRenderGroup(adaptor->getRamsesObjectPointer(), orderIndex);
					}
				}
			}
		}
	}
}

bool RenderLayerAdaptor::sync(core::Errors* errors) {
	buildRenderGroup(errors);

	tagDirty(false);
	return true;
}

}  // namespace raco::ramses_adaptor
