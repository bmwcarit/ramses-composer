/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "property_browser/PropertyBrowserRef.h"

#include "core/Queries.h"
#include "core/PrefabOperations.h"
#include "core/Project.h"
#include "property_browser/PropertyBrowserItem.h"
#include "user_types/PrefabInstance.h"

namespace raco::property_browser {

PropertyBrowserRef::PropertyBrowserRef(PropertyBrowserItem* parent)
	: QObject{parent},
	  parent_{parent},
	  subscription_{parent->dispatcher()->registerOn(parent_->valueHandle(), [this]() {
		  updateIndex();
		  Q_EMIT indexChanged(index_);
	  })},
	  lifecycleSub_{parent->dispatcher()->registerOnObjectsLifeCycle([this](auto) { update(); }, [this](auto) { update(); })},
	  childMoveSub_{parent->dispatcher()->registerOnPropertyChange("children", [this](raco::core::ValueHandle handle) { update(); })}
 {
	update();
}

const PropertyBrowserRef::ComboBoxItems& PropertyBrowserRef::items() const noexcept {
	return items_;
}

int PropertyBrowserRef::currentIndex() noexcept {
	return index_;
}

void PropertyBrowserRef::update() noexcept {
	if (parent_->valueHandle()) {
		updateItems();
		updateIndex();
		Q_EMIT itemsChanged(items_);
		Q_EMIT indexChanged(index_);
	}
}

void PropertyBrowserRef::updateItems() noexcept {
	if (parent_->valueHandle()) {
		objectNames_.clear();
		items_.clear();
		QString emptyRefDescription = "<empty>";
		if (auto expectEmptyRef = parent_->valueHandle().query<core::ExpectEmptyReference>()) {
			if (const std::string& emptyRefLabel = expectEmptyRef->emptyRefLabel_.asString(); !emptyRefLabel.empty()) {
				emptyRefDescription = QString::fromStdString(emptyRefLabel);
			}
		}
		items_.push_back({emptyRefDescription, "", ""});

		auto validReferenceTargets = core::Queries::findAllValidReferenceTargets(*parent_->project(), parent_->valueHandle());
		std::sort(validReferenceTargets.begin(), validReferenceTargets.end(), [](const auto& lhs, const auto& rhs) { return lhs->objectName() < rhs->objectName(); });

		for (const auto& instance : validReferenceTargets) {
			auto projName = parent_->project()->getProjectNameForObject(instance, false);
			const auto &objName = instance->objectName();
			std::string displayObjectName;
			std::string tooltip;

			if (!projName.empty()) {
				displayObjectName = fmt::format("{} [{}]", objName, projName);
			} else {
				displayObjectName = objName;
			}

			if (auto prefabInstance = core::PrefabOperations::findContainingPrefabInstance(instance)) {
				tooltip = raco::core::Queries::getFullObjectHierarchyPath(instance);
			}

			items_.push_back({displayObjectName.c_str(), instance->objectID().c_str(), tooltip.c_str()});
			// This seems a little bit of an overkill
			objectNames_.push_back(parent_->dispatcher()->registerOn({instance, {"objectName"}}, [this]() { update(); }));
		}
	}
}

void PropertyBrowserRef::updateIndex() noexcept {
	if (parent_->valueHandle()) {
		if (parent_->valueHandle().asRef() == nullptr) {
			index_ = EMPTY_REF_INDEX;
		} else {
			auto it = std::find_if(items_.begin(), items_.end(), [this](const auto& item) {
				return item.objId == parent_->valueHandle().asRef()->objectID().c_str();
			});
			if (it == items_.end()) {
				index_ = EMPTY_REF_INDEX;
			} else {
				index_ = std::distance(items_.begin(), it);
			}
		}
	}
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
