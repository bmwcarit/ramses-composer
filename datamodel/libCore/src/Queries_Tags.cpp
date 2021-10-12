/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
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

bool Queries::isMeshNodeInMaterialFilter(user_types::SMeshNode const& obj, std::set<std::string> const& materialFilterTags, bool invertMaterialFilter) {
	bool matFilterDiscarded = false;

	if (!materialFilterTags.empty()) {
		auto meshnode = obj->as<raco::user_types::MeshNode>();
		auto material = meshnode->getMaterial(0);

		if (!material && !invertMaterialFilter) {
			matFilterDiscarded = true;
		} else {
			if (material) {
				bool matHasTag = core::Queries::hasObjectAnyTag(material, materialFilterTags);

				if (invertMaterialFilter && matHasTag || !invertMaterialFilter && !matHasTag) {
					matFilterDiscarded = true;
				}
			} else {
				// !material && invertMaterialFilter -> keep
			}
		}
	} else {
		matFilterDiscarded = !invertMaterialFilter;
	}
	return !matFilterDiscarded;
}

}  // namespace raco::core
