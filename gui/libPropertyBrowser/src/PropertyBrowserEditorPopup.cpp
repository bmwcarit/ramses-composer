/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "property_browser/PropertyBrowserEditorPopup.h"

#include "property_browser/PropertyBrowserItem.h"
#include "property_browser/PropertyBrowserLayouts.h"
#include "property_browser/PropertyBrowserModel.h"
#include "property_browser/PropertySubtreeChildrenContainer.h"
#include "style/Icons.h"

#include <QApplication>
#include <QDesktopWidget>

namespace raco::property_browser {

using namespace raco::style;

PropertyBrowserEditorPopup::PropertyBrowserEditorPopup(PropertyBrowserItem* item, QWidget* anchor, ObjectSearchView* list) : QDialog{anchor}, item_{item}, list_{list} {
	setWindowFlags(Qt::Popup);
	setAttribute(Qt::WA_DeleteOnClose);
	setSizeGripEnabled(true);

	acceptButton_.setFlat(true);
	acceptButton_.setIcon(Icons::instance().done);
	closeButton_.setFlat(true);
	closeButton_.setIcon(Icons::instance().close);

	frame_.setLineWidth(1);
	frame_.setFrameStyle(QFrame::Panel | QFrame::Raised);

	dataTypeLabel_.setAlignment(Qt::AlignCenter);
	dataTypeLabel_.setText(QString::fromStdString(item->luaTypeName()));

	outerLayout_.setContentsMargins(0, 0, 0, 0);
	outerLayout_.addWidget(&frame_, 0, 0, 1, 1);
	layout_.addWidget(&currentRelation_, 0, 0, 1, 2);
	layout_.addWidget(&deleteButton_, 0, 2);
	layout_.addWidget(&search_, 1, 0, 1, 3);
	layout_.addWidget(list_, 2, 0, 1, 3);
	layout_.addWidget(&acceptButton_, 3, 0);
	layout_.addWidget(&dataTypeLabel_, 3, 1);
	layout_.addWidget(&closeButton_, 3, 2);
	layout_.setColumnStretch(1, 1);

	search_.installEventFilter(this);
	search_.setFocus();
	acceptButton_.setEnabled(list_->hasValidSelection());

	QObject::connect(dynamic_cast<QApplication*>(QApplication::instance()), &QApplication::focusChanged, this, [this](QWidget* old, QWidget* now) { if (now != this && !this->isAncestorOf(now)) close(); });
	QObject::connect(&search_, &QLineEdit::textChanged, list_, &ObjectSearchView::setFilterByName);
	QObject::connect(&closeButton_, &QPushButton::clicked, this, &QWidget::close);
	QObject::connect(&deleteButton_, &QPushButton::clicked, this, [this]() { removeObjectRelation(); close(); });
	QObject::connect(list_, &ObjectSearchView::selectionChanged, &acceptButton_, &QPushButton::setEnabled);
	QObject::connect(&acceptButton_, &QPushButton::clicked, [this]() { establishObjectRelation(); close(); });
	QObject::connect(list_, &ObjectSearchView::clicked, [this]() { establishObjectRelation(); close(); });
	QObject::connect(list_, &ObjectSearchView::activated, [this]() { establishObjectRelation(); close(); });

	// center horizontally on button and keep on screen
	search_.setMinimumWidth(500);
	updateGeometry();
	QPoint parentLocation = anchor->parentWidget()->mapToGlobal(anchor->geometry().topLeft());
	QSize size = sizeHint();
	QRect screen = QApplication::desktop()->screenGeometry(anchor);
	int x = std::min((parentLocation.x() - size.width() / 2) + 25, screen.x() + screen.width() - size.width());
	int y = std::min(parentLocation.y(), screen.height() - size.height());
	move(x, y);

	show();
}


bool PropertyBrowserEditorPopup::eventFilter(QObject* obj, QEvent* event) {
	if (&search_ == obj) {
		if (event->type() == QEvent::KeyPress) {
			QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
			if (keyEvent->key() == Qt::Key_Down) {
				list_->setFocus();
				return true;
			}
		}
		return false;
	}
	return QDialog::eventFilter(obj, event);
}

void PropertyBrowserEditorPopup::keyPressEvent(QKeyEvent* event) {
	auto key{event->key()};
	if (key == Qt::Key_Escape) {
		close();
	} else if (key == Qt::Key_Enter || key == Qt::Key_Return) {
		if (list_->hasValidSelection()) {
			establishObjectRelation();
		} else if (search_.text().isEmpty() && !currentRelation_.text().isEmpty()) {
			removeObjectRelation();
			close();
		} else {
			close();
		}
	} else if ((key == Qt::UpArrow || key == Qt::Key_Up)) {
		search_.setFocus();
	}
}

}  // namespace raco::property_browser
