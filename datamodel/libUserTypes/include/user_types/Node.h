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

#include "user_types/BaseObject.h"

#include "user_types/LuaScript.h"
#include "core/Link.h"

namespace raco::user_types {

class Node;
using SNode = std::shared_ptr<Node>;
using WNode = std::weak_ptr<Node>;

class Node : public BaseObject {
public:
	static inline const TypeDescriptor typeDescription = {"Node", false};
	TypeDescriptor const& getTypeDescription() const override {
		return typeDescription;
	}

	Node(const Node& other) : BaseObject(other), tags_(other.tags_), visible_(other.visible_), translation_(other.translation_),
		rotation_(other.rotation_), scale_(other.scale_)
	{
		fillPropertyDescription();
	}

	Node(std::string name = std::string(), std::string id = std::string())
		: BaseObject(name, id) {
		fillPropertyDescription();
	}

	void fillPropertyDescription() {
		properties_.emplace_back("tags", &tags_);
		properties_.emplace_back("visible", &visible_ );
		properties_.emplace_back("translation", &translation_);
		properties_.emplace_back("rotation", &rotation_);
		properties_.emplace_back("scale", &scale_);
	}

	void onAfterContextActivated(BaseContext& context) override {
		context.updateBrokenLinkErrors(shared_from_this());
	}

	void onAfterValueChanged(BaseContext& context, ValueHandle const& value) override {
		if (value.isRefToProp(&Node::objectName_)) {
			context.updateBrokenLinkErrors(shared_from_this());
		}
	}

	Property<Table, ArraySemanticAnnotation, TagContainerAnnotation, DisplayNameAnnotation> tags_{{}, {}, {}, {"Tags"}};

	Property<bool, DisplayNameAnnotation, LinkEndAnnotation> visible_{true, DisplayNameAnnotation("Visible"), {}};

	Property<Vec3f, DisplayNameAnnotation, LinkEndAnnotation> translation_{Vec3f(0.0, 1.0, -100, 100 ), DisplayNameAnnotation("Translation"), {}};
	Property<Vec3f, DisplayNameAnnotation, LinkEndAnnotation> scale_{Vec3f(1.0, 0.1, 0.1, 100 ), DisplayNameAnnotation("Scaling"), {}};
	Property<Vec3f, DisplayNameAnnotation, LinkEndAnnotation> rotation_{Vec3f(0.0, 5.0, -360.0, 360.0), DisplayNameAnnotation("Rotation"), {}};
};

template <auto T> struct property_name { static constexpr std::string_view value {}; };
template <> struct property_name<&Node::visible_> { static constexpr std::string_view value { "visible" }; };
template <> struct property_name<&Node::translation_> { static constexpr std::string_view value { "translation" }; };
template <> struct property_name<&Node::rotation_> { static constexpr std::string_view value { "rotation" }; };
template <> struct property_name<&Node::scale_> { static constexpr std::string_view value { "scale" }; };

}
