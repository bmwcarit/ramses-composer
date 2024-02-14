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

#include <QWidget>

namespace raco::property_browser {

template <class T>
T* findAncestor(QWidget* widget) {
	while (widget) {
		if (const auto typedWidget = qobject_cast<T*>(widget)) {
			return typedWidget;
		}
		widget = widget->parentWidget();
	}
	return nullptr;
}

}  // namespace raco::property_browser
