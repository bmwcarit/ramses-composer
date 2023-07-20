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

class QComboBox;

namespace raco::property_browser {
class PropertyBrowserItem;

class EnumerationEditor : public PropertyEditor {
public:
	explicit EnumerationEditor(PropertyBrowserItem* item, QWidget* parent);

	int currentIndex() const;

protected:
	QComboBox* comboBox_;
	std::vector<int> comboBoxIndexToRamsesEnumIndex_;
	std::map<int, int> ramsesEnumIndexToComboBoxIndex_;
};
}  // namespace raco::property_browser
