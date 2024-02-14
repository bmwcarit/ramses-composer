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
#include "property_browser/PropertyBrowserItem.h"

#include <QPushButton>
#include <QLabel>

namespace raco::property_browser {

class ArrayEditor : public PropertyEditor {
public:
	explicit ArrayEditor(PropertyBrowserItem* item, QWidget* parent = nullptr);

protected:
	void updateLabel();
	
	QLabel* descriptionLabel_;
	QPushButton* shrinkButton_;
	QPushButton* growButton_;
	QLabel* sizeLabel_;
};

}  // namespace raco::property_browser