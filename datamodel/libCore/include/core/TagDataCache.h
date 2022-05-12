/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#pragma once

#include "core/EditorObject.h"

#include "user_types/RenderLayer.h"

#include <memory>
#include <map>
#include <set>
#include <string>

namespace raco::data_storage {
class Table;
}

namespace raco::user_types {
class RenderPass;
}

namespace raco::core {

class Project;

enum class TagType { MaterialTags, NodeTags_Referenced,	NodeTags_Referencing };

class TagDataCache {
	public:
		struct TagData {
			std::set<core::SEditorObject> referencingObjects_;
			std::set<core::SEditorObject> taggedObjects_;
		};

		static std::unique_ptr<TagDataCache> createTagDataCache(core::Project const* project, TagType whichTags);
		TagDataCache(core::Project const* project, TagType whichTags);

		~TagDataCache();
	
		void addReferencingObject(std::string const& tag, core::SEditorObject const& instance);
		void addTaggedObject(std::string const& tag, core::SEditorObject const& instance);

		auto begin() const {
			return tagData_.begin();
		}
		auto end() const {
			return tagData_.end();
		}

		template<class UserType>
		std::set<std::shared_ptr<UserType>> allTaggedObjects(std::string const& tag) const {
			return allObjectsWithTag<UserType, false>(tag);
		}
		template<class UserType>
		std::set<std::shared_ptr<UserType>> allReferencingObjects(std::string const& tag) const {
			return allObjectsWithTag<UserType, true>(tag);			
		}
		template <class UserType>
		std::set<std::shared_ptr<UserType>> allReferencingObjects(std::set<std::string> const& tags) const {
			std::set<std::shared_ptr<UserType>> allobjs; 
			for (auto const& tag : tags) {
				allobjs.merge(allObjectsWithTag<UserType, true>(tag));
			}
			return allobjs;
		}

		std::set<std::shared_ptr<user_types::RenderPass>> allRenderPassesForObjectWithTags(core::SEditorObject const& obj, std::set<std::string> const& tags) const;

		static std::set<std::string> tagsFromTable(data_storage::Table const& tagContainer);

		void collectRenderableTagsFromRenderLayerChildren(user_types::SRenderLayer const& renderLayer, std::set<std::string>& outAllTags) const;

		void collectAppliedTagsFromRenderLayerParents(user_types::SRenderLayer const& renderLayer, std::set<std::string>& outAllTags) const;

	private:
		TagDataCache(TagDataCache const&) = delete;
		TagDataCache& operator=(TagDataCache const&) = delete;
	
		template <typename TaggedObjectTypeList>
		void initCache(std::string_view referencingProperty);

		template <class UserType, bool referencingObjects>
		std::set<std::shared_ptr<UserType>> allObjectsWithTag(std::string const& tag) const;

		TagType whichTags_ {};
		core::Project const* project_ {};
		std::map<std::string, TagData> tagData_;
	};

	template <class UserType, bool referencingObjects>
	inline std::set<std::shared_ptr<UserType>> TagDataCache::allObjectsWithTag(std::string const& tag) const {
		std::set<std::shared_ptr<UserType>> r;
		auto const it = tagData_.find(tag);
		if (it == tagData_.end()) {
			return {};
		}
		std::set<core::SEditorObject> objectSet;
		if constexpr (referencingObjects) {
			objectSet = it->second.referencingObjects_;
		} else {
			objectSet = it->second.taggedObjects_;
		}
		for (auto const& taggedObject : objectSet) {
			std::shared_ptr<UserType> p = taggedObject->as<UserType>();
			if (p != nullptr) {
				r.emplace(p);
			}
		}
		return r;
	}

}