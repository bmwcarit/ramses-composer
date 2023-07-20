/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "property_browser/LinkStartSearchView.h"

#include "property_browser/PropertyBrowserUtilities.h"

#include "core/Queries.h"
#include "core/Project.h"

namespace raco::property_browser {

LinkStartSearchView::LinkStartSearchView(components::SDataChangeDispatcher dispatcher, core::Project* project, const std::set<core::ValueHandle>& endHandles, QWidget* parent) 
	: ObjectSearchView(dispatcher, project, endHandles, parent),
	  project_(project),
	  objects_(endHandles),
	  projectChanges_{dispatcher->registerOnObjectsLifeCycle(
		  [this, dispatcher](core::SEditorObject obj) {
			  if (core::Queries::typeHasStartingLinks(obj)) {
                outputsChanges_[obj] = dispatcher->registerOnPreviewDirty(obj, [this]() {
                    rebuild();
                });
                rebuild();
            } },
		  [this](core::SEditorObject obj) {
			  if (outputsChanges_.find(obj) != outputsChanges_.end()) {
				  outputsChanges_.erase(outputsChanges_.find(obj));
				  rebuild();
			  }
		  })} {

	for (auto obj : project_->instances()) {
		if (core::Queries::typeHasStartingLinks(obj)) {
			outputsChanges_[obj] = dispatcher->registerOnPreviewDirty(obj, [this]() {
				rebuild();
			});
		}
	}

	// TODO missing subscriptions:
	// - link lifecycle (loops)
	// - weak <-> strong link change
	// - move scenegraph nodes

	rebuild();
	updateSelection();
}

bool LinkStartSearchView::allowedStrong(const QModelIndex& index) const {
	return (dynamic_cast<const LinkStartViewItem*>(model_.itemFromIndex(index)))->allowedStrong_;
}

bool LinkStartSearchView::allowedWeak(const QModelIndex& index) const {
	return (dynamic_cast<const LinkStartViewItem*>(model_.itemFromIndex(index)))->allowedWeak_;
}

void LinkStartSearchView::rebuild() noexcept {
	model_.clear();

	auto linkStartProperties = map_reduce<std::set<std::tuple<core::ValueHandle, bool, bool>>>(
		objects_,
		intersection<std::set<std::tuple<core::ValueHandle, bool, bool>>>,
		[this](auto object) {
			return core::Queries::allLinkStartProperties(*project_, object);
	});

	for (const auto& [handle, strong, weak] : linkStartProperties) {
		QString title{handle.getPropertyPath().c_str()};
		if (weak && !strong) {
			title.append(" (weak)");
		}
		auto* item{new LinkStartViewItem{title, handle, strong, weak}};
		model_.appendRow(item);
	}
}

}  // namespace raco::property_browser