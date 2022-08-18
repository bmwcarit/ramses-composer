/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "property_browser/controls/ExpandButton.h"

#include <QGuiApplication>
#include <QStyle>
#include <QWidget>

#include "style/Icons.h"
#include "property_browser/PropertyBrowserItem.h"

namespace raco::property_browser {
using namespace ::raco::style;

ExpandButton::ExpandButton(PropertyBrowserItem* item, QWidget* parent)
	: QPushButton{parent} {
	setIcon(Icons::instance().collapsed);
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

	QObject::connect(this, &ExpandButton::clicked, this, [this, item]() {
		if (QGuiApplication::queryKeyboardModifiers().testFlag(Qt::KeyboardModifier::ShiftModifier)) {
			item->setExpandedRecursively(!item->expanded());
		} else {
			item->setExpanded(!item->expanded());
		}
	});

	QObject::connect(item, &PropertyBrowserItem::expandedChanged, this, &ExpandButton::updateIcon);
	updateIcon(item->expanded());
}

void ExpandButton::updateIcon(bool expanded) {
	if (expanded) {
		setIcon(Icons::instance().expanded);
	} else {
		setIcon(Icons::instance().collapsed);
	}
}

}  // namespace raco::property_browser
