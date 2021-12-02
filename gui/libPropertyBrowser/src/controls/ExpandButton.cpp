/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "property_browser/controls/ExpandButton.h"

#include <QStyle>
#include <QWidget>

#include "style/Icons.h"
#include "property_browser/PropertyBrowserItem.h"

namespace raco::property_browser {

using namespace ::raco::style;

ExpandControlButton::ExpandControlButton(PropertyBrowserItem* item, QWidget* parent)
	: QPushButton{parent} {
	auto icon = Icons::icon(Pixmap::collapsed, this);
	setIcon(icon);
	setContentsMargins(0, 0, 0, 0);
	setFlat(true);

	QSizePolicy retainSizePolicy = sizePolicy();
	retainSizePolicy.setRetainSizeWhenHidden(true);
	setSizePolicy(retainSizePolicy);

	if (item->size() == 0) {
		setVisible(false);
		setMaximumHeight(0);
	}
		
	QObject::connect(item, &PropertyBrowserItem::childrenChanged, this, [this](const auto& children) {
		if (children.size() > 0) {
			setMaximumHeight(QWIDGETSIZE_MAX);
			setVisible(true);
		} else {
			setMaximumHeight(0);
			setVisible(false);
		}
	});

	QObject::connect(this, &ExpandControlButton::clicked, item, &PropertyBrowserItem::toggleExpanded);
	QObject::connect(item, &PropertyBrowserItem::expandedChanged, this, &ExpandControlButton::updateIcon);
	updateIcon(item->expanded());
}

void ExpandControlButton::updateIcon(bool expanded) {
	if (expanded) {
		setIcon(Icons::icon(Pixmap::expanded, this));
	} else {
		setIcon(Icons::icon(Pixmap::collapsed, this));
	}
}

}  // namespace raco::property_browser
