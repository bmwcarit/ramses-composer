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

#include <QLineEdit>
#include <QPushButton>
#include <QMenu>
#include <QWidget>

#include "core/Queries.h"

#include "property_browser/PropertyBrowserItem.h"
#include "property_browser/PropertyBrowserLayouts.h"

namespace raco::property_browser {

class LinkEditor final : public QWidget {
	using LinkState = raco::core::Queries::LinkState;

public:
	Q_PROPERTY(bool validDropTarget MEMBER validDropTarget_);

	explicit LinkEditor(
		PropertyBrowserItem* item,
		QWidget* parent = nullptr);
	void setControl(QWidget* widget);

public Q_SLOTS:
	void setExpanded(bool expanded);

protected:
	void dragEnterEvent(QDragEnterEvent *event) override;
    void dragLeaveEvent(QDragLeaveEvent *event) override;
	void dropEvent(QDropEvent *event) override;

protected Q_SLOTS:
	void linkButtonClicked();
	void setLinkState(const LinkState& linkstate);

private:
	bool validDropTarget_ { false };
	PropertyBrowserItem* item_;
	QPushButton* linkButton_;
	PropertyBrowserGridLayout* layout_;
};

}  // namespace raco::property_browser
