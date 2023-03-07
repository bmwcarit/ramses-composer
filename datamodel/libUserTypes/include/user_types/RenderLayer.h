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

#include "core/EngineInterface.h"
#include "user_types/BaseObject.h"
#include "user_types/DefaultValues.h"
#include "user_types/Enumerations.h"

namespace raco::user_types {

class RenderLayer : public BaseObject {
public:
	static inline const TypeDescriptor typeDescription = {"RenderLayer", true};
	TypeDescriptor const& getTypeDescription() const override {
		return typeDescription;
	}

	RenderLayer(RenderLayer const& other) : BaseObject(other), tags_(other.tags_), renderableTags_(other.renderableTags_), materialFilterTags_(other.materialFilterTags_), materialFilterMode_(other.materialFilterMode_), sortOrder_(other.sortOrder_) {
		fillPropertyDescription();
	}

	RenderLayer(std::string name = std::string(), std::string id = std::string())
		: BaseObject(name, id) {
		fillPropertyDescription();
	}

	void fillPropertyDescription() {
		properties_.emplace_back("tags", &tags_);
		properties_.emplace_back("renderableTags", &renderableTags_);
		properties_.emplace_back("materialFilterTags", &materialFilterTags_);
		properties_.emplace_back("materialFilterMode", &materialFilterMode_);
		properties_.emplace_back("sortOrder", &sortOrder_);
	}

	std::vector<std::string> renderableTags() const {
		return renderableTags_->propertyNames();
	}

	std::set<std::string> materialFilterTags() const {
		std::set<std::string> r; 
		for (int i = 0; i < materialFilterTags_->size(); ++i) {
			r.emplace(materialFilterTags_->get(i)->asString());
		}
		return r;
	}

	bool isRendering(std::set<std::string> const& tags) const {
		for (std::size_t i = 0; i < renderableTags_->size(); ++i) {
			if (tags.find(renderableTags_->name(i)) != tags.end()) {
				return true;
			}
		}
		return false;
	}

	Property<Table, ArraySemanticAnnotation, HiddenProperty, TagContainerAnnotation, DisplayNameAnnotation> tags_{{}, {}, {}, {}, {"Tags"}};

	// contains tag name -> order index map as normal Value<int> properties
	// - property name is the tag name
	// - property value is the order index 
	Property<Table, RenderableTagContainerAnnotation, DisplayNameAnnotation> renderableTags_{{}, {}, {"Renderable Tags"}};

	Property<Table, ArraySemanticAnnotation, HiddenProperty, TagContainerAnnotation, DisplayNameAnnotation> materialFilterTags_{{}, {}, {}, {}, {"Material Filter Tags"}};
	Property<int, DisplayNameAnnotation, EnumerationAnnotation> materialFilterMode_{static_cast<int>(ERenderLayerMaterialFilterMode::Exclusive), {"Material Filter Mode"}, EUserTypeEnumerations::RenderLayerMaterialFilterMode};

	Property<int, DisplayNameAnnotation, EnumerationAnnotation> sortOrder_{static_cast<int>(ERenderLayerOrder::Manual), {"Render Order"}, EUserTypeEnumerations::RenderLayerOrder};
};

using SRenderLayer = std::shared_ptr<RenderLayer>;

}  // namespace raco::user_types
