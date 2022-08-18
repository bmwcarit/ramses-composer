/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "common_widgets/PropertyBrowserButton.h"

#include <QVariant>

namespace raco::common_widgets {

PropertyBrowserButton::PropertyBrowserButton(const QString &text, QWidget *parent)
	: QPushButton(text, parent) {
	setUp();
}

PropertyBrowserButton::PropertyBrowserButton(const QIcon &icon, const QString &text, QWidget *parent)
	: QPushButton(icon, text, parent) {
	setUp();
}

void PropertyBrowserButton::setUp() {
	setFlat(true);
	setMaximumWidth(MAXIMUM_WIDTH_PX);
	setProperty("slimButton", true);
}

}  // namespace raco::common_widgets
