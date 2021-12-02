/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "property_browser/editors/StringEditor.h"

#include <QBoxLayout>
#include <QColor>
#include <QDebug>
#include <QDesktopServices>
#include <QFocusEvent>
#include <QMenu>
#include <QObject>
#include <QPalette>
#include <QPushButton>

#include "style/Icons.h"

#include "common_widgets/NoContentMarginsLayout.h"
#include "core/Queries.h"
#include "property_browser/PropertyBrowserItem.h"
#include "style/Colors.h"

namespace raco::property_browser {

StringEditor::StringEditor(PropertyBrowserItem* item, QWidget* parent)
	: QWidget{parent} {
	this->setLayout(new raco::common_widgets::NoContentMarginsLayout<QHBoxLayout>(this));
	lineEdit_ = new StringEditorLineEdit(this);
	layout()->addWidget(lineEdit_);
	
	// connect item to QLineEdit
	QObject::connect(lineEdit_, &QLineEdit::editingFinished, item, [this, item]() { item->set(lineEdit_->text().toStdString()); });
	QObject::connect(item, &PropertyBrowserItem::valueChanged, this, [this, item](core::ValueHandle & handle) {
		lineEdit_->setText(handle.asString().c_str());
		this->updateErrorState(item);
		lineEdit_->update();
	});
	QObject::connect(item, &PropertyBrowserItem::errorChanged, this, [this, item](core::ValueHandle& handle) {
		this->updateErrorState(item);
	});
	QObject::connect(item, &PropertyBrowserItem::widgetRequestFocus, this, [this]() {
		lineEdit_->setFocus();
	});
	QObject::connect(lineEdit_, &StringEditorLineEdit::focusNextRequested, this, [this, item]() { item->requestNextSiblingFocus(); });

	lineEdit_->setText(item->valueHandle().asString().c_str());
	if (item->hasError()) {
		errorLevel_ = item->error().level();
		lineEdit_->setToolTip(item->error().message().c_str());
	} else {
		errorLevel_ = core::ErrorLevel::NONE;
	}
	// simple reset of outdate color on editingFinished
	QObject::connect(lineEdit_, &QLineEdit::editingFinished, this, [this]() {
		updatedInBackground_ = false;
	});
}

void StringEditor::updateErrorState(raco::property_browser::PropertyBrowserItem* item) {
	if (item->hasError()) {
		errorLevel_ = item->error().level();
		lineEdit_->setToolTip(item->error().message().c_str());
	} else {
		errorLevel_ = core::ErrorLevel::NONE;
		lineEdit_->setToolTip({});
	}
}

void StringEditor::setText(const QString& t) {
	if (lineEdit_->hasFocus() && editingStartedByUser() && t != lineEdit_->text()) {
		updatedInBackground_ = true;
	} else {
		lineEdit_->setText(t);
	}
}

bool StringEditor::editingStartedByUser() {
	return lineEdit_->isModified() || lineEdit_->cursorPosition() != lineEdit_->text().size();
}

bool StringEditor::updatedInBackground() const {
	return updatedInBackground_;
}

int StringEditor::errorLevel() const noexcept {
	return static_cast<int>(errorLevel_);
}

void StringEditorLineEdit::focusInEvent(QFocusEvent* event) {
	this->selectAll();
	focusInOldText_ = text();
	QLineEdit::focusInEvent(event);
}

void StringEditorLineEdit::keyPressEvent(QKeyEvent* event) {
	QLineEdit::keyPressEvent(event);

	if (event->key() == Qt::Key_Escape) {
		setText(focusInOldText_);
		clearFocus();
	} else if (event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return) {
		clearFocus();
		Q_EMIT focusNextRequested();
	}
}

}  // namespace raco::property_browser
