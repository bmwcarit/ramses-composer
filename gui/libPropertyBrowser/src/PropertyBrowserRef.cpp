/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "property_browser/PropertyBrowserRef.h"

#include "core/Queries.h"
#include "core/Project.h"
#include "property_browser/PropertyBrowserItem.h"
#include "property_browser/PropertyBrowserUtilities.h"

namespace raco::property_browser {

PropertyBrowserRef::PropertyBrowserRef(PropertyBrowserItem* parent)
	: QObject{parent},
	  parent_{parent}
 {
	std::for_each(parent_->valueHandles().begin(), parent_->valueHandles().end(), [this](const core::ValueHandle& handle) {
		subscriptions_.emplace_back(components::Subscription{parent_->dispatcher()->registerOn(handle, [this]() {
			updateIndex();
			Q_EMIT indexChanged(index_);
		})});
	});
	lifecycleSub_ = {parent->dispatcher()->registerOnObjectsLifeCycle([this](auto) { update(); }, [this](auto) { update(); })};
	childMoveSub_ = {parent->dispatcher()->registerOnPropertyChange("children", [this](core::ValueHandle handle) { update(); })};
	update();
}

const PropertyBrowserRef::RefItems& PropertyBrowserRef::items() const noexcept {
	return items_;
}

int PropertyBrowserRef::currentIndex() noexcept {
	return index_;
}

void PropertyBrowserRef::update() noexcept {
	if (parent_->isValid()) {
		updateItems();
		updateIndex();
		Q_EMIT itemsChanged(items_);
		Q_EMIT indexChanged(index_);
	}
}

QString PropertyBrowserRef::getEmptyRefDescription(const core::ValueHandle& handle) {
	if (auto expectEmptyRef = handle.query<core::ExpectEmptyReference>()) {
		if (const std::string& emptyRefLabel = expectEmptyRef->emptyRefLabel_.asString(); !emptyRefLabel.empty()) {
			return QString::fromStdString(emptyRefLabel);
		}
	}
	return "<empty>";
}

void PropertyBrowserRef::updateItems() noexcept {
	objectNames_.clear();
	items_.clear();

	// fill empty description if it's equal for all handles
	QString emptyRefDescription = getEmptyRefDescription(*parent_->valueHandles().begin());

	if (std::all_of(++parent_->valueHandles().begin(), parent_->valueHandles().end(), [this, &emptyRefDescription](const core::ValueHandle& handle) {
			return emptyRefDescription == getEmptyRefDescription(handle);
		})) {
		items_.push_back({emptyRefDescription, "", ""});
	} else {
		items_.push_back({"<empty>", "", ""});
	}

	auto validReferenceTargets = map_reduce<std::vector<core::SEditorObject>>(
		parent_->valueHandles(),
		intersection<std::vector<core::SEditorObject>>,
		[this](auto handle) {
			return sorted(core::Queries::findAllValidReferenceTargets(*parent_->project(), handle));
		});
	
	std::sort(validReferenceTargets.begin(), validReferenceTargets.end(), [](const auto& lhs, const auto& rhs) { return lhs->objectName() < rhs->objectName(); });

	// fill items and object names
	for (const auto& instance : validReferenceTargets) {
		auto projName = parent_->project()->getProjectNameForObject(instance, false);
		const auto& objName = instance->objectName();
		std::string displayObjectName;
		std::string tooltip;

		if (!projName.empty()) {
			displayObjectName = fmt::format("{} [{}]", objName, projName);
		} else {
			displayObjectName = objName;
		}

		tooltip = core::Queries::getFullObjectHierarchyPath(instance);

		items_.push_back({displayObjectName.c_str(), instance->objectID().c_str(), tooltip.c_str()});
		// This seems a little bit of an overkill
		objectNames_.push_back(parent_->dispatcher()->registerOn({instance, {"objectName"}}, [this]() { update(); }));
	}
}

void PropertyBrowserRef::updateIndex() noexcept {
	if (parent_->isValid()) {
		auto value = parent_->asRef();
		if (value.has_value()) {
			hasMultipleValues_ = false;

			auto refValue = value.value();
			if (refValue == nullptr) {
				index_ = EMPTY_REF_INDEX;
			} else {
				auto it = std::find_if(items_.begin(), items_.end(), [this, refValue](const auto& item) {
					return item.objId == refValue->objectID().c_str();
				});
				if (it == items_.end()) {
					index_ = EMPTY_REF_INDEX;
				} else {
					index_ = std::distance(items_.begin(), it);
				}
			}
		} else {
			hasMultipleValues_ = true;
			index_ = EMPTY_REF_INDEX;
		}
	}
}

bool PropertyBrowserRef::hasMultipleValues() const {
	return hasMultipleValues_;
}

void PropertyBrowserRef::setIndex(int index) noexcept {
	if (index == EMPTY_REF_INDEX) {
		parent_->set(core::SEditorObject{});
	} else {
		auto objectId = items_.at(index).objId.toStdString();
		core::SEditorObject object = parent_->project()->getInstanceByID(objectId);
		parent_->set(object);
	}
}

}  // namespace raco::property_browser
