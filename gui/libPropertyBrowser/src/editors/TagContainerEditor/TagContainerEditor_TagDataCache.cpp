/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "TagContainerEditor_TagDataCache.h"

#include "user_types/Material.h"
#include "user_types/MeshNode.h"
#include "user_types/Node.h"
#include "user_types/PrefabInstance.h"
#include "user_types/RenderLayer.h"
#include "user_types/RenderPass.h"

#include "core/Project.h"
#include "core/Queries.h"
#include "core/Queries_Tags.h"

namespace raco::property_browser {

std::unique_ptr<TagDataCache> TagDataCache::createTagDataCache(core::Project const* project, TagType whichTags) {
	auto cache = std::make_unique<TagDataCache>(project, whichTags);
	switch (whichTags) {
		case TagType::NodeTags_Referenced:
		case TagType::NodeTags_Referencing:
			cache->initCache<core::Queries::UserTypesWithRenderableTags>("renderableTags");
			break;
		case TagType::MaterialTags:
			cache->initCache<core::Queries::UserTypesWithMaterialTags>("materialFilterTags");
			break;
		default:
			assert(false);
	}
	return cache;
}

TagDataCache::~TagDataCache() {
}

TagDataCache::TagDataCache(core::Project const* project, TagType whichTags) : project_{project}, whichTags_(whichTags) {
}

template <typename TaggedObjectTypeList>
void TagDataCache::initCache(std::string_view referencingProperty) {
	assert(tagData_.empty());
	for (auto const& instance : project_->instances()) {
		if (auto referencingObject = instance->as<user_types::RenderLayer>(); referencingObject != nullptr) {
			core::Table const& tagContainer = instance->get(std::string(referencingProperty))->asTable();
			for (auto tagIndex = 0; tagIndex < tagContainer.size(); ++tagIndex) {
				if (referencingProperty == "renderableTags") {
					std::string const& tag = tagContainer.name(tagIndex);
					addReferencingObject(tag, instance);
				} else {
					std::string const& tag = tagContainer.get(tagIndex)->asString();
					addReferencingObject(tag, instance);
				}
			}
			core::ValueHandle referencingPropHandle{referencingObject, {std::string(referencingProperty)}};
		}
		if (core::Queries::isUserTypeInTypeList(instance, TaggedObjectTypeList{})) {
			core::Table const& tagContainer = instance->get(std::string("tags"))->asTable();
			for (auto tagIndex = 0; tagIndex < tagContainer.size(); ++tagIndex) {
				std::string const& tag = tagContainer.get(tagIndex)->asString();
				addTaggedObject(tag, instance);
			}
		}
	}
}

void TagDataCache::addReferencingObject(std::string const& tag, core::SEditorObject const& instance) {
	tagData_[tag].referencingObjects_.emplace(instance);
}

void TagDataCache::addTaggedObject(std::string const& tag, core::SEditorObject const& instance) {
	tagData_[tag].taggedObjects_.emplace(instance);
}

std::set<user_types::SRenderPass> TagDataCache::allRenderPassesForObjectWithTags(core::SEditorObject const& obj, std::set<std::string> const& tags) const {
	std::set<user_types::SRenderPass> r;
	std::set<std::string> newTags = tags;
	std::set<std::string> allTags = tags;
	std::set<user_types::SRenderLayer> newLayers;
	std::set<user_types::SRenderLayer> allLayers;
	do {
		// Find all layers referencing the new tags
		// Collect all new referenced tags in the new layers
		if (const auto meshNode = obj->as<user_types::MeshNode>(); meshNode != nullptr) {
			auto allNewLayers = allReferencingObjects<user_types::RenderLayer>(newTags);
			std::copy_if(allNewLayers.begin(), allNewLayers.end(), std::insert_iterator(newLayers, newLayers.end()), [&meshNode](user_types::SRenderLayer const& renderLayer) {
				return core::Queries::isMeshNodeInMaterialFilter(meshNode, renderLayer->materialFilterTags(), renderLayer->invertMaterialFilter_.asBool());
			});			
		} else {
			newLayers = allReferencingObjects<user_types::RenderLayer>(newTags);
		}
		
		for (auto const& rl : newLayers) {
			auto tags = core::Queries::renderableTags(rl);
			std::set_difference(tags.begin(), tags.end(), allTags.begin(), allTags.end(), std::insert_iterator(newTags, newTags.end()));
			allTags.insert(newTags.begin(), newTags.end());
		}
		allLayers.insert(newLayers.begin(), newLayers.end());
		newTags.clear();
		newLayers.clear();
	} while (!newLayers.empty());
	auto rps = core::Queries::filterByType<user_types::RenderPass>(project_->instances());
	for (auto const& rp : rps) {
		auto rprls = {rp->layer0_.asRef(), rp->layer1_.asRef(), rp->layer2_.asRef(), rp->layer3_.asRef(),
			rp->layer4_.asRef(), rp->layer5_.asRef(), rp->layer6_.asRef(), rp->layer7_.asRef()};
		const bool isRenderPassUsingTags = std::any_of(rprls.begin(), rprls.end(), [&layers = std::as_const(allLayers), obj](user_types::SEditorObject const& rl) {
			return rl != nullptr && (rl == obj || layers.find(rl->as<user_types::RenderLayer>()) != layers.end());
		});
		if (isRenderPassUsingTags) {
			r.insert(rp);
		}
	}
	return r;
}

std::set<std::string> TagDataCache::tagsFromTable(data_storage::Table const& tagContainer) {
	std::set<std::string> r;
	for (auto tagIndex = 0; tagIndex < tagContainer.size(); ++tagIndex) {
		r.emplace(tagContainer.get(tagIndex)->asString());
	}
	return r;
}

}
