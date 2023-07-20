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

#include "PropertyEditor.h"
#include "property_browser/controls/ScalarSlider.h"
#include "property_browser/controls/SpinBox.h"

class QStackedWidget;

namespace raco::property_browser {

class PropertyBrowserItem;

class IntEditor : public PropertyEditor {
public:
	explicit IntEditor(
		PropertyBrowserItem* item,
		QWidget* parent = nullptr);

protected:
	QStackedWidget* stack_;
	IntSpinBox* spinBox_;
	IntSlider* slider_;

private:
	void setValueToControls(IntSlider* slider, IntSpinBox* spinBox) const;

	std::map<core::ValueHandle, int> focusInValues_;
};

}  // namespace raco::property_browser
