/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#pragma once

#include "EditorObject.h"
#include "Handles.h"
#include "TagDataCache.h"

#include <algorithm>
#include <map>
#include <tuple>

namespace raco::user_types {

class Material;
class MeshNode;
class Node;
class PrefabInstance;
class RenderLayer;

using SNode = std::shared_ptr<Node>;
using SMeshNode = std::shared_ptr<MeshNode>;
using SRenderLayer = std::shared_ptr<RenderLayer>;
}  // namespace raco::user_types

namespace raco::core {

class Project;

namespace Queries {

// All types with tags which are referenced by the render layer renderables property for an object
using UserTypesWithRenderableTags = std::tuple<
	std::shared_ptr<user_types::Node>,
	std::shared_ptr<user_types::MeshNode>,
	std::shared_ptr<user_types::PrefabInstance>,
	std::shared_ptr<user_types::RenderLayer>>;
// All types with tags which are referenced by the render layer materials property for an object
using UserTypesWithMaterialTags = std::tuple<
	std::shared_ptr<user_types::Material>>;

// Check if the object is in one of the lists above. Example:
// bool isObjWithRenderableTags = isUserTypeInTypeList(obj, UserTypesWithRenderableTags{});
template <typename... TaggedObjectTypes>
bool isUserTypeInTypeList(SEditorObject const& obj, std::tuple<std::shared_ptr<TaggedObjectTypes>...> const&) {
	if (obj == nullptr) {
		return {};
	}
	std::array<bool, sizeof...(TaggedObjectTypes)> isOfType = {obj->as<TaggedObjectTypes>() != nullptr...};
	return std::any_of(std::begin(isOfType), std::end(isOfType), [](bool b) { return b; });
}

// true if the handle points to a property storing tags applied to an object.
bool isTagProperty(ValueHandle const& handle);

// Check if a property carries any of the various types of TagContainerAnnotations 
bool isTagContainerProperty(const ValueHandle& handle);


// Return all tags for an object which are referenced by the render layer renderables property
std::set<std::string> renderableTags(SEditorObject const& obj);

// Return all tags for an object which are referenced by the render layer materials property
std::set<std::string> materialTags(SEditorObject const& obj);

// Return all tags relevant for an object which are referenced by the render layer renderables property.
// Nodes in inherit all tags from their parent nodes.
std::set<std::string> renderableTagsWithParentTags(SEditorObject node);

// Check if a mesh node will pass through the material filter.
bool isMeshNodeInMaterialFilter(user_types::SMeshNode const& obj, std::set<std::string> const& materialFilterTags, bool materialFilterExclusive);

// Check if an object is tagged with the given tag
template <typename T>
bool hasObjectTag(std::shared_ptr<T> const& obj, std::string_view tag) {
	if (obj == nullptr) {
		return false;
	}
	const auto tags = obj->tags_->template asVector<std::string>();
	return std::find(tags.begin(), tags.end(), tag) != tags.end();
}

// Check if an object is tagged with any of the tags in tagset
template <typename T>
bool hasObjectAnyTag(std::shared_ptr<T> const& obj, const std::set<std::string>& tagset) {
	if (obj == nullptr) {
		return false;
	}
	const auto tags = obj->tags_->template asVector<std::string>();
	return std::find_if(tags.begin(), tags.end(), [&tagset](std::string const& tag) { return tagset.find(tag) != tagset.end(); }) != tags.end();
}

void findForbiddenTags(const TagDataCache& tagDataCache, SEditorObject const& object, std::set<std::string>& outForbiddenTags);

void findForbiddenTags(const Project& project, SEditorObject const& object, std::set<std::string>& outForbiddenTags);

void findRenderLayerForbiddenRenderableTags(const TagDataCache& tagDataCache, user_types::SRenderLayer const& renderLayer, std::set<std::string>& outForbiddenTags);

void findRenderLayerForbiddenRenderableTags(const Project& project, user_types::SRenderLayer const& renderLayer, std::set<std::string>& outForbiddenTags);

}  // namespace Queries

}  // namespace raco::core