/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "common_widgets/LinkStartSearchView.h"

namespace raco::common_widgets {

LinkStartSearchView::LinkStartSearchView(components::SDataChangeDispatcher dispatcher, core::Project* project, const core::ValueHandle& end, QWidget* parent) : ObjectSearchView(dispatcher, project, end, parent) {
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
	for (const auto& [handle, strong, weak] : core::Queries::allLinkStartProperties(*project_, obj_)) {
		QString title{handle.getPropertyPath().c_str()};
		if (weak && !strong) {
			title.append(" (weak)");
		}
		auto* item{new LinkStartViewItem{title, handle, strong, weak}};
		model_.appendRow(item);
	}
}

}  // namespace raco::common_widgets
