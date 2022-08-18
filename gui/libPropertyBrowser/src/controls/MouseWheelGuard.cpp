/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "property_browser/controls/MouseWheelGuard.h"

#include <QEvent>

namespace raco::property_browser {

bool MouseWheelGuard::eventFilter(QObject* o, QEvent* e) {
	if (e->type() == QEvent::Wheel) {
		e->ignore();
		return true;
	}

	return QObject::eventFilter(o, e);
}

}  // namespace raco::property_browser
