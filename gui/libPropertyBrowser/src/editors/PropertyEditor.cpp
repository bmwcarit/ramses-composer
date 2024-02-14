/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "property_browser/editors/PropertyEditor.h"
#include "property_browser/PropertyBrowserItem.h"
#include "property_browser/PropertyCopyPaste.h"
#include <QMouseEvent>
#include <QMenu>

namespace raco::property_browser {

PropertyEditor::PropertyEditor(PropertyBrowserItem* item, QWidget* parent)
	: QWidget{parent},
	  item_{item} {
	setEnabled(item->editable());
	QObject::connect(item, &PropertyBrowserItem::editableChanged, this, &QWidget::setEnabled);

	propertyMenu_ = new QMenu(this);
	propertyMenu_->addAction("Copy", this, &PropertyEditor::menuCopyAction);
}

bool PropertyEditor::eventFilter(QObject* watched, QEvent* event) {
	if (canDisplayCopyDialog && event->type() == QEvent::MouseButtonRelease) {
		const auto* mouseEvent = dynamic_cast<QMouseEvent*>(event);
		if (mouseEvent != nullptr && mouseEvent->button() == Qt::RightButton) {
			const auto* widget = dynamic_cast<QWidget*>(watched);
			if (widget != nullptr && !widget->isEnabled()) {
				displayCopyContextMenu();
				return true;
			}
		}
	}
	return QWidget::eventFilter(watched, event);
}

void PropertyEditor::displayCopyContextMenu() {
	propertyMenu_->exec(QCursor::pos());
}

void PropertyEditor::menuCopyAction() {
	PropertyCopyPaste::copyValuePlainText(item_);
}


}  // namespace raco::property_browser
