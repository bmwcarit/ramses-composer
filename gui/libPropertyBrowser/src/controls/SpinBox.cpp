/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "property_browser/controls/SpinBox.h"

namespace raco::property_browser {

raco::property_browser::IntSpinBox::IntSpinBox(QWidget* parent) : SpinBox<int>{parent} {}

void raco::property_browser::IntSpinBox::emitValueChanged(int value) {
	Q_EMIT valueChanged(value);
}

void raco::property_browser::IntSpinBox::emitEditingFinished() {
	Q_EMIT editingFinished();
}

void raco::property_browser::IntSpinBox::emitFocusNextRequested() {
	Q_EMIT focusNextRequested();
}

raco::property_browser::DoubleSpinBox::DoubleSpinBox(QWidget* parent) : SpinBox<double>{parent} {
	widget_.setDecimals(5);
}

void raco::property_browser::DoubleSpinBox::emitValueChanged(double value) {
	Q_EMIT valueChanged(value);
}

void raco::property_browser::DoubleSpinBox::emitEditingFinished() {
	Q_EMIT editingFinished();
}

void raco::property_browser::DoubleSpinBox::emitFocusNextRequested() {
	Q_EMIT focusNextRequested();
}

}  // namespace raco::property_browser