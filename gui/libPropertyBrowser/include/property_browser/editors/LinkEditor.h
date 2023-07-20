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

#include <QLineEdit>
#include <QPushButton>
#include <QMenu>
#include <QWidget>

#include "core/Queries.h"

#include "property_browser/PropertyBrowserItem.h"
#include "property_browser/PropertyBrowserLayouts.h"
#include "style/Icons.h"

namespace raco::property_browser {

class LinkEditor : public QWidget {
	using LinkState = core::Queries::LinkState;

public:
	
	Q_PROPERTY(bool validDropTarget MEMBER validDropTarget_);

	explicit LinkEditor(
		PropertyBrowserItem* item,
		QWidget* parent = nullptr);
	void setControl(QWidget* widget);

public Q_SLOTS:
	void setExpanded(bool expanded);

	enum class LinkIcon {
		linkable,
		linkBroken,
		linked,
		linkMultipleValues,
		parentIsLinked,
		unlinkable,
		doubleArrowLeft,
		goTo,
		goToLeftRight,
		singleArrowLeft
	};

protected:
	void dragEnterEvent(QDragEnterEvent *event) override;
    void dragLeaveEvent(QDragLeaveEvent *event) override;
	void dropEvent(QDropEvent *event) override;

	QPushButton* linkButton_;
	QPushButton* goToLinkButton_;

	LinkIcon linkButtonIcon_;
	LinkIcon goToLinkButtonIcon_;

protected Q_SLOTS:
	void linkButtonClicked();
	void setLinkState();

private:
	std::map<LinkIcon, QIcon> LinkStateIcons_{
		{LinkIcon::unlinkable, style::Icons::instance().unlinkable},
		{LinkIcon::linkable, style::Icons::instance().linkable},
		{LinkIcon::linked, style::Icons::instance().linked},
		{LinkIcon::linkBroken, style::Icons::instance().linkBroken},
		{LinkIcon::linkMultipleValues, style::Icons::instance().linkMultipleValues},
		{LinkIcon::parentIsLinked, style::Icons::instance().parentIsLinked},
		{LinkIcon::goTo, style::Icons::instance().goTo},
		{LinkIcon::singleArrowLeft, style::Icons::instance().singleArrowLeft},
		{LinkIcon::doubleArrowLeft, style::Icons::instance().doubleArrowLeft},
		{LinkIcon::goToLeftRight, style::Icons::instance().goToLeftRight}
	};

	void addLinkEndpointMenuItems(const std::vector<core::SLink>& startingLinks, QMenu* endsMenu, QString& requestedLinkEndObj);
	std::map<std::string, std::string> generateSortedLinkPoints(const std::vector<core::SLink> links);

	bool validDropTarget_ { false };
	PropertyBrowserItem* item_;
	QMetaObject::Connection goToLinkButtonConnection_;
	PropertyBrowserGridLayout* layout_;
};

}  // namespace raco::property_browser
