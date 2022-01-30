/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "property_browser/editors/PropertyEditor.h"
#include "property_browser/PropertyBrowserItem.h"

namespace raco::property_browser {

PropertyEditor::PropertyEditor(
	PropertyBrowserItem* item,
	QWidget* parent)
	: QWidget{parent}, item_{item} {

	setEnabled(item->editable());
	QObject::connect(item, &PropertyBrowserItem::editableChanged, this, &QWidget::setEnabled);
}

}  // namespace raco::property_browser
