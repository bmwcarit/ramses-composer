/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "core/Queries_Tags.h"

#include "user_types/LuaScript.h"
#include "user_types/MeshNode.h"
#include "user_types/Node.h"
#include "user_types/Prefab.h"
#include "user_types/PrefabInstance.h"
#include "user_types/RenderPass.h"

namespace raco::core {

namespace {

template <typename TaggedObjectTypeList>
std::set<std::string> tagsForTypes(SEditorObject const& obj, TaggedObjectTypeList const&) {
	std::set<std::string> r;
	if (Queries::isUserTypeInTypeList(obj, TaggedObjectTypeList{})) {
		core::Table const& tagContainer = obj->get(std::string("tags"))->asTable();
		for (auto tagIndex = 0; tagIndex < tagContainer.size(); ++tagIndex) {
			std::string const& tag = tagContainer.get(tagIndex)->asString();
			r.insert(tag);
		}
	}
	return r;
}

}  // namespace

bool Queries::isTagProperty(ValueHandle const& handle) {
	return handle.getPropName() == "tags";
}

bool Queries::isTagContainerProperty(const ValueHandle& handle) {
	return handle.query<core::UserTagContainerAnnotation>() || handle.query<core::TagContainerAnnotation>() || handle.query<core::RenderableTagContainerAnnotation>();
}

std::set<std::string> Queries::renderableTags(SEditorObject const& obj) {
	return tagsForTypes(obj, UserTypesWithRenderableTags{});
}

std::set<std::string> Queries::materialTags(SEditorObject const& obj) {
	return tagsForTypes(obj, UserTypesWithMaterialTags{});
}

std::set<std::string> Queries::renderableTagsWithParentTags(SEditorObject obj) {
	if (obj == nullptr) {
		return {};
	}
	std::set<std::string> tags;
	while (obj != nullptr) {
		tags.merge(renderableTags(obj));
		auto node = obj->as<user_types::Node>();
		obj = node == nullptr ? nullptr : node->getParent();
	}
	return tags;
}

bool Queries::isMeshNodeInMaterialFilter(user_types::SMeshNode const& obj, std::set<std::string> const& materialFilterTags, bool materialFilterExclusive) {
	bool matFilterDiscarded = false;

	if (!materialFilterTags.empty()) {
		auto meshnode = obj->as<raco::user_types::MeshNode>();
		auto material = meshnode->getMaterial(0);

		if (!material && !materialFilterExclusive) {
			matFilterDiscarded = true;
		} else {
			if (material) {
				bool matHasTag = core::Queries::hasObjectAnyTag(material, materialFilterTags);

				if (materialFilterExclusive && matHasTag || !materialFilterExclusive && !matHasTag) {
					matFilterDiscarded = true;
				}
			} else {
				// !material && materialFilterExclusive -> keep
			}
		}
	} else {
		matFilterDiscarded = !materialFilterExclusive;
	}
	return !matFilterDiscarded;
}

void Queries::findForbiddenTags(const TagDataCache& tagDataCache, SEditorObject const& object, std::set<std::string>& outForbiddenTags) {
	if (auto const renderLayer = object->as<user_types::RenderLayer>()) {
		tagDataCache.collectRenderableTagsFromRenderLayerChildren(renderLayer, outForbiddenTags);
	}
}

void Queries::findForbiddenTags(const Project& project, SEditorObject const& object, std::set<std::string>& outForbiddenTags) {
	auto tagDataCache = TagDataCache::createTagDataCache(&project, TagType::NodeTags_Referenced);
	Queries::findForbiddenTags(*tagDataCache, object, outForbiddenTags);
}

void Queries::findRenderLayerForbiddenRenderableTags(const TagDataCache& tagDataCache, user_types::SRenderLayer const& renderLayer, std::set<std::string>& outForbiddenTags) {
	tagDataCache.collectAppliedTagsFromRenderLayerParents(renderLayer, outForbiddenTags);
}

void Queries::findRenderLayerForbiddenRenderableTags(const Project& project, user_types::SRenderLayer const& renderLayer, std::set<std::string>& outForbiddenTags) {
	auto tagDataCache = TagDataCache::createTagDataCache(&project, TagType::NodeTags_Referenced);
	Queries::findRenderLayerForbiddenRenderableTags(*tagDataCache, renderLayer, outForbiddenTags);
}

}  // namespace raco::core
