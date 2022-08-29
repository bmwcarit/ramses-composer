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

#include "common_widgets/ObjectSearchView.h"

namespace raco::common_widgets {

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
	LinkStartSearchView(components::SDataChangeDispatcher dispatcher, core::Project* project, const core::ValueHandle& end, QWidget* parent);

	bool allowedStrong(const QModelIndex& index) const;
	bool allowedWeak(const QModelIndex& index) const;

protected:
	void rebuild() noexcept override;
};

}  // namespace raco::common_widgets
