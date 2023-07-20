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

#include "property_browser/ObjectSearchView.h"

namespace raco::property_browser {

class LinkStartViewItem final : public ObjectSearchViewItem {
public:
	LinkStartViewItem(const QString& s, const core::ValueHandle& handle, bool allowedStrong, bool allowedWeak)
		: ObjectSearchViewItem(s, handle), allowedStrong_(allowedStrong), allowedWeak_(allowedWeak) {
		setToolTip(QString::fromStdString(handle_.getDescriptor().getFullPropertyPath()));
	}
	bool allowedStrong_;
	bool allowedWeak_;
};

/**
 * Basic Widget to display all properties which have a LinkStartAnnotation.
 */
class LinkStartSearchView : public ObjectSearchView {
	Q_OBJECT
public:
	LinkStartSearchView(components::SDataChangeDispatcher dispatcher, core::Project* project, const std::set<core::ValueHandle>& endHandles, QWidget* parent);

	bool allowedStrong(const QModelIndex& index) const;
	bool allowedWeak(const QModelIndex& index) const;

protected:
	void rebuild() noexcept override;

	core::Project* project_;
	std::set<core::ValueHandle> objects_;

	components::Subscription projectChanges_;
	std::map<core::SEditorObject, raco::components::Subscription> outputsChanges_;
};

}  // namespace raco::property_browser